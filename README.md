# hid-asus-slashbar

> [!WARNING] This repo is deprecated. Please use [`asusctl`](https://gitlab.com/asus-linux/asusctl) and be sure to support the developers!

Kernel module to control the ROG Zephyrus G14 GA403WR "slashbar" LED via USB HID interface.

- **Vendor ID**: `0x0B05`
- **Product ID**: `0x19B6`
- **Report ID**: `0x5D`

Exposes a `/sys/class/leds/asus::slashbar` LED interface using `led_classdev`.

## Setup

After cloning this repository, create a symbolic link so that editing files here automatically updates the DKMS source:

```bash
sudo rm -rf /usr/src/hid-asus-slashbar-0.1  # Remove existing directory if present
sudo ln -sf $(pwd) /usr/src/hid-asus-slashbar-0.1
```

## Build

```bash
make
sudo insmod hid-asus-slashbar.ko
```

## Development Flow
With the symbolic link in place, you can now make changes to files in this repository and they will automatically be reflected in /usr/src/hid-asus-slashbar-0.1:

```bash
sudo dkms remove -m hid-asus-slashbar -v 0.1 --all
sudo dkms add -m hid-asus-slashbar -v 0.1
sudo dkms build -m hid-asus-slashbar -v 0.1
sudo dkms install -m hid-asus-slashbar -v 0.1
```

## Helpful Links
https://github.com/clsv/rog-ga403-slashctl/blob/master/slashctl.py
https://github.com/NeroReflex/hid-msi-claw-dkms
