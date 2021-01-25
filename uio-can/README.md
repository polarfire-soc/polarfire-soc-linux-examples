# PolarFire SoC UIO CAN Example

uio_can_example is a simple example of using CAN via UIO from Linux user-space.

The example expects to find a CAN device mapped to /dev/uio0.

It initialises the CAN device, places it into loopback mode, puts a single message in a TX buffer and sends it.

It waits for an RX_MSG interrupt and extracts the message from an RX buffer and prints it.

To run the example, type make and then run uio_can_example

