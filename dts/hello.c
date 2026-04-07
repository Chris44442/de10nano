#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h> 
#include <linux/dma-mapping.h>
#include <linux/io.h>

#include <linux/of_reserved_mem.h>

#define DRIVER_NAME "msgdma_platform_test"

// The mSGDMA descriptor structure (Standard/Extended)
struct msgdma_desc {
    u32 read_addr;
    u32 write_addr;
    u32 length;
    u32 control;
}; // Note: Ensure this matches your QSys "Descriptor Configuration" (Standard vs Extended)

struct msgdma_dev {
    void __iomem *csr;
    void __iomem *prefetcher;
    void *buffer_virt;
    dma_addr_t buffer_phys;
    size_t buffer_size;
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

    // 4. Basic Reset
    iowrite32(0x02, mdev->csr + 0x04);

    platform_set_drvdata(pdev, mdev);
    return 0;
}

static void msgdma_remove(struct platform_device *pdev)
{
    struct msgdma_dev *mdev = platform_get_drvdata(pdev);
    if (mdev->buffer_virt) {
        dma_free_coherent(&pdev->dev, mdev->buffer_size, 
                          mdev->buffer_virt, mdev->buffer_phys);
    }
    of_reserved_mem_device_release(&pdev->dev);
    dev_info(&pdev->dev, "Removed successfully\n");
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

