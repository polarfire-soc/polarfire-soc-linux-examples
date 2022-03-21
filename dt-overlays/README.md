# PolarFire SoC Configfs Device Tree Overlay Example

Configfs overlay script file (overlay.sh) for added device nodes from Linux user-space.

## Prerequisites:
### Kernel:
Configuration for overlay and configfs should be enabled in kernel.
```
CONFIG_OF_OVERLAY=y
CONFIG_OF_CONFIGFS=y
```
### yocto:
Below macro must be added in Yocto build `conf/machine/xxx.conf` file.
it will add node symbol info in device tree blob.
```
KERNEL_DTC_FLAGS += "-@"
```

## Writing and compiling device tree overlay:


### Writing dtso file:
```
root@icicle-kit-es:~# vim  rpi-lps25h.dtso

/dts-v1/;
/plugin/;

&i2c2 {
	lps25h-press@5c {
		compatible = "st,lps25h-press";
		reg = <0x5c>;
		status = "okay";
	};
};
```
### Commands to Compile dtso and add to kernel device tree:

```
root@icicle-kit-es:~# dtc -@ -O dtb -o rpi-lps25h.dtbo rpi-lps25h.dtso
root@icicle-kit-es:~# /opt/microchip/dt-overlays/overlay.sh rpi-lps25h.dtbo
```
kernel config needed for lps25h sensor to work `CONFIG_IIO_ST_PRESS=y`.


## Command to add RPI sense HAT overlay:

```
root@icicle-kit-es:~# cd /boot/
root@icicle-kit-es:/boot# /opt/microchip/dt-overlays/overlay.sh mpfs_icicle_rpi_sense_hat.dtbo

```

## Command to get the updated device tree file from kernal sysfs to cross check:

```
root@icicle-kit-es:/boot# dtc -I fs /sys/firmware/devicetree/base > icicle.dts

root@icicle-kit-es:/boot# vim icicle.dts
```
