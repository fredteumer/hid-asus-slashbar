// drivers/hid/hid-asus-slashbar.c

#include <linux/module.h>
#include <linux/hid.h>
#include <linux/leds.h>
#include <linux/slab.h>

#define ASUS_SLASHBAR_REPORT_ID 0x5D
#define ASUS_VENDOR_ID 0x0B05
#define ASUS_PRODUCT_ID 0x19B6

struct slashbar_data {
    struct hid_device *hdev;
    struct led_classdev led;
    struct mutex lock;
};

static int slashbar_send(struct slashbar_data *data, const u8 *payload, size_t len)
{
    u8 buf[64] = {0};
    buf[0] = ASUS_SLASHBAR_REPORT_ID;
    memcpy(&buf[1], payload, len);

    return hid_hw_output_report(data->hdev, buf, sizeof(buf));
}

static int slashbar_set(struct slashbar_data *data, bool enable)
{
    u8 packet[5] = {0xD8, 0x02, 0x00, 0x01, enable ? 0x00 : 0x80};
    return slashbar_send(data, packet, sizeof(packet));
}

static void slashbar_led_set(struct led_classdev *led_cdev,
                              enum led_brightness brightness)
{
    struct slashbar_data *data = container_of(led_cdev, struct slashbar_data, led);

    mutex_lock(&data->lock);

    if (brightness > 0)
        slashbar_set(data, true);
    else
        slashbar_set(data, false);

    mutex_unlock(&data->lock);
}

static int slashbar_probe(struct hid_device *hdev, const struct hid_device_id *id)
{
    struct slashbar_data *data;
    int ret;

    ret = hid_parse(hdev);
    if (ret) {
        hid_err(hdev, "hid_parse failed\n");
        return ret;
    }

    // Check for report ID 0x5D (slashbar) before starting hardware
    if (!hid_validate_values(hdev, HID_OUTPUT_REPORT, ASUS_SLASHBAR_REPORT_ID, 0, 0)) {
        dev_info(&hdev->dev, "ASUS slashbar report not found, skipping\n");
        return -ENODEV;
    }

    ret = hid_hw_start(hdev, HID_CONNECT_DEFAULT);
    if (ret) {
        hid_err(hdev, "hid_hw_start failed\n");
        return ret;
    }

    data = devm_kzalloc(&hdev->dev, sizeof(*data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    data->hdev = hdev;
    mutex_init(&data->lock);

    data->led.name = "asus::slashbar";
    data->led.max_brightness = 1;
    data->led.brightness_set = slashbar_led_set;

    ret = devm_led_classdev_register(&hdev->dev, &data->led);
    if (ret) {
        hid_err(hdev, "led_classdev_register failed\n");
        return ret;
    }

    hid_set_drvdata(hdev, data);

    dev_info(&hdev->dev, "ASUS slashbar LED driver loaded\n");

    return 0;
}

static void slashbar_remove(struct hid_device *hdev)
{
    struct slashbar_data *data = hid_get_drvdata(hdev);
    slashbar_set(data, false); // turn off
    hid_hw_stop(hdev);
}

static const struct hid_device_id slashbar_devices[] = {
    { HID_USB_DEVICE(ASUS_VENDOR_ID, ASUS_PRODUCT_ID) },
    { }
};
MODULE_DEVICE_TABLE(hid, slashbar_devices);

static struct hid_driver slashbar_driver = {
    .name = "hid-asus-slashbar",
    .id_table = slashbar_devices,
    .probe = slashbar_probe,
    .remove = slashbar_remove,
};

module_hid_driver(slashbar_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ft@nostromo");
MODULE_DESCRIPTION("ASUS Zephyrus G14 Slashbar LED HID Driver");

