# PolarFire SoC System Services Examples

This example application is a demonstration of how to acquire:   
- the FPGA device serial number
- the FPGA device digests
- a constant stream of output from the on-board true random number generator (TRNG)
- an ECDSA signature of a 48 bit hash

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
1 - to show the FPGA device serial number
2 - to show the FPGA device digests
3 - continuously output random numbers from the TRNG, until ctrl+c
4 - to request an ECDSA signature
e - to exit this program
```

Enter one of the above characters and press enter to request the service.