#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h> 
#include <linux/dma-mapping.h>
#include <linux/io.h>

#include <linux/of_reserved_mem.h>

#include <linux/delay.h>
#define DRIVER_NAME "msgdma_platform_test"
#define START_BIT   0x01
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
// struct msgdma_desc {
//     u32 read_addr;
//     u32 write_addr;
//     u32 length;
//     u32 control;
// }; // Note: Ensure this matches your QSys "Descriptor Configuration" (Standard vs Extended)

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
};

static int msgdma_probe(struct platform_device *pdev)
{
    struct msgdma_dev *mdev;
    struct resource *res;
    int ret;
 
    pr_info(DRIVER_NAME ": Probing mSGDMA\n");

    mdev = devm_kzalloc(&pdev->dev, sizeof(*mdev), GFP_KERNEL);
    if (!mdev) return -ENOMEM;

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

    dev_info(&pdev->dev, "Buffer Virt: %px, Phys: %pad\n", mdev->buffer_virt, &mdev->buffer_phys);






    struct msgdma_desc *desc;
    dma_addr_t desc_phys;
    u32 status;

    // 1. Clear the target RAM so we can see the counter appear
    memset(mdev->buffer_virt, 0, 1024);

    // 2. Allocate a small piece of coherent memory for the descriptor itself
    // The prefetcher needs to read this descriptor from RAM
    desc = dma_alloc_coherent(&pdev->dev, sizeof(*desc), &desc_phys, GFP_KERNEL);
    if (!desc) return -ENOMEM;

    // 3. Fill the descriptor
    // desc->read_addr  = 0x00000000;         // Not used for ST->MM
    desc->write_addr = mdev->buffer_phys;  // 0x38000000
    // desc->length     = 128;                /* Transfer 128 bytes (16 64-bit counts) */
    desc->length     = 32;
    
    /* Control bits: 
       Bit 31: GO (1)
       Bit 24: Write Fixed Address (0 for incrementing, usually 0 for RAM)
       Bit 30: Wait for Dispatcher (1) - recommended for prefetcher
    */
    desc->control    = (1 << 31) | (1 << 30); 

    dev_info(&pdev->dev, "Submitting descriptor at phys %pad\n", &desc_phys);

    // 4. Feed the Descriptor address to the Prefetcher
    // Prefetcher registers: 0x04 = Low Address, 0x08 = High Address
    iowrite32(lower_32_bits(desc_phys), mdev->prefetcher + 0x04);
    iowrite32(upper_32_bits(desc_phys), mdev->prefetcher + 0x08);
    
    // 5. Start the Prefetcher (Writing 1 to Control/Status reg at 0x00)
    iowrite32(START_BIT, mdev->prefetcher + 0x00);

    // 6. Wait a moment for FPGA to stream data
    msleep(10);

    // 7. Check Status
    status = ioread32(mdev->csr + 0x00); // Dispatcher Status
    dev_info(&pdev->dev, "mSGDMA Status: 0x%08x\n", status);

    // 8. Print the results
    {
        u64 *data = (u64 *)mdev->buffer_virt;
        int j;
        for (j = 0; j < 4; j++) {
            dev_info(&pdev->dev, "Counter[%d]: 0x%016llx\n", j, data[j]);
        }
    }

    // Cleanup descriptor (it has been read into FPGA internal FIFO now)
    dma_free_coherent(&pdev->dev, sizeof(*desc), desc, desc_phys);




    // 4. Basic Reset
    // iowrite32(0x02, mdev->csr + 0x04);
    //
    // platform_set_drvdata(pdev, mdev);
    return 0;
}

static void msgdma_remove(struct platform_device *pdev)
{
    // struct msgdma_dev *mdev = platform_get_drvdata(pdev);
    // if (mdev->buffer_virt) {
    //     dma_free_coherent(&pdev->dev, mdev->buffer_size, 
    //                       mdev->buffer_virt, mdev->buffer_phys);
    // }
    // of_reserved_mem_device_release(&pdev->dev);
    // dev_info(&pdev->dev, "Removed successfully\n");

    struct msgdma_dev *mdev = platform_get_drvdata(pdev);

    dev_info(&pdev->dev, "msgdma_remove starting\n");
    if (!mdev) return;

    // 1. Stop the mSGDMA Dispatcher (Reset it)
    if (mdev->csr) iowrite32(0x02, mdev->csr + 0x04);

    dev_info(&pdev->dev, "HUHU 000 \n");
    // 2. Free the Descriptor memory
    if (mdev->desc_virt) { dma_free_coherent(&pdev->dev, sizeof(struct msgdma_desc), mdev->desc_virt, mdev->desc_phys); }

    // 3. Free the Data Buffer (the 0x38000000 one)
    if (mdev->buffer_virt) { dma_free_coherent(&pdev->dev, mdev->buffer_size, mdev->buffer_virt, mdev->buffer_phys); }

    // 4. Release the reserved memory handle
    dev_info(&pdev->dev, "HUHU 111 \n");
    of_reserved_mem_device_release(&pdev->dev);
    dev_info(&pdev->dev, "HUHU 222 \n");
    
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

