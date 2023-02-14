# Accessing shared LPDDR4 Memory from Fabric and Linux User Application

## Objective

Using this application, a shared uncached LPDDR4 memory space is accessed by  linux user application using UIO framework and fabric. 


## Description

In the Libero design, fabric LSRAM and fabric DMA Controller are interfaced to MSS via FIC. The shared LPDDR4 memory region (0XC8000000-0xC800FFFF) is accessed via  uncached path for fabric. For more information about the Libero design, see [ICICLE Kit Reference Design](https://github.com/polarfire-soc/icicle-kit-reference-design).

The Microchip PolarFire SoC Yocto BSP includes the following support to access the uncached LPDDR4 region from user space.

- User application to perform data transfers from LSRAM to LPDDR4 using uio-dev node (/dev/uio).
- A device tree node (uio-generic) is added for LSRAM and uncached LPDDR4 memory addresses in the device tree file. 
- A device tree node (uio-generic) is added for DMA Controller memory address in the device tree file.
- A driver for Fabric DMA controller is added in the uio framework to handle DMA interrupt.
- UIO framework with DMA support is enabled in the Linux configuration file (defconfig).

The following table lists the addresses and sizes included in device tree nodes based on ICICLE Kit Reference Design.

| Component | Start Addr | Size |
| --- | --- | --- |
| LSRAM | 0x60000000 | 4 K |
| LPDDR4 | 0xC8000000 | 64 K |
| DMA Controller | 0x60010000 | 4 K |

The C application includes the following DMA register configuration to initiate data transfers.

- Source address register (LSRAM)
- Destination address register (LPDDR4)
- Byte count register
- Descriptor0 configuration register
- Interrupt mask register (Configures Interrupt)
- Start operation Resister (starts DMA transfer)

The C application waits for the interrupt assertion and validates the data once the DMA transfer is completed. 

## Running the User Application

The user application (uio-dma-interrupt) is available under /opt/microchip/dma directory in rootfs.

```
root@icicle-kit-es:~# cd /opt/microchip/dma/  
```
To run the application, follow these steps:
1. Type the ./uio-dma-interrupt command and Press Enter to execute the application.

```
root@icicle-kit-es:/opt/microchip/dma# ./uio-dma-interrupt
locating device for dma-controller@60010000
located /dev/uio2 
opened /dev/uio2 (r,w)
mapped 0x1000 bytes for /dev/uio2
locating device for fpga_lsram
located /dev/uio0 
opened /dev/uio0 (r,w)
mapped 0x1000 bytes for /dev/uio0
mmap at c8000000 successful

         # Choose one of  the following options:
         Enter 1 to perform memory test on LSRAM
         Enter 2 to write data from LSRAM to LPDD4 via DMA access
         Enter 3 to Exit  
```

2. Enter 1 to perform memory test on LSRAM.

   After successful completion of memory test on LSRAM, "LSRAM memory test passed with incremental pattern" message is displayed on console.

```
locating device for dma-controller@60020000
located /dev/uio2 
opened /dev/uio2 (r,w)
mapped 0x1000 bytes for /dev/uio2
locating device for fpga_lsram
located /dev/uio0 
opened /dev/uio0 (r,w)
mapped 0x1000 bytes for /dev/uio0
mmap at c8000000 successful

         # Choose one of  the following options:
         Enter 1 to perform memory test on LSRAM
         Enter 2 to transfer data from LSRAM to uncached LPDD4 via DMA
         Enter 3 to Exit
1

Writing incremental pattern starting from address 0x61000000

Reading data  starting from address 0x61000000

Comparing data
..............................................................................


**** LSRAM memory test passed with incremental pattern *****


         # Choose one of  the following options:
         Enter 1 to perform memory test on LSRAM
         Enter 2 to write data from LSRAM to LPDD4 via DMA acces
         Enter 3 to Exit
```

3. Enter 2 to perform data transfer from LSRAM to uncached LPDDR4 memory region using Fabric DMA controller:

```
locating device for dma-controller@60020000
located /dev/uio2 
opened /dev/uio2 (r,w)
mapped 0x1000 bytes for /dev/uio2
locating device for fpga_lsram
located /dev/uio0 
opened /dev/uio0 (r,w)
mapped 0x1000 bytes for /dev/uio0
mmap at c8000000 successful

         # Choose one of  the following options:
         Enter 1 to perform memory test on LSRAM
         Enter 2 to transfer data from LSRAM to uncached LPDD4 via DMA
         Enter 3 to Exit
2

Initialized LSRAM (64KB) with incremental pattern.
Fabric DMA controller configured for LSRAM to LPDDR4 data transfer.
DMAC Version = 0x20064
        Source Address (LSRAM) - 0x60000000
        Destination Address (LPDDR4) - 0xc8000000

        Byte count - 0x10000

DMA Transfer Initiated...

DMA Transfer completed and interrupt generated.

Cleared DMA interrupt.

Comparing LSRAM data with LPDDR4 data....

***** Data Verification Passed *****

Displaying interrupt count by executing "cat /proc/interrupts":

(Show the output for one interrupt)



         # Choose one of  the following options:
         Enter 1 to perform memory test on LSRAM
         Enter 2 to transfer data from LSRAM to uncached LPDD4 via DMA
         Enter 3 to Exit
```

4. If you perform option 2 again, the interrupt count should be incremented.

5. Enter 3 to exit the application.

