#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>
#include <linux/dma-mapping.h>
#include <linux/io.h>
#include <linux/of_reserved_mem.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/poll.h>

#define DRIVER_NAME "msgdma_platform_test"

// mSGDMA standard descriptor, Embedded Peripherals IP User Guide 25.3, pg. 474, 28.13.2. Descriptor Format
struct msgdma_desc {
    u32 read_addr;
    u32 write_addr;
    u32 length;
    u32 next_desc;
    u32 actual_len;
    u32 status;
    u32 reserved;
    u32 control;
};

struct msgdma_dev {
    void __iomem *csr;
    void __iomem *prefetcher;
    
    // Data Buffer (0x38000000)
    void *buffer_virt;
    dma_addr_t buffer_phys;
    size_t buffer_size;

    // Descriptor Memory
    struct msgdma_desc *desc_virt;
    dma_addr_t desc_phys;

    struct device *dev;
    struct miscdevice miscdev;

    // irq
    wait_queue_head_t wait_queue;
    int data_ready;
};

static struct msgdma_dev *to_mdev(struct file *file) {
    return container_of(file->private_data, struct msgdma_dev, miscdev);
}

static __poll_t msgdma_poll(struct file *file, poll_table *wait)
{
    struct msgdma_dev *mdev = to_mdev(file);
    __poll_t mask = 0;

    // Register the wait queue with the poll system
    poll_wait(file, &mdev->wait_queue, wait);

    // If data is ready, set the POLLIN bit
    if (mdev->data_ready) {
        mask |= EPOLLIN | EPOLLRDNORM;
    }
    return mask;
}

static irqreturn_t msgdma_irq_handler(int irq, void *dev_id) {
    struct msgdma_dev *mdev = dev_id;
    u32 status;
    status = ioread32(mdev->prefetcher + 0x10);
    // pr_info(DRIVER_NAME ": IRQ triggered! Status: 0x%08x\n", status);
    mdev->data_ready = 1;
    wake_up_interruptible(&mdev->wait_queue);
    iowrite32(status, mdev->prefetcher + 0x10); // Reset the IRQ (Write 1 to clear bit 0)
    return IRQ_HANDLED;
}

static int msgdma_mmap(struct file *file, struct vm_area_struct *vma)
{
    struct miscdevice *mptr = file->private_data;
    struct msgdma_dev *mdev = container_of(mptr, struct msgdma_dev, miscdev);
    unsigned long pgoff = vma->vm_pgoff;
    if (pgoff == 0) {
        return dma_mmap_coherent(mdev->dev, vma, mdev->buffer_virt, mdev->buffer_phys, mdev->buffer_size);
    } else if (pgoff == 16) { // 1 page offset = 0x1000 bytes
        vma->vm_pgoff = 0; 
        return dma_mmap_coherent(mdev->dev, vma, mdev->desc_virt, mdev->desc_phys, PAGE_SIZE);
    }
    return -ENXIO;
}

static ssize_t msgdma_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    struct miscdevice *mptr = file->private_data;
    struct msgdma_dev *mdev = container_of(mptr, struct msgdma_dev, miscdev);
    if (wait_event_interruptible(mdev->wait_queue, mdev->data_ready != 0))
        return -ERESTARTSYS;
    mdev->data_ready = 0;
    return 0;
}

static int msgdma_reset_prefetcher(struct msgdma_dev *mdev) {
    u32 ctrl;
    int timeout = 1000;

    ctrl = ioread32(mdev->prefetcher + 0x00);
    iowrite32(ctrl | (1 << 2), mdev->prefetcher + 0x00);

    while (timeout > 0) {
        ctrl = ioread32(mdev->prefetcher + 0x00);
        if (!(ctrl & (1 << 2))) {
            return 0;
        }
        udelay(1);
        timeout--;
    }
    pr_err(DRIVER_NAME ": Prefetcher reset timed out!\n");
    return -ETIMEDOUT;
}

static int msgdma_reset_dispatcher(struct msgdma_dev *mdev) {
    u32 ctrl;
    u32 stat;
    int timeout = 1000;

    ctrl = ioread32(mdev->csr + 0x04);
    iowrite32(ctrl | (1 << 1), mdev->csr + 0x04);

    while (timeout > 0) {
        stat = ioread32(mdev->csr + 0x00);
        if (!(stat & (1 << 6))) {
            return 0;
        }
        udelay(1);
        timeout--;
    }
    pr_err(DRIVER_NAME ": Dispatcher reset timed out!\n");
    return -ETIMEDOUT;
}

