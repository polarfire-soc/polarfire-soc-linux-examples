# PolarFire SoC System Services Examples

This example application contains a proof of concept for acquiring:   
- the board serial number
- the fpga digest
- a constant stream of output from the rng device


Run the example program, first build it by running make:
```
make
```
Once built, it can be run:

```
./system-services-example
```

It will present the following options:

```
PolarFire SoC system services test program.

Press:
1 - to show the serial number
2 - to show the fpga digest
3 - to cat hwrng, until ctrl+c
e - to exit this program
```

Enter one of the above characters and press enter to perform the test.