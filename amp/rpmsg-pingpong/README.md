# PolarFire SoC AMP RPMsg PingPong Example

This application can be used to test data integrity of inter-hart communication from linux userspace to a remote
software context.

This application sends chunks of data (payloads) of variable sizes to the remote software context. The remote side
echoes the data back to the application which then validates the data returned.

This application uses the RPMsg char driver to create an endpoint using an ioctl control interface (/dev/rpmsg_ctrlX) and exposes the endpoint to user space by creating a /dev/rpmsgX device.

For more information on RPMsg protocol, please refer to the [PolarFire SoC RPMsg documentation](https://github.com/polarfire-soc/polarfire-soc-documentation/tree/master/asymmetric-multiprocessing/rpmsg.md).

## Pre-requisites:

This application should be run in PolarFire configured in AMP mode. Instructions on how to run a Linux + FreeRTOS AMP configuration using Yocto
or Buildroot can be found in the [PolarFire SoC AMP documentation](https://github.com/polarfire-soc/polarfire-soc-documentation/tree/master/asymmetric-multiprocessing/amp.md) page.

## Building the Application:

Before running the example program, build it by running make:
```
make
```

## Running the Application:

Once built, it can be run:

```
./rpmsg-pingpong
```

By default the application will try to open the virtio0.rpmsg-amp-demo-channel.-1.0 channel.

This channel will then be used to create an endpoint with the following destination and source address:

source address: 0

destination address: 0xFFFFFFFF

Once run, the following information should be displayed on the console:

```
root@icicle-kit-es-amp: ./rpmsg-pingpong
        Open rpmsg dev virtio0.rpmsg-amp-demo-channel.-1.0!
        Opening file rpmsg_ctrl0.
        checking /sys/class/rpmsg/rpmsg_ctrl0/rpmsg0/name
        svc_name: rpmsg-amp-demo-channel

        **************************************

          Echo Test Round 0

        **************************************
        sending payload number 0 of size 17
        echo test: sent : 17
        received payload number 0 of size 17
```
The test should continue until the application sends and receives back the last payload of size 232.

At the end, a test report should be displayed on the console:

```
 **************************************

 Echo Test Round 0 Test Results: Error count = 0

 **************************************
```





