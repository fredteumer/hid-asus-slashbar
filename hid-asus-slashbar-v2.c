// ASUS Slashbar LED Companion Driver (Platform-based)
// Uses hidraw interface to coexist with hid-asus driver

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/minmax.h>
#include <linux/hid.h>
#include <linux/usb.h>

#define SLASHBAR_DEVICE_NAME "asus-slashbar"
#define ASUS_SLASHBAR_REPORT_ID 0x5D
#define ASUS_VENDOR_ID 0x0B05
#define ASUS_PRODUCT_ID 0x19B6

struct slashbar_data {
    struct led_classdev led;
    struct mutex lock;
    struct hid_device *hdev;
};

static struct platform_device *slashbar_pdev;

static struct hid_device *slashbar_find_hid_device(void)
{
    struct hid_device *hdev;
    struct usb_device *usb_dev;
    
    // Find HID device with our vendor/product ID
    list_for_each_entry(hdev, &hid_bus_list, bus_list) {
        if (hdev->vendor == ASUS_VENDOR_ID && hdev->product == ASUS_PRODUCT_ID) {
            // Check if it's a USB HID device
            if (hdev->bus == BUS_USB) {
                usb_dev = hid_to_usb_dev(hdev);
                if (usb_dev) {
                    pr_info("asus-slashbar: Found ASUS device %04x:%04x\n", 
                           hdev->vendor, hdev->product);
                    return hdev;
                }
            }
        }
    }
    return NULL;
}

static int slashbar_hid_write(struct slashbar_data *data, const u8 *buf, size_t len)
{
    if (!data->hdev) {
        pr_err("asus-slashbar: No HID device available\n");
        return -ENODEV;
    }

    return hid_hw_output_report(data->hdev, (u8*)buf, len);
}

static int slashbar_send_packet(struct slashbar_data *data, const u8 *payload, size_t payload_len)
{
    u8 buf[64] = {0};
    
    buf[0] = ASUS_SLASHBAR_REPORT_ID;
    if (payload_len > 0) {
        memcpy(&buf[1], payload, min(payload_len, sizeof(buf) - 1));
    }

    return slashbar_hid_write(data, buf, sizeof(buf));
}

static int slashbar_initialize(struct slashbar_data *data)
{
    int ret;
    
    // Wake-up sequence
    const u8 wakeup1[] = "ASUS Tech.Inc.";
    ret = slashbar_send_packet(data, wakeup1, strlen(wakeup1));
    if (ret < 0) return ret;
    
    msleep(10);
    
    const u8 wakeup2[] = {0xC2};
    ret = slashbar_send_packet(data, wakeup2, sizeof(wakeup2));
    if (ret < 0) return ret;
    
    msleep(10);
    
    const u8 wakeup3[] = {0xD1, 0x01, 0x00, 0x01};
    ret = slashbar_send_packet(data, wakeup3, sizeof(wakeup3));
    if (ret < 0) return ret;
    
    msleep(10);
    
    // Initialization sequence
    const u8 init1[] = {0xD7, 0x00, 0x00, 0x01, 0xAC};
    ret = slashbar_send_packet(data, init1, sizeof(init1));
    if (ret < 0) return ret;
    
    msleep(10);
    
    const u8 init2[] = {0xD2, 0x02, 0x01, 0x08, 0xAB};
    ret = slashbar_send_packet(data, init2, sizeof(init2));
    if (ret < 0) return ret;
    
    msleep(10);
    
    pr_info("asus-slashbar: Device initialized\n");
    return 0;
}

static int slashbar_send_command(struct slashbar_data *data, bool enable)
{
    const u8 cmd[] = {0xD8, 0x02, 0x00, 0x01, enable ? 0x00 : 0x80};
    return slashbar_send_packet(data, cmd, sizeof(cmd));
}

static void slashbar_led_set(struct led_classdev *led_cdev, enum led_brightness brightness)
{
    struct slashbar_data *data = container_of(led_cdev, struct slashbar_data, led);
    bool enable = brightness > 0;

    mutex_lock(&data->lock);
    
    if (slashbar_send_command(data, enable) < 0) {
        pr_err("asus-slashbar: Failed to set LED state\n");
    }
    
    mutex_unlock(&data->lock);
}

static int slashbar_probe(struct platform_device *pdev)
{
    struct slashbar_data *data;
    int ret;

    pr_info("asus-slashbar: Probing slashbar companion driver\n");

    data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    mutex_init(&data->lock);

    // Find the HID device
    data->hdev = slashbar_find_hid_device();
    if (!data->hdev) {
        dev_err(&pdev->dev, "ASUS slashbar HID device not found\n");
        return -ENODEV;
    }

    data->led.name = "asus::slashbar";
    data->led.max_brightness = 1;
    data->led.brightness_set = slashbar_led_set;

    ret = devm_led_classdev_register(&pdev->dev, &data->led);
    if (ret) {
        dev_err(&pdev->dev, "Failed to register LED class device: %d\n", ret);
        return ret;
    }

    platform_set_drvdata(pdev, data);

    // Initialize the slashbar device
    ret = slashbar_initialize(data);
    if (ret) {
        dev_warn(&pdev->dev, "Failed to initialize slashbar: %d\n", ret);
        // Don't fail probe, device might still work
    }

    dev_info(&pdev->dev, "ASUS slashbar companion driver loaded\n");
    
    return 0;
}

static void slashbar_remove(struct platform_device *pdev)
{
    struct slashbar_data *data = platform_get_drvdata(pdev);
    
    // Turn off LED on removal
    slashbar_send_command(data, false);
    
    dev_info(&pdev->dev, "ASUS slashbar companion driver removed\n");
}

static struct platform_driver slashbar_driver = {
    .driver = {
        .name = SLASHBAR_DEVICE_NAME,
    },
    .probe = slashbar_probe,
    .remove = slashbar_remove,
};

static int __init slashbar_init(void)
{
    int ret;

    pr_info("asus-slashbar: Initializing ASUS slashbar companion driver\n");

    ret = platform_driver_register(&slashbar_driver);
    if (ret) {
        pr_err("asus-slashbar: Failed to register platform driver: %d\n", ret);
        return ret;
    }

    slashbar_pdev = platform_device_register_simple(SLASHBAR_DEVICE_NAME, -1, NULL, 0);
    if (IS_ERR(slashbar_pdev)) {
        pr_err("asus-slashbar: Failed to register platform device\n");
        platform_driver_unregister(&slashbar_driver);
        return PTR_ERR(slashbar_pdev);
    }

    return 0;
}

static void __exit slashbar_exit(void)
{
    platform_device_unregister(slashbar_pdev);
    platform_driver_unregister(&slashbar_driver);
    pr_info("asus-slashbar: Companion driver unloaded\n");
}

module_init(slashbar_init);
module_exit(slashbar_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ft@nostromo");
MODULE_DESCRIPTION("ASUS Zephyrus G14 Slashbar LED Companion Driver");
MODULE_ALIAS("platform:" SLASHBAR_DEVICE_NAME);