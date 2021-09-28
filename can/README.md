# PolarFire SoC UIO CAN Example

uio-can-example is a simple example of using CAN via UIO from Linux user-space.

The example expects to find a CAN device mapped to /dev/uioX.

It initialises the CAN device, places it into loopback mode, puts a single message in a TX buffer and sends it.

It waits for an RX_MSG interrupt and extracts the message from an RX buffer and prints it.

Run the example program, first build it by running make:
```
make
```
Once built, it can be run:

```
./uio-can-example
```
A successful execution should look like:
```
locating device for uiocan0
located /dev/uio1
opened /dev/uio1 (r,w)
mapped 0x1000 bytes for /dev/uio1
setting can device at /dev/uio1 into loopback mode
sent msg: 'example'
received msg: 'example'
unmapped /dev/uio1
closed /dev/uio1
```
