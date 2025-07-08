# PolarFire SoC Gateware Script for AutoUpdate

Using auto update to reprogram your device.

## Prerequisites

Before attempting to use the gateware script, please ensure you have a board with a PolarFire SoC device with production silicon.
Icicle Kits with Engineering Sample silicon (denoted as -es) are not able to write to the flash from the MSS. As such, IAP and auto update are not supported.

## Programming a Device with a Custom Bitstream

Please follow the instructions from the "[Re-programming the FPGA from Linux](https://github.com/polarfire-soc/polarfire-soc-documentation/blob/master/how-to/re-programming-the-fpga-from-linux.md)" guide.

### Buildroot Support

Once you have followed the above guide and are ready to perform autoupdate you can use the
script provided to carry out the autoupdate.
Ensure you have a bitstream with a `.spi` file extension which contains a design version higher
than the design version programmed in the device. For example `foo.spi`.

On your Linux host development computer, copy the bitstream to your board, replacing
<path/to/your/buildroot-external> with the path to your board's root file system.
    ```cp ./<path/to/your/buildroot-external>/board/rootfs-overlay/lib/firmware/```

To apply these changes to your image, make sure to re-build it with:
    ```make```

Then, re-flash your image - to ensure you have the necessary firmware files.

On your board, initiate the reprogramming of the FPGA with your gateware bitstream:
    ```./update-gateware.sh```

Wait for a couple of minutes for the board to reprogram itself.