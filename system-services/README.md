# PolarFire SoC System Services Examples

## Loading the mpfs-generic-service module

These examples rely on the presence of the mpfs-generic-service driver.
This driver exposes all of the features of the System Controller to userspace as
a character device, so that developing applications that use the System
Controller is made easier.
Exposing the entire System Controller is not ideal, as many of the services can
kill the running operating system, or otherwise disrupt normal operation, in a
way that userspace should not be able to on a production system.  
In our default kernel configuration, `mpfs_defconfig`, this driver is enabled as
a module.
To load the module, which is required to run the below applications, run:

```bash
modprobe mpfs-generic-service
```

The following message will appear when the module is loaded:

```text
mpfs-generic-service mpfs-generic-service: Registered MPFS generic service - FOR DEVELOPMENT ONLY, DO NOT USE IN PRODUCTION
```

## system-services-example

This example application is a basic demonstration of how to acquire:

- the FPGA device serial number
- the FPGA device digests
- a constant stream of output from the on-board true random number generator
  (TRNG)
- an ECDSA signature of a 48 bit hash

Before running the example program, build it by running make:

```bash
make all
```

Once built, it can be run:

```bash
./system-services-example
```

The following text will appear, showing the demonstrations available:

```text
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

This example demonstrates how to use two services, the Digital Signature service
and the Device Certificate service to sign a message and verify that signature:

- The Digital Signature service takes a user supplied 48-byte SHA384 hash and
  signs it with the device's private EC key. The second operational mode is
  used, resulting in a P-384 ECDSA signature in DER format.
- The Device Certificate service returns the Device Supply Chain Assurance
  certificate, useful in this case as the certificate contains the public EC
  key.

Before running the demonstration program, build it by running make:

```bash
make all
```

Once built, it can be run:

```bash
./signature-verification-demo
```

The demonstration runs without interaction from the user, executing the
following steps:

1. First, it gets the SHA384 hash of an arbitrary string using the openssl
   library
2. Then this 48-byte hash is sent to the System Controller via the mailbox, by
   writing to the signature service's device file
3. The device file is then read back to acquire the signature in 104-byte DER
   format
4. Once again Openssl functions are used to convert this raw DER signature into
   a usable object
5. To verify the signature, the public key is now retrieved from the device
   certificate
6. The certificate is been read in raw form from the certificate device file
7. Again openssl functions are used to parse the certificate to find the public
   key and store it as a usable object
8. Finally, the signature can be verified against the hash of the original
   message using the public key
