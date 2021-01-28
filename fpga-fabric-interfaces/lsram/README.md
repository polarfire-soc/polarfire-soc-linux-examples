# Accessing Fabric LSRAM using UIO

## Objective

Using this application, Read's and Write's to memory mapped Fabric LSRAM are performed.

## Description

In the Libero design, Fabric LSRAM component is interfaced to the MSS using FIC_0 and it is accessible to the processors at 0x61000000. For more information about the Libero design, see [ICICLE Kit Reference Design](https://github.com/polarfire-soc/icicle-kit-reference-design).

The Microchip PolarFire SoC Yocto BSP includes the following support to access LSRAM from user space:

- User application to perform LSRAM memory test using uio-dev node (/dev/uio).
- A device tree node (uio-generic) is added with LSRAM memory address and size of 64KB in the device tree file.
- UIO framework is enabled in the Linux configuration file (defconfig).

## Running the User Application

The LSRAM user application (uio-lsram-read-write) is available under `/opt/microchip/fpga-fabric-interfaces/lsram` directory in rootfs.


```
root@icicle-kit-es:~# cd /opt/microchip/fpga-fabric-interfaces/lsram
```
To run the application, follow these steps:
1. Type the ./uio-lsram-read-write command and Press Enter to execute the application.


```
root@icicle-kit-es:/opt/microchip/fpga-fabric-interfaces/lsram# ./uio-lsram-read-write
locating device for uio_lpddr4
located /dev/uio0
opened /dev/uio0 (r,w)

         # Choose one of  the following options:
         Enter 1 to perform memory test on LSRAM
         Enter 2 to Exit  
```

2. Enter 1 to perform memory test on LSRAM.
   After successful completion of memory test on LSRAM, "LSRAM memory test passed successfully" message is displayed on console.


```
root@icicle-kit-es:/opt/microchip/fpga-fabric-interfaces/lsram# ./uio-lsram-read-write
locating device for uio_lpddr4
located /dev/uio0
opened /dev/uio0 (r,w)

         # Choose one of  the following options:
         Enter 1 to perform memory test on LSRAM
         Enter 2 to Exit
1
Writing incremental pattern starting from address 0x61000000
Reading data  starting from address 0x61000000
Comparing data

**** LSRAM memory test passed successfully *****
unmapped /dev/uio0

# Choose one of  the following options:
         Enter 1 to perform memory test on LSRAM
         Enter 2 to Exit
```

3. Enter 2 to exit the application

