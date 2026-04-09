#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h> 
#include <linux/dma-mapping.h>
#include <linux/io.h>

#include <linux/of_reserved_mem.h>

#include <linux/delay.h>

#include <linux/miscdevice.h>
#include <linux/fs.h>

#define DRIVER_NAME "msgdma_platform_test"
#define START_BIT   0x09  // Global Interrupt Enable Mask, Run
// The mSGDMA descriptor structure (Standard/Extended)
struct msgdma_desc {
    u32 reserved0;
    u32 write_addr;
    u32 length;
    u32 next_desc;
    u32 reserved7;
    u32 reserved8;
    u32 reserved;
    u32 control;
};

struct msgdma_dev {
    void __iomem *csr;
    void __iomem *prefetcher;
    
    // The Data Buffer (0x38000000)
    void *buffer_virt;
    dma_addr_t buffer_phys;
    size_t buffer_size;

    // The Descriptor Memory (New)
    struct msgdma_desc *desc_virt;
    dma_addr_t desc_phys;

    struct device *dev; // store &pdev->dev here
    struct miscdevice miscdev;
};


static struct msgdma_dev *to_mdev(struct file *file) {
    return container_of(file->private_data, struct msgdma_dev, miscdev);
}

// static irqreturn_t msgdma_irq_handler(int irq, void *dev_id)
// {
//     struct msgdma_dev *mdev = dev_id;
//     u32 status;
//
//     // 1. Read status to see what happened
//     status = ioread32(mdev->prefetcher + 0x10);
//
//     // 2. Print the message (Note: Use pr_info_ratelimited if data is fast!)
//     pr_info(DRIVER_NAME ": IRQ triggered! Status: 0x%08x\n", status);
//
//     // 3. Reset the IRQ (Write 1 to clear bit 0)
//     // We write the whole status back to clear any other sticky bits (like EOP)
//     iowrite32(status, mdev->prefetcher + 0x10);
//
//     return IRQ_HANDLED;
// }

static int msgdma_mmap(struct file *file, struct vm_area_struct *vma)
{
    struct miscdevice *mptr = file->private_data;
    struct msgdma_dev *mdev = container_of(mptr, struct msgdma_dev, miscdev);

    // vma->vm_page_prot should be set for non-cached access by dma_mmap_coherent
    return dma_mmap_coherent(mdev->dev, vma, 
                             mdev->buffer_virt, 
                             mdev->buffer_phys, 
                             mdev->buffer_size);
}

static const struct file_operations msgdma_fops = {
    .owner          = THIS_MODULE,
    .mmap           = msgdma_mmap,
    .open           = nonseekable_open,
};

static int msgdma_probe(struct platform_device *pdev)
{
    struct msgdma_dev *mdev;
    struct resource *res;
    int ret;
 
    pr_info(DRIVER_NAME ": Probing mSGDMA\n");

    mdev = devm_kzalloc(&pdev->dev, sizeof(*mdev), GFP_KERNEL);
    if (!mdev) return -ENOMEM;

    mdev->dev = &pdev->dev; // Save for dma_mmap_coherent

    // 1. Map CSR Base (Reg 0 in DT)
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res) return -EINVAL;
    mdev->csr = devm_ioremap(&pdev->dev, res->start, resource_size(res));
    if (!mdev->csr) return -ENOMEM;

    // 2. Map Prefetcher Base (Reg 1 in DT)
    res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
    if (!res) return -EINVAL;
    mdev->prefetcher = devm_ioremap(&pdev->dev, res->start, resource_size(res));
    if (!mdev->prefetcher) return -ENOMEM;

    // 3. Setup DMA
    ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32));
    if (ret) return ret;
    ret = of_reserved_mem_device_init(&pdev->dev);
    if (ret) return ret;

    mdev->buffer_size = 0x1000;
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


    struct msgdma_desc *desc;
    dma_addr_t desc_phys;
    u32 status;

    iowrite32(0x08, mdev->csr + 0x04); // stop on early termination // TODO remove?
    // if (mdev->csr) iowrite32(0x02, mdev->csr + 0x04);

    // 1. Clear the target RAM so we can see the counter appear
    memset(mdev->buffer_virt, 0, 1024);

    // 2. Allocate a small piece of coherent memory for the descriptor itself
    // The prefetcher needs to read this descriptor from RAM
    desc = dma_alloc_coherent(&pdev->dev, sizeof(*desc), &desc_phys, GFP_KERNEL);
    if (!desc) return -ENOMEM;

    // 3. Fill the descriptor
    desc->write_addr = mdev->buffer_phys;  // 0x38000000
    desc->length     = 192;
    desc->control    = (1 << 30) | (1 << 14) | (1 << 12); // owned_by_hw, transfer complete IRQ, end on eop

    dev_info(&pdev->dev, "Submitting descriptor at phys %pad\n", &desc_phys);

    // 4. Feed the Descriptor address to the Prefetcher
    iowrite32(lower_32_bits(desc_phys), mdev->prefetcher + 0x04);
    iowrite32(upper_32_bits(desc_phys), mdev->prefetcher + 0x08);
    
    // 5. Start the Prefetcher (Writing 1 to Control/Status reg at 0x00)
    iowrite32(START_BIT, mdev->prefetcher + 0x00);

    // 6. Wait a moment for FPGA to stream data
    msleep(10);

    iowrite32(0x01, mdev->prefetcher + 0x10); // clear IRQ // TODO remove later, must be done from somewhere else

    // 7. Check Status
    status = ioread32(mdev->csr + 0x00); // Dispatcher Status
    dev_info(&pdev->dev, "mSGDMA Status: 0x%08x\n", status);

    // 8. Print the results
    {
        u64 *data = (u64 *)mdev->buffer_virt;
        int j;
        for (j = 0; j < 24; j++) {
            dev_info(&pdev->dev, "data content[%d]: 0x%016llx\n", j, data[j]);
        }
    }

    // Cleanup descriptor (it has been read into FPGA internal FIFO now)
    dma_free_coherent(&pdev->dev, sizeof(*desc), desc, desc_phys);

    // 4. Basic Reset
    // iowrite32(0x02, mdev->csr + 0x04);
    //
    platform_set_drvdata(pdev, mdev);
    return 0;
}

static void msgdma_remove(struct platform_device *pdev) {
    struct msgdma_dev *mdev = platform_get_drvdata(pdev);
    if (!mdev) return;

    misc_deregister(&mdev->miscdev); // <--- Add this!

    // 1. Stop the mSGDMA Dispatcher (Reset it)
    if (mdev->csr) iowrite32(0x02, mdev->csr + 0x04);

    // 2. Free the Descriptor memory
    if (mdev->desc_virt) { dma_free_coherent(&pdev->dev, sizeof(struct msgdma_desc), mdev->desc_virt, mdev->desc_phys); }

    // 3. Free the Data Buffer (the 0x38000000 one)
    if (mdev->buffer_virt) { dma_free_coherent(&pdev->dev, mdev->buffer_size, mdev->buffer_virt, mdev->buffer_phys); }

    // 4. Release the reserved memory handle
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