static int msgdma_open(struct inode *inode, struct file *file)
{
    struct miscdevice *mptr = file->private_data;
    struct msgdma_dev *mdev = container_of(mptr, struct msgdma_dev, miscdev);

//  Reset dma engine to a defined state
//  Resetting Prefetcher Core Flow, 28.13.5.2.  pg. 483
    if (msgdma_reset_prefetcher(mdev)) {return -EIO;}
    if (msgdma_reset_dispatcher(mdev)) {return -EIO;}

    mdev->data_ready = 0;

    int i;
    size_t desc_size = sizeof(struct msgdma_desc);
    u32 slot_size = 1024;
    u32 max_msg_bytes = 960;
    for (i = 0; i < 64; i++) {
        struct msgdma_desc *d = &mdev->desc_virt[i];
        d->write_addr = mdev->buffer_phys + (i * slot_size);
        d->length = max_msg_bytes;
        if (i == 63) {
            d->next_desc = mdev->desc_phys;
        } else {
            d->next_desc = mdev->desc_phys + ((i + 1) * desc_size);
        }
        d->control = (1 << 30) | (1 << 14) | (1 << 12);
    }

    iowrite32(lower_32_bits(mdev->desc_phys), mdev->prefetcher + 0x04); // Restart the Prefetcher from Descriptor 0
    iowrite32(upper_32_bits(mdev->desc_phys), mdev->prefetcher + 0x08);
    iowrite32(400, mdev->prefetcher + 0x0C); // poll freq in clk cycles
    iowrite32(0x0b, mdev->prefetcher + 0x00); // Global Interrupt Enable Mask, Desc_poll_en, Run

    pr_info(DRIVER_NAME ": Device opened, mSGDMA reset to Slot 0\n");
    return nonseekable_open(inode, file);
}

static const struct file_operations msgdma_fops = {
    .owner = THIS_MODULE,
    .open  = msgdma_open,
    .mmap  = msgdma_mmap,
    .read  = msgdma_read,
    .poll  = msgdma_poll,
};

