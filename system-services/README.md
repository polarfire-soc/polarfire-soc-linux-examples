# PolarFire SoC System Services Examples

This example application is a demonstration of how to acquire:   
- the board serial number
- the fpga digest
- a constant stream of output from the on-board true random number generator (TRNG)
- an ecdsa signature of a 48 bit hash

Before running the example program, build it by running make:
```
make
```
Once built, it can be run:

```
./system-services-example
```

The following text will appear, showing the demonstrations available:
```
PolarFire SoC system services example program.
Press:
1 - to show the serial number
2 - to show the fpga digest
3 - continuously output from the trng, until ctrl+c
4 - to request a signature
e - to exit this program
```

Enter one of the above characters and press enter to perform the test.