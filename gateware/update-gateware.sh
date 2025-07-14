#!/bin/sh

if [ $# -gt 2 ]; then
    echo "Too many arguments provided."
fi

if [ $# -eq 0 ]; then
    echo "No gateware location provided, checking default location (/lib/firmware/)"
    gateware_location=/lib/firmware/mpfs_bitstream.spi
    dtbo_location=/lib/firmware/mpfs_dtbo.spi

    if [ ! -e $gateware_location ]; then
        echo "No gateware file found."
        exit 1
    fi

    if [ ! -e $dtbo_location ]; then
        echo "No dtbo file found."
        exit 1
    fi
elif [ $# -eq 1 ]; then
    if [ ! -e "$1" ]; then
        echo "Gateware location invalid."
        exit 1
    fi

    if [ -d "$1" ]; then
        gateware_location="$1"/mpfs_dtbo.spi
        dtbo_location="$1"/mpfs_bitstream.spi
    else
        gateware_location=$1
    fi
elif [ $# -eq 2 ]; then
    if [ ! -e "$1" ]; then
        echo "No gateware file found."
        exit 1
    fi
    if [ ! -e "$2" ]; then
        echo "No dtbo file found."
        exit 1
    fi
    gateware_location=$1
    dtbo_location=$2
fi

echo "Using gateware file $gateware_location"

if [ -n "$dtbo_location" ] && [ -e "$dtbo_location" ]; then
    echo "Using dtbo file $dtbo_location"

    # Trash existing device tree overlay in case the rest of the process fails:
    flash_erase /dev/mtd0 0 16

    # Initiate FPGA update for dtbo
    echo 1 > /sys/class/firmware/mpfs-auto-update/loading

    # Write device tree overlay
    cat "$dtbo_location" > /sys/class/firmware/mpfs-auto-update/data

    # Signal completion for dtbo load
    echo 0 > /sys/class/firmware/mpfs-auto-update/loading

    while [ "$(cat /sys/class/firmware/mpfs-auto-update/status)" != "idle" ]; do
        # Do nothing, just keep checking
        sleep 1
    done
fi

# Fake the presence of a golden image for now.
dd if=/dev/zero of=/dev/mtd0 count=1 bs=4 2>/dev/null

# Initiate FPGA update for bitstream
echo 1 > /sys/class/firmware/mpfs-auto-update/loading

# Write the firmware image to the data sysfs file
cat "$gateware_location" > /sys/class/firmware/mpfs-auto-update/data

# Signal completion for bitstream load
echo 0 > /sys/class/firmware/mpfs-auto-update/loading

while [ "$(cat /sys/class/firmware/mpfs-auto-update/status)" != "idle" ]; do
    # Do nothing, just keep checking
    sleep 1
done

# When the status is 'idle' and no error has occured, reboot the system for
# the gateware update to take effect. FPGA reprogramming takes places between
# Linux shut-down and HSS restarting the board.
if [ "$(cat /sys/class/firmware/mpfs-auto-update/error)" = "" ]; then
    echo "FPGA update ready. Rebooting."
    reboot
else
    echo "FPGA update failed with status: $(cat /sys/class/firmware/mpfs-auto-update/error)"
    exit 1
fi
