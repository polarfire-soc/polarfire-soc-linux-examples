
# PolarFire SoC AMP RPMsg TTY Example

This application can be used to send messages to another software context using the /dev/ttyRPMSGx device created
by the RPMSG TTY client driver.

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
./rpmsg-tty
```

By default the application will try to open /dev/ttyRPMSG4 device. The number four indicates the destination endpoint configured in the FreeRTOS context running the RPMsg console demo.

Optionally, a different device can be provided as an input to this application:

```
./rpmsg-tty -d /dev/ttyRPMSGx
```

where x is the destination endpoint configured in the application running in the associated software context.

Once run, the following information should be displayed on the console:

```
root@icicle-kit-es-amp: ./rpmsg-tty
        Opening device /dev/ttyRPMSG4
        Device is open
        Enter message to send or type quit to quit :
```

Type a text message on the console and press enter to send to the associated software context.

