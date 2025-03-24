# PolarFire SoC JAPLL PI Controller Application

This example demonstrates how to run the japll-pi-controller application to
adjust the frequency of TSU clock on the PolarFire SoC Video Kit as per master clock,
using the PPB offsets, which are part of the incoming PTP packets.

## Pre-requisites

The japll-pi-controller application relies on the linuxptp application v3.1.1.
The linuxptp application should either be part of the downloaded WIC image, or may be
downloaded and compiled directly on the target. To confirm whether linuxptp is installed,
following command can be tried:

```text
ptp4l -v
```

## Configuration

To modify the configuration, Open japll-pi.cfg with following command:

```text
root@mpfs-video-kit:/opt/microchip/japll-pi-controller# vim /opt/microchip/japll-pi-controller/configs/japll-pi.cfg
```

Tune the parameters according to your usecase and Solution requirements:

```text
     k_proportional     0.9                 -> proportional constant
     k_integral         0.9                 -> integral constant
     delta_time         0.125               -> time frame between two packets
     set_point          0                   -> ideal value for tuning ppb/freq
     pi_enable          1                   -> enable PI controller
     japll_wr_enable    1                   -> enable write operation for tuned PPB value to JAPLL registers
     board_ref_clk_freq 125.00              -> External reference clock from which the TSU clock will be derived
     eth_interface      eth1                -> ethernet driver interface for input to ptp4l command
     ptp4l_config       ./configs/gPTP.cfg  -> filename along with path for input to ptp4l command
```

Note: The delta_time parameter is inversely proportional to the incoming packets per second.

## Running the Application

The following steps need to be performed to launch the application:

```text
root@mpfs-video-kit:~# cd /opt/microchip/japll-pi-controller
```

```text
root@mpfs-video-kit:/opt/microchip/japll-pi-controller# ./japll-pi
```

## Terminating the Application

Once launched, the PI Controller application will run indefinitely, unless it is
interrupted using a ctrl+c, in which case, it will perform a graceful exit.

## Re-build

If needed, the application can be rebuilt by issuing `make clean` and then
`make` command from the same directory.

```text
root@mpfs-video-kit:/opt/microchip/japll-pi-controller# make clean && make
```
