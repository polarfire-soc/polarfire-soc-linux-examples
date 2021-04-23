# PolarFire SoC System Services Examples

## system-services-example
This example application is a basic demonstration of how to acquire:   
- the FPGA device serial number
- the FPGA device digests
- a constant stream of output from the on-board true random number generator (TRNG)
- an ECDSA signature of a 48 bit hash

Before running the example program, build it by running make:
```
make all
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

## signature-verification-demo
This example demonstrates how to use two services, the Digital Signature service and the Device Certificate service to sign a message and verify that signature.     
The Digital Signature service takes a user supplied 48 byte SHA384 hash and signs it with the devices private EC key. The second operational mode is used, resulting in a P-384 ECDSA signature in DER format.     
The Device Certificate services returns the Device Supply Chain Assurance certificate, useful in this case as the certificate contains the public EC key.

Before running the demonstration program, build it by running make:
```
make all
```
Once built, it can be run:

```
./signature-verification-demo
```

The demonstration runs without interaction from the user, first getting the SHA384 hash of an arbitrary string using the openssl library.

This 48 byte hash is then sent to the system controller via the mailbox, by writing to the signature service's device file, and the device file is then read back to acquire signature in 104 byte DER format.

openssl functions are used once again to convert this raw DER signature into a usable object.

To verify the signature, the public key is now retrieved from the device certificate. Once the cert has been read in raw form from the certificate device file, openssl functions are again used to parse the cert to find the public key and store it as a usable object.

Finally the signature can be verified against the hash of the original message using the public key.