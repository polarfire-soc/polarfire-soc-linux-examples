# PolarFire SoC Configfs Device Tree Overlay Example

Modifying the devicetree using overlays from Linux user-space using configfs

## Prerequisites

This example has been written for the Yocto BSP.

### Kernel

Configuration for device tree overlays and configfs should be enabled in kernel:

```kconfig
CONFIG_OF_OVERLAY=y
CONFIG_OF_CONFIGFS=y
```

### Yocto

The below macro is required, as it will add node symbol info to the device tree blob.
In Yocto, this macro has been added to `conf/machine/xxx.conf`.

```kconfig
KERNEL_DTC_FLAGS += "-@"
```

## Basic Overlay Example

### Creating an Overlay

Create the dt overlay as follows:
```text
root@icicle-kit-es:~# vim rpi-pressure.dtso
```

```devicetree
/dts-v1/;
/plugin/;

&i2c2 {
	#address-cells = <0x1>;
	#size-cells = <0x0>;

	pressure@5c {
		compatible = "st,lps25h-press";
		reg = <0x5c>;
		status = "okay";
	};
};
```

### Compiling and Adding The Overlay

Run the following commands to first compile the overlay and then apply it

```bash
root@icicle-kit-es:~# dtc -@ -O dtb -o rpi-pressure.dtbo rpi-pressure.dtso
root@icicle-kit-es:~# /opt/microchip/dt-overlays/overlay.sh rpi-pressure.dtbo
```

The following kernel config options are needed for the lps25h pressure sensor to work and has been enabled in Yocto:
 `CONFIG_IIO_ST_PRESS=y`.


## RPI Sensehat Overlay:

The overlay for the RPI sensehat is included in the root file system in Yocto, in the `/boot` directory.  
The following commands can be used to load this overlay:

```bash
root@icicle-kit-es:~# cd /boot/
root@icicle-kit-es:/boot# /opt/microchip/dt-overlays/overlay.sh mpfs_icicle_rpi_sense_hat.dtbo
```

You should then see the drivers get loaded for the sensehat and the sensors on it.  
To check how overlay has been applied, we can examine the running kernel's devicetree:

```bash
root@icicle-kit-es:/boot# dtc -I fs /sys/firmware/devicetree/base > icicle.dts
root@icicle-kit-es:/boot# vim icicle.dts
```
