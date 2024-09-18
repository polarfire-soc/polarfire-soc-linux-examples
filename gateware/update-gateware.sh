#!/bin/sh
if [ $# -eq 0 ]; then
    echo "No gateware location provided. Checking default location."
    if [ ! -e /lib/firmware/mpfs_bitstream.spi ]; then
         echo "No gateware file found."
         exit 1
    fi
else
    echo "Gateware location provided: $1"
    if [ ! -e "$1"/mpfs_bitstream.spi ]; then
        echo "No gateware file found."
        exit 1
    else
        if [ ! -d /lib/firmware ]; then
            mkdir /lib/firmware
        fi
        cp "$1"/mpfs_dtbo.spi /lib/firmware/mpfs_dtbo.spi
        cp "$1"/mpfs_bitstream.spi /lib/firmware/mpfs_bitstream.spi
    fi
fi
1
flash_erase /dev/mtd0 0 64
dtbo_size=$(stat -c %s /lib/firmware/mpfs_dtbo.spi)
dd if=/lib/firmware/mpfs_dtbo.spi of=/dev/mtd0 bs=1 seek=$((0x400)) count="$dtbo_size" status=progress
dd if=/dev/zero of=/dev/mtd0 count=1 bs=4
echo 0 > /sys/class/firmware/timeout
echo 1 > /sys/class/firmware/mpfs-auto-update/loading
bitstream_size=$(stat -c %s /lib/firmware/mpfs_bitstream.spi)
dd if=/lib/firmware/mpfs_bitstream.spi of=/sys/class/firmware/mpfs-auto-update/data bs=1 count="$bitstream_size" status=progress
echo 0 > /sys/class/firmware/mpfs-auto-update/loading
while [ "$(cat /sys/class/firmware/mpfs-auto-update/status)" != "idle" ]; do
  sleep 1
done
reboot
