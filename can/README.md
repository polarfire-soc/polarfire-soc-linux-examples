# PolarFire SoC SocketCAN Interface

This document describes how to set up a SocketCAN interface using the [iproute2][1]
suite of tools.

## Prerequisites

The socketCAN interface as well as the iproute tools required to follow this guide are
available in PolarFire SoC Linux release 2024.02 and onwards.

## Displaying the SocketCAN interface

The ip tool can be used to display all the available IP/network interfaces. For example, on the
Icicle Kit, the `can0` and `can1` interface should be displayed as shown below:

```shell
root@icicle-kit-es:~# ip link show
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue state UNKNOWN mode DEFAULT group default qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
2: can0: <NOARP,ECHO> mtu 16 qdisc noop state DOWN mode DEFAULT group default qlen 10
    link/can
3: can1: <NOARP,ECHO> mtu 16 qdisc noop state DOWN mode DEFAULT group default qlen 10
    link/can
4: eth0: <NO-CARRIER,BROADCAST,MULTICAST,UP> mtu 1500 qdisc mq state DOWN mode DEFAULT group default qlen 1000
    link/ether 00:04:a3:d2:d9:29 brd ff:ff:ff:ff:ff:ff
5: end0: <NO-CARRIER,BROADCAST,MULTICAST,UP> mtu 1500 qdisc mq state DOWN mode DEFAULT group default qlen 1000
   link/ether 00:04:a3:d2:d9:28 brd ff:ff:ff:ff:ff:ff
6: sit0@NONE: <NOARP> mtu 1480 qdisc noop state DOWN mode DEFAULT group default qlen 1000
   link/sit 0.0.0.0 brd 0.0.0.0
```

## Configuring And Enabling The SocketCAN Interface

### Configuring Loopback mode

It is possible to configure the SocketCAN in internal Loopback test mode. In that case, the
interface treats its own transmitted messages as received messages.

This section describes how to set loopback mode for the `can0` interface in the Icicle Kit. The
same steps would apply for the `can1` interface as well.

1. First make sure that the interface is down before configuring it:

    ```shell
    root@icicle-kit-es:~# ip link set can0 down
    ```

2. Set the bitrate and enable loopback mode. In this example, we will set the bitrate to 1Mbps

    ```shell
    root@icicle-kit-es:~# ip link set can0 type can bitrate 1000000 loopback on
    ```

3. Now that the interface has been configured, bring the interface up:

    ```shell
    root@icicle-kit-es:~# ip link set can0 up
    ```

4. To test the loopback mode, use the candump command to listen/receive for CAN packets:

    ```shell
    root@icicle-kit-es:~# candump can0 &
    ```

5. Finally, use the cansend command to send some data to the can0 interface:

    ```shell
    root@icicle-kit-es:~# cansend can0 001#1122334455667788
    ```

    The above command will send a CAN message on can0 with the identifier 0x001 and the data bytes
    [ 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 ]. Values are always treated as hexadecimal.

    If successful, you should expect the the following output from the candump process running in the
    background:

    ```shell
    $ can0  001   [8]  11 22 33 44 55 66 77 88
    $ can0  001   [8]  11 22 33 44 55 66 77 88
    ```

### Configuring SocketCAN Interface in Normal mode

To configure the socketCAN interface to communicate with an external device/controller connected
to J27 (CAN0) or J25 (CAN1) on the Icicle Kit. For CAN0, the following commands can be used:

1. First make sure that the interface is down before configuring it:

    ```shell
    root@icicle-kit-es:~# ip link set can0 down
    ```

2. Set the bitrate. In this example, we will set the bitrate to 1Mbps:

    ```shell
    root@icicle-kit-es:~# ip link set can0 type can bitrate 1000000
    ```

3. Now that the interface has been configured, bring the interface up:

    ```shell
    root@icicle-kit-es:~# ip link set can0 up
    ```

4. To receive packets, use the candump command.

    ```shell
    root@icicle-kit-es:~# candump can0
    ```

    For sending packets the cansend command can be used instead:

    ```shell
    root@icicle-kit-es:~# cansend can0 001#1122334455667788
    ```

For additional information on how to use the iproute2 command, please use the following command:

```shell
$ ip link set can0 up type can help
```

[1]: https://wiki.linuxfoundation.org/networking/iproute2
