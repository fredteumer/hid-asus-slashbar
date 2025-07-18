# hid-asus-slashbar

Kernel module to control the ROG Zephyrus G14 GA403WR "slashbar" LED via USB HID interface.

- **Vendor ID**: `0x0B05`
- **Product ID**: `0x19B6`
- **Report ID**: `0x5D`

Exposes a `/sys/class/leds/asus::slashbar` LED interface using `led_classdev`.

## Build

```bash
make
sudo insmod hid-asus-slashbar.ko
```

## Development Flow
- Make changes
- Copy files to /usr/src/hid-asus-slashbar-0.1
```bash
sudo dkms remove -m hid-asus-slashbar -v 0.1 --all
sudo dkms add -m hid-asus-slashbar -v 0.1
sudo dkms build -m hid-asus-slashbar -v 0.1
sudo dkms install -m hid-asus-slashbar -v 0.1
```