static int msgdma_probe(struct platform_device *pdev)
{
    struct msgdma_dev *mdev;
    struct resource *res;
 
    pr_info(DRIVER_NAME ": Probing mSGDMA\n");

    mdev = devm_kzalloc(&pdev->dev, sizeof(*mdev), GFP_KERNEL);
    if (!mdev) return -ENOMEM;
    mdev->dev = &pdev->dev;

    // Map CSR Base (Reg 0 in DT)
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res) return -EINVAL;
    mdev->csr = devm_ioremap(&pdev->dev, res->start, resource_size(res));
    if (!mdev->csr) return -ENOMEM;

    // Map Prefetcher Base (Reg 1 in DT)
    res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
    if (!res) return -EINVAL;
    mdev->prefetcher = devm_ioremap(&pdev->dev, res->start, resource_size(res));
    if (!mdev->prefetcher) return -ENOMEM;

    int irq;
    irq = platform_get_irq(pdev, 0);
    if (irq < 0) {
        dev_err(&pdev->dev, "Failed to get IRQ from DT\n");
        return irq;
    }

    int ret;
    ret = devm_request_irq(&pdev->dev, irq, msgdma_irq_handler, 0, DRIVER_NAME, mdev);
    if (ret) {
        dev_err(&pdev->dev, "Failed to request IRQ %d\n", irq);
        return ret;
    }

    // irq wait queue
    init_waitqueue_head(&mdev->wait_queue);
    mdev->data_ready = 0;

    // Setup DMA
    ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32));
    if (ret) return ret;
    ret = of_reserved_mem_device_init(&pdev->dev);
    if (ret) return ret;

    mdev->buffer_size = 64*1024;
    mdev->buffer_virt = dma_alloc_coherent(&pdev->dev, mdev->buffer_size, &mdev->buffer_phys, GFP_KERNEL);
    if (!mdev->buffer_virt) {
        of_reserved_mem_device_release(&pdev->dev);
        return -ENOMEM;
    }

    mdev->miscdev.minor = MISC_DYNAMIC_MINOR;
    mdev->miscdev.name = "msgdma_test"; // This creates /dev/msgdma_test
    mdev->miscdev.fops = &msgdma_fops;
    mdev->miscdev.parent = &pdev->dev;

    ret = misc_register(&mdev->miscdev);
    if (ret) {
        dev_err(&pdev->dev, "Failed to register misc device\n");
        dma_free_coherent(&pdev->dev, mdev->buffer_size, mdev->buffer_virt, mdev->buffer_phys);
        return ret;
    }

    dev_info(&pdev->dev, "mmap device registered at /dev/%s\n", mdev->miscdev.name);
    dev_info(&pdev->dev, "Buffer Virt: %px, Phys: %pad\n", mdev->buffer_virt, &mdev->buffer_phys);

    // Clear the target RAM so we can see the counter appear
    memset(mdev->buffer_virt, 0, 1024);

    int i;
    size_t desc_size = sizeof(struct msgdma_desc);
    u32 slot_size = 1024; // Assuming 1KB per message slot in your 4KB buffer
    u32 max_msg_bytes = 960; // 720 bytes (The most the FPGA will send)

    mdev->desc_virt = dma_alloc_coherent(&pdev->dev, PAGE_SIZE, &mdev->desc_phys, GFP_KERNEL);
    if (!mdev->desc_virt) return -ENOMEM;
    memset(mdev->desc_virt, 0, PAGE_SIZE);

    for (i = 0; i < 64; i++) {
        struct msgdma_desc *d = &mdev->desc_virt[i];
        d->write_addr = mdev->buffer_phys + (i * slot_size);
        d->length = max_msg_bytes;
        if (i == 63) {
            d->next_desc = mdev->desc_phys; // If it's the last one, link back to the start
        } else {
            d->next_desc = mdev->desc_phys + ((i + 1) * desc_size);
        }
        d->control = (1 << 30) | (1 << 14) | (1 << 12);
    }


    dev_info(&pdev->dev, "Submitting descriptor at phys %pad\n", &mdev->desc_phys);

    iowrite32(lower_32_bits(mdev->desc_phys), mdev->prefetcher + 0x04); // Feed the Descriptor address to the Prefetcher
    iowrite32(upper_32_bits(mdev->desc_phys), mdev->prefetcher + 0x08);
    iowrite32(400, mdev->prefetcher + 0x0C); // poll freq in clk cycles

    u32 status;
    status = ioread32(mdev->csr + 0x00); // Dispatcher Status
    dev_info(&pdev->dev, "mSGDMA Status: 0x%08x\n", status);

    // {
    //     u64 *data = (u64 *)mdev->buffer_virt;
    //     int j;
    //     for (j = 0; j < 24; j++) {
    //         dev_info(&pdev->dev, "data content[%d]: 0x%016llx\n", j, data[j]);
    //     }
    // }

    platform_set_drvdata(pdev, mdev);
    return 0;
}

static void msgdma_remove(struct platform_device *pdev) {
    struct msgdma_dev *mdev = platform_get_drvdata(pdev);
    if (!mdev) return;

    misc_deregister(&mdev->miscdev);

    if (mdev->csr) iowrite32(0x02, mdev->csr + 0x04); // stop dispatcher

    if (mdev->desc_virt) { dma_free_coherent(&pdev->dev, PAGE_SIZE, mdev->desc_virt, mdev->desc_phys); }
    if (mdev->buffer_virt) { dma_free_coherent(&pdev->dev, mdev->buffer_size, mdev->buffer_virt, mdev->buffer_phys); }

    of_reserved_mem_device_release(&pdev->dev);
    dev_info(&pdev->dev, "Driver removed and memory cleaned up\n");
}

static const struct of_device_id msgdma_ids[] = {
    { .compatible = "altr,msgdma-1.0", },
    { }
};

MODULE_DEVICE_TABLE(of, msgdma_ids);

static struct platform_driver msgdma_driver = {
    .driver = {
        .name = DRIVER_NAME,
        .of_match_table = msgdma_ids,
    },
    .probe = msgdma_probe,
    .remove = msgdma_remove,
};

module_platform_driver(msgdma_driver);

MODULE_LICENSE("GPL");

