# PolarFire SoC Gateware Script for AutoUpdate

Using auto update to reprogram your device.

## Prerequisites

Before attempting to use the gateware script, please ensure you have a board with a PolarFire SoC device with production silicon.
Icicle Kits with Engineering Sample silicon (denoted as -es) are not able to write to the flash from the MSS. As such, IAP and auto update are not supported.

## Programming a Device with a Custom Bitstream

Please follow the instructions from the "[Re-programming the FPGA from Linux](https://github.com/polarfire-soc/polarfire-soc-documentation/blob/master/how-to/re-programming-the-fpga-from-linux.md)" guide.

### Buildroot Support

Once you have followed the above guide and are ready to perform auto update you can use the
script provided to carry out the auto update.
Ensure you have a bitstream with a `.spi` file extension which contains a design version higher
than the design version programmed in the device. For example `foo.spi`.

On your Linux host development computer, copy the bitstream to your board, replacing
`/path/to/your/buildroot-external/` with the path to your board's root file system and
`mpfs_bitstream.spi` with the name of the bitstream produced by Libero.

```sh
cp ./mpfs_bitstream.spi /path/to/your/buildroot-external/board/rootfs-overlay/lib/firmware/
```

To apply these changes to your image, make sure to re-build it with:

```sh
make
```

Then, re-flash your image - to ensure you have the necessary firmware files.

On your board, initiate the reprogramming of the FPGA with your gateware bitstream:

```sh
./update-gateware.sh /lib/firmware/mpfs_bitstream.spi
```

Wait for a couple of minutes for the board to reprogram itself.

## Script Arguments

Due to supporting several different boards and the variety of ways that gateware
files might be loaded onto a board, the script can take several arguments.

The default behaviour, no arguments, expects that there will be both a design
info file and bitstream file located in /lib/firmware with the names
`mpfs_dtbo.spi` and `mpfs_bitstream.spi`, and it will program both. For example:

```sh
./update-gateware.sh
```

A single argument can be provided and if this is a directory, the script expects
to find `mpfs_dtbo.spi` and `mpfs_bitstream.spi` there, and as with the no
argument case, both files will be programmed to the flash. For example:

```sh
./update-gateware.sh /lib/firmware/
```

If the single argument is a path to a file, then that file must be the bitstream
file. This supports use cases where there is no design info file to be programmed.
The name of the file is not mandated to be `mpfs_bitstream.spi` in this case. For
example:

```sh
./update-gateware.sh ./mpfs_bitstream_v5.spi
```

Finally, the script supports passing both bitstream and design info file paths.
As with the previous case, there are no mandatory file names. The first file is
expected to be the bitstream, and the second the design info file. For example:

```sh
./update-gateware.sh ./mpfs_bitstream_v5.spi ./mpfs_dtbo_v5.spi
```
