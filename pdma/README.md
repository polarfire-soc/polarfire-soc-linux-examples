# Transferring Large Blocks of Data between Linux User Space and FPGA Fabric on PolarFire SoC

One task that often needs to be done on devices with FPGA fabrics is transferring large blocks of memory between user space Linux and the fabric.

This is one way to do that where the designer can control memory allocation.

The design pattern used in this example is:

  * allocate blocks of physical memory that are outside Linux's general memory allocation strategy. There may be specific requirements, particularly around the areas of:

    * allocating from cached memory, non-cached memory, or non-cached memory via the write-combine buffer
    * allocating physically contiguous blocks of memory (to simplify design on fabric side)

  * use a kernel driver to manage these blocks of memory
  * in user space, map those blocks of memory as pools and allocate and free buffers from those pools using strategies, for example, static allocation or dynamic allocation analagous to `malloc()/free()`
  * transfer to/from these buffers using `memcpy()`
  * for performance reasons, the PDMA hardware is useful. Extend the pattern to:
    * map in the PDMA hardware into user space
    * drive the PDMA hardware using a port of the Microchip MSS Bare Metal PDMA driver
    * use this PDMA hardware to manipulate the buffers

## DDR on Icicle-Kit
There is 2 GB of DDR on Icicle-Kit. PolarFire SoC has extensive capabilities for mapping DDR into either cached, non-cached, and/or non-cached write-combine buffer based interfaces (or memory regions). This example uses the default memory region allocation for the 2 GB of physical memory on Icicle-Kit by PolarFire SoC.

### DDR on PolarFire SoC
On PolarFire SoC, ignoring any board constraints, there are six interfaces (or memory regions) that are relevant to this example.

The six regions on PolarFire SoC are summarised here.

|Region Name   | Region Base  | Region Size | Region Properties				       |
|--------------|--------------|-------------|------------------                                |
|DDRC-LO       | 0x80000000   |   1 GB	    | 32-bit address, cached                           |
|DDR-NC-LO     | 0xC0000000   | 256 MB	    | 32-bit address, non-cached		       |
|DDR-NC-WCB-LO | 0xD0000000   | 256 MB	    | 32-bit address, non-cached, write-combine buffer |
|DDRC-HI       | 0x1000000000 |  16 GB	    | 64-bit address, cached			       |
|DDR-NC-HI     | 0x1400000000 |  16 GB	    | 64-bit address, non-cached		       |
|DDR-NC-WCB-HI | 0x1800000000 |  16 GB	    | 64-bit address, non-cached, write-combine buffer |

### DDR on PolarFire SoC on Icicle-Kit
By default, PolarFire SoC's Icicle-Kit boards come with 2 GB of DDR on-board, and with 1.75 GB (1 GB + 768 MB) of that 2 GB of DDR allocated to cached memory and 256 MB of DDR allocated to non-cached memory (with physical memory behind the non-cacheable memory regions addressable at two addresses, one address via the write combine buffer and one address without the write-combine buffer.

So, by default, on Icicle-Kit, the system memory map defaults to:

|Region Name   | Region Base  | Region Size | Region Properties				       |
|--------------|--------------|-------------|------------------                                |
|DDRC-LO       | 0x80000000   | 768 MB	    | 32-bit address, cached                           |
|DDR-NC-LO     | 0xC0000000   | 256 MB [1]  | 32-bit address, non-cached		       |
|DDR-NC-WCB-LO | 0xD0000000   | 256 MB [1]  | 32-bit address, non-cached, write-combine buffer |
|DDRC-HI       | 0x1000000000 |   1 GB	    | 64-bit address, cached			       |

[1] NB: This 256MB is a shared view of a single physical memory area.

By default, the Linux device tree further partitions the memory map by:

a) allocating 32 MB in the 0x80000000 memory space for contiguous cached buffer allocation;
b) allocating 128 MB in the 0xc0000000 for contiguous non-cached buffer allocation;
c) allocating 128 MB in the 0xd0000000 for continguous non-cached buffer (wcb) allocation;
d) allocating all other memory into the Linux general memory allocation pool.

Just using the device-tree, a designer can adjust the memory allocations within either a cached or a non-cached category.  For example, a designer can allocate more memory to non-cached wcb area, provided they reduce the memory allocated to the regular non-cached area correspondingly.

Or a developer may allocate more memory to a contiguous cached buffer provided they reduce the memory allocated to Linux for general purposes correspondingly.

Much more significant memory map changes can be made. Please refer to the Libero documentation for details.

### Default DDR Allocation on Icicle-Kit
The default device tree (dts) file for PolarFire SoC on Icicle-Kit uses two mappings as shown here; the first mapping associates 736 MB of DDR memory, cached, at the LO interface at 0x80000000 and the second mapping associates another 1 GB or DDR memory, cached, at the HI address.

```
DDRC-LO: memory@80000000 {
	device_type = "memory";
	reg = "0x0 0x80000000 0x0 0x2e000000>;
	...
};
DDRC-HI: memory@1040000000 {
	device_type = "memory";
	reg = "0x10 0x40000000 0x0 0x40000000>;
	...
};
```

This leaves 288 MB of memory for other purposes.


### Using 'Reserved-Memory' to create pools

By default with Icicle-Kit, the Linux device tree creates 3 buffers using the 288 MB of memory reserved in the previous steps:
 
  * one in the cached DDR address space
  * one in the non-cached DDR address space
  * one in the non-cached write-combined DDR address space 

These buffers are all allocated in the 'LO' space, that is the 32-bit addressable memory space, as:

  * 32 MB buffer, cache-coherent;
  * 128 MB buffer, non-cache-coherent;
  * 128 MB buffer, non-cache-coherent, write combine buffered


There are 3 `reserved-memory` stanzas in the device tree (dts) file; one for each buffer.

```
reserved-memory {
	ranges;
	#size-cells = <2>;
	#address-cells = <2>;
	fabricbuf0: fabricbuf@0 {
		compatible = "shared-dma-pool";
		reg =  <0x0 0xae000000 0x0 0x02000000>; /* Top 32 MB @ 80000000 */
		label = "fabricbuf-ddrc";
	};
	fabricbuf1: fabricbuf@1 {
		compatible = "shared-dma-pool";
		reg = <0x0 0xc0000000 0x0 0x08000000>; /* Bottom 128 MB @ c0000000 (shared with d0000000) */
		label = "fabricbuf-ddr-nc";
	};
	fabricbuf2: fabricbuf@2 {
		compatible = "shared-dma-pool";
		reg = <0x0 0xd8000000 0x0 0x08000000>; /* Top 128 MB @ d0000000 (shared with c0000000) */
		label = "fabricbuf-ddr-nc-wcb";
	};
};
```

Note, to be picked up by the Linux subsystem, the `reserved-memory` stanza must be at the top-level of the device tree (dts) file.

If adjusting this example:

1. the addresses and size of each `fabricbuf` may need to be adjusted
2. the size of the u-dma-buf that serves one or more `shared-dma-pool` out to user space may need to be adjusted
3. the size of memory allocated to Linux for general memory allocation may need to be adjusted
4. the partitioning of the physical DDR between the six memory regions may need to be reviewed (specifically 
   the values of the SEG0 and SEG1 registers)

As Linux boots, it reports these three DMA memory pools.
```
[..] Reserved memory: created DMA memory pool at 0x00000000ae000000, size 32 MiB
[..] OF: reserved mem: initialized node fabricbuf@0, compatible id shared-dma-pool
[..] Reserved memory: created DMA memory pool at 0x00000000c0000000, size 128 MiB
[..] OF: reserved mem: initialized node fabricbuf@1, compatible id shared-dma-pool
[..] Reserved memory: created DMA memory pool at 0x00000000d8000000, size 128 MiB
[..] OF: reserved mem: initialized node fabricbuf@2, compatible id shared-dma-pool
```

Linux also notes the Zone ranges it will use for general memory allocation.
```
[..] Zone ranges:
[..]   DMA32    [mem 0x0000000080200000-0x00000000ffffffff]
[..]   Normal   [mem 0x0000000100000000-0x000000103fffffff]
```
with the `Normal` region ending just below `0x1040000000`. Linux reserves the space from 0x80000000 to 0x80200000 for its own code and data.

### Accessing `reserved-memory` from user space

Here's one technique for accessing `reserved-memory` from user space.

This example uses `u-dma-buf` by Ichiro Kawazome.

`u-dma-buf` is a Linux device driver that allocates contiguous memory blocks in the kernel space as DMA buffers and makes them available from the user space.  It is intended that these memory blocks are used as DMA buffers when a user application implements device driver in user space using UIO (User space I/O).

Full details and code are available on [github](https://github.com/ikwzm/udmabuf).

This example runs on variants of PolarFire SoC's yocto and buildroot distributions which include `u-dma-buf` in the built kernel.

This example configures three contiguous buffers so `u-dma-buf` can present each of the three fabric buffers to user space.

This example refers to these contiguous buffers in user space as 'pools'.

This example enables:

  * access to one pool from 'cached' memory from user space
  * access to one pool from 'non-cached' memory from user space, and
  * access to one pool which operates via the non-cached write-combine buffer from user space.

This are the relevant device tree stanzas, one for each buffer. Each buffer has a size and a reference to a memory region, and each buffer now has a device name. This example uses these device names to locate the buffers in user space.

```
	udmabuf@0 {
		compatible = "ikwzm,u-dma-buf";
		device-name = "udmabuf-ddr-c0";
		minor-number = <0>;
		size = <0x0 0x02000000>; /* 32 MB */
		memory-region = <&fabricbuf0>;
		sync-mode = <3>;
	};
	udmabuf@1 {
		compatible = "ikwzm,u-dma-buf";
		device-name = "udmabuf-ddr-nc0";
		minor-number = <1>;
		size = <0x0 0x08000000>; /* 128 MB */
		memory-region = <&fabricbuf1>;
		sync-mode = <3>;
		
	};
	udmabuf@2 {
		compatible = "ikwzm,u-dma-buf";
		device-name = "udmabuf-ddr-nc-wcb0";
		minor-number = <2>;
		size = <0x0 0x08000000>; /* 128 MB */
		memory-region = <&fabricbuf2>;
		sync-mode = <3>;

	};
```

If a designer changes the size of a fabricbuf, they probably want to adjust the size of the fabricbuf here as well.

As Linux boots on PolarFire SoC, the Linux boot console will contain details of the three `u-dma-dev` buffers created using the stanzas above.

```
[    0.810707] u-dma-buf soc:udmabuf@0: assigned reserved memory node fabricbuf@0
[    1.408414] u-dma-buf udmabuf-ddrc0: driver version = 3.2.4
[    1.414011] u-dma-buf udmabuf-ddrc0: major number   = 248
[    1.419497] u-dma-buf udmabuf-ddrc0: minor number   = 0
[    1.424731] u-dma-buf udmabuf-ddrc0: phys address   = 0x00000000ae000000
[    1.431490] u-dma-buf udmabuf-ddrc0: buffer size    = 33554432
[    1.437475] u-dma-buf soc:udmabuf@0: driver installed.
```

```
[    1.444401] u-dma-buf soc:udmabuf@1: assigned reserved memory node fabricbuf@1
[    1.830654] u-dma-buf udmabuf-ddrc-nc0: driver version = 3.2.4
[    1.836553] u-dma-buf udmabuf-ddrc-nc0: major number   = 248
[    1.842225] u-dma-buf udmabuf-ddrc-nc0: minor number   = 1
[    1.847777] u-dma-buf udmabuf-ddrc-nc0: phys address   = 0x00000000c0000000
[    1.854747] u-dma-buf udmabuf-ddrc-nc0: buffer size    = 134217728
[    1.860896] u-dma-buf soc:udmabuf@1: driver installed.
```

```
[    1.867652] u-dma-buf soc:udmabuf@2: assigned reserved memory node fabricbuf@2
[    1.907039] u-dma-buf udmabuf-ddrc-nc-wcb0: driver version = 3.2.4
[    1.913237] u-dma-buf udmabuf-ddrc-nc-wcb0: major number   = 248
[    1.919344] u-dma-buf udmabuf-ddrc-nc-wcb0: minor number   = 2
[    1.925231] u-dma-buf udmabuf-ddrc-nc-wcb0: phys address   = 0x00000000d8000000
[    1.932553] u-dma-buf udmabuf-ddrc-nc-wcb0: buffer size    = 13417728
[    1.939054] u-dma-buf soc:udmabuf@2: driver installed.

```

## User Space
Now, the example switches focus to user space and describes interacting with the pools created by the kernel from user space.

The source files for this example can be found at `/opt/microchip/pdma`


### Allocating and freeing pools in user space
This example uses `u-dma-buf` so a user space application can access three large contiguous buffers as pools.  One pool is located in a cached DDR region, one pool is located in a non-cached DDR region, and one pool is located in a non-cached DDR region which uses a write-combine buffer.

### Devices in User Space

This section describes the pieces of information about the pools presented by `u-dma-buf` in the user space filesystem.  This example will use these pieces of information to access the pools.

This example expects to see three devices in `/dev/`, namely:
```
crw------- 1 root root 248,  2 Aug  1 02:24 /dev/udmabuf-ddrc-nc-wcb0
crw------- 1 root root 248,  1 Aug  1 02:24 /dev/udmabuf-ddrc-nc0
crw------- 1 root root 248,  0 Aug  1 02:24 /dev/udmabuf-ddrc0
```

This example uses these devices in a user space application by opening each device, and using `mmap()` to map the physical address of the base of the memory region associated with each device, along with the size of the memory region associated with each device into user space.

`u-dma-buf` provides the information needed by `mmap()` in the Linux file-system via `sysfs`.

#### sysfs
`sysfs` is  pseudo file system provided by the Linux kernel that exports information about various kernel subsystems, hardware devices, and associated device drivers from the kernel's device model to user space through virtual files.

The Linux kernel documentation contains more detailed information about [sysfs](https://www.kernel.org/doc/Documentation/filesystems/sysfs.txt).

`u-dma-buf` locates information under this directory.

```
/sys/class/u-dma-buf/
```

That directory contains 3 directories, one corresponding to each device.
```
lrwxrwxrwx  1 root root 0 Aug  1 02:24 udmabuf-ddrc-nc-wcb0
lrwxrwxrwx  1 root root 0 Aug  1 02:24 udmabuf-ddrc-nc0
lrzxrwxrwx  1 root root 0 Aug  1 02:24 udmabuf-ddrc0
```

Each directory contains the files listed here.  

```
# ls -la /sys/class/u-dma-buf/udmabuf-ddrc0/
-r--r--r-- 1 root root 4096 Aug  1 02:25 phys_addr
-r--r--r-- 1 root root 4096 Aug  1 02:25 size
-rw-rw-r-- 1 root root 4096 Aug  1 02:25 sync_direction
-rw-rw-r-- 1 root root 4096 Aug  1 02:25 sync_for_cpu
-rw-rw-r-- 1 root root 4096 Aug  1 02:25 sync_for_device
-rw-rw-r-- 1 root root 4096 Aug  1 02:25 sync_mode
-rw-rw-r-- 1 root root 4096 Aug  1 02:25 sync_offset
-r--r--r-- 1 root root 4096 Aug  1 02:25 sync_owner
-rw-rw-r-- 1 root root 4096 Aug  1 02:25 sync_size
```

This example focusses on the `phys_addr` and `size` files.

There are a number of other files of interest in this directory, mainly related to adjusting the synchronisation rules. There is a guide to using these files for achieving synchronisation on [github](https://github.com/ikwzm/udmabuf) if extending this basic example.

To see a `phys_addr` value, `cat` the relevant file, in a similar manner to this.


```
# cat /sys/class/u-dma-buf/udmabuf-ddrc0/phys_addr
0x00000000ae000000
```

And, to see a `size` value for a particular device, `cat` the relevant file, in a
similar manner to this.


```
# cat /sys/class/u-dma-buf/udmabuf-ddrc0/size
33554432
```

Note, the `phys_addr` field is in hexadecimal and `size` is in decimal.

## User space C Code
This example highlights the key C code fragments.  Please refer to `/opt/microchip/pdma` for a thorough walk-through of all the source code in the example.

### Getting names, base addresses, and sizes of pools
This routine gets the `phys_addr` and `size` of the three pools used in this example.

The full `get_pools()` example function and supporting code is available at `opt\microchip\pdma`, but this listing highlights some key aspects:

```
#include <dirent.h>

/* Create 3 pools */
struct pool_t {
	uint64 phys_addr;		/* Track the physical address, size,
	size_t size;			   name, virtual address, and a
					   file descriptor for each pool     */
	char name[128];
	uint_t *ptr;
	int fd;
	size_t allocated;		/* Just used for crude alloc/free
					   for each pool                     */
} pools[3] = { 
	{ 0, 0, "", NULL, 0, 0 },
	{ 0, 0, "", NULL, 0, 0 },
	{ 0, 0, "", NULL, 0, 0 },
};

char root[] = "/sys/class";
char pool_provider[] = "u-dma-buf";
char phys_addr[] = "phys_addr";
char size[] = "size";
char poolinfo[256];

DIR *dirp;
struct dirent *dp;

FILE *fp;
int index = 0;

/* Open /sys/class/u-dma-buf to search for devices */
snprintf(poolinfo, 256, "%s/%s/%", root, pool_provider);
dirp = opendir(poolinfo);

/* cycle through each entry in directory */
while ((dp = readdir(dirp)) != NULL) {
	...
	if (!strcmp(dp->d_name, ".") ||
	    !strcmp(dp->d_name, ".." )) {
		/* ignore */
		;
	} else {
		strcpy(pool[index].name, dp->d_name);

		/* Found a device; open its 'phys_addr' file */
		snprintf(poolinfo, 256, "%s/%s/%s/%s",
			 root,
			 pool_provider,
			 dp->d_name,
			 addr);
		fp = fopen(poolinfo, "r");
		if (!fp) {
			...
		}

		...

		/* Read the device's phys_addr value */
		fscanf(fp, "%lx", pool[index].phys_addr);
		fclose(fp);

		/* Open the 'size' file associated with the device */
		snprintf(poolinfo, 256, "%s/%s/%s/%s",
			 root,
			 pool_provider,
			 dp->d_name,
			 size);
		fp = fopen(poolinfo, "r");
		if (!fp) {
			...
		}

		...
		/* Read the 'size' value from that file */
		fscanf(fp, "%lu", pool[index].size);
		fclose(fp);

		index++;
	}
	closedir(dirp);
}

```

### Opening and mapping pools into user space
After locating each pool's physical address and size, this example uses `open()` to open the device associated with each pool. The example retains the file descriptor for that pool and then the example uses that file descriptor and `mmap()` to map the memory for each pool into user space.  

Refer to `pdma-ex.c` for details, but the key code pieces are highlighted here.

```
char udma_devname[256];
int i;

for (i = 0; i < 3; i++) {
	snprintf(udma_devname, 256, "%s%s, "/dev/", pool[i].name);
	/* Open the device associated with a pool */
	pool[i].fd = open(udma_devname, O_RDWR);
	if (pool[i].fd < 0) {
		...
	}

	/* Map in the memory associated with that device */
	pool[i].ptr = mmap(NULL, pool[i].sz, PROT_READ | PROT_WRITE,
			   MAP_SHARED, pool[i].fd, 0);
	if (pool[i].ptr == MAP_FAILED) {
		...
	
	}
}

```

### Allocating and freeing buffers from pools

Any number of increasing complex allocation/free strategies can be built to manage these pools.

This example uses an extremely trivial strategy:

* allocate a number of bytes by adjusting one single allocation ptr;
* free a number of bytes by reducing to the same single allocation ptr.

Obviously, this is not usable for production code and would need to be replaced.

This example highlights the physical address and virtual address obtained from `mmap()` for each pool and how these addresses might be manipulated by dividing each pool into a number of buffers.

```
struct buf_t {
	uint64_t phys_addr;
	char * virt_addr;
	size_t sz;
};

/* example trivial allocator */
void alloc(struct pool_t *pool, struct buf_t * buf, size_t sz)
{
	buf->phys_addr = pool->phys_addr + pool->allocated;
	buf->virt_addr = pool->virt_addr + pool->allocated;
	buf->sz = sz;
	buf->allocated += sz;
}

/* example trivial de-allocator */
void free(struct pool_t *pool, struct buf_t * buf)
{
	buf->allocated -= sz;
}

buf_t src;
buf_t dest;

alloc(&pool[0], &src, 1024);
alloc(&pool[0], &dest, 1024);
```

### Unmapping and closing pools
When finished with pools, the example uses `munmap()` to unmap the memory associated with the pool from user space and uses the `close()` call to close the file descriptor associated with the device controlling that memory. 

Again, refer to `pdma-ex.c` for details, but the use of `munmap()` and `close()` are shown here.

```
int i;

for (i = 0; i < 3; i++) {
	munmap(pools[i].ptr, pools[i].sz);
	close(pools[i].fd;
}
```

### Using `memcpy()` to copy buffers
In user space, a developer can copy, move, and set up these buffers using `memcpy()`, `memset()`, etc on the virtual addresses of these buffers.

```
	size_t xfer_sz = min(dest.sz, src.sz);

	memcpy(dest.virt_addr, src.virt_addr, xfer_sz);
```

### Using PDMA to copy buffers
Another way to interact with these buffers is to use their physical addresses in conjunction with the PolarFire SoC Platform DMA engine.

Generally, this will be significantly faster than using `memcpy()`. For buffers with sizes less than approximately 4 KB, the static setup time for PDMA may outweigh the more efficient 'per-byte' transfer time; as the buffer size increases.  the PDMA is usually faster than `memcpy()` - typically transferring approximately 5 times faster.

#### PDMA in the Kernel
One way to interact with the PDMA from a user application is to expose the device using the UIO framework and directly control it from user space.  It's a fairly simple device so that probably makes sense for this example application.

##### UIO PDMA Driver
By default, PolarFire SoC's `yocto` distribution ships with SiFive's PDMA driver bound to the PDMA hardware in the device tree file.

This example switches out that default driver and replaces it with a UIO driver.

#### PDMA Hardware

The SiFive Platform DMA (PDMA) engine has memory mapped control registers accessed over a slave interface to allow software to set up DMA transfers. It also has a bus master port into the bus fabric to allow it to autonously transfer data between slave devices and main memory or to rapidly copy data between two locations in memory. The PDMA unit can support 4 independent simultaneous DMA transfers using different PDMA channels and can generate PLIC interrupts.

Chapter 12 of the SiFive [FU540 Manual](https://static.dev.sifive.com/FU540-C000-v1.0.pdf) contains more details about the SiFive PDMA used on PolarFire SoC, including its register map.

#### Userspace I/O (UIO)

For many types of devices, creating a Linux kernel driver is overkill. All that is really needed is some way to handle an interrupt and provide access to the memory space of the device.  The logic of controlling the device does not necessarily have to be within the kernel.  To address this situation, the user space I/O system (UIO) was developed.

Further details about UIO can be found in the [UIO Howto](https://www.kernel.org/doc/html/v4.14/driver-api/uio-howto.html).

#### PDMA UIO driver
This example uses a UIO driver to expose the hardware registers for the PDMA on PolarFire SoC to user space.

UIO devices only handle one interrupt and the SiFive PDMA generates 8. So, the Microchip PDMA UIO driver creates 8 PDMA UIO devices.

This UIO driver exposes a UIO driver for the PDMA hardware which:

* maps in the register space for the PDMA hardware into user space addressable memory
* creates 8 devices (one for each interrupt source)

There are 4 channels on the PDMA and each channel has 2 possible interrupt sources, making a total of 8 possible interrupt sources.
Each channel has a `done` interrupt and an `error` interrupt associated with it.

The UIO driver converts each of these interrupt source into a device.  As a result, there is a `/dev/pdma` device for each channel (to which `done` interrupts for each channel are routed and a `/dev/pdmaerr` device for each channel (to which
`error` interrupts for each channel are routed.

In user space, the example uses code based on Microchip's MSS Bare Metal driver to drive the PDMA.

##### Compiling UIO Driver

In the platform's `defconfig`, ensure that
```
CONFIG_UIO_MICROCHIP_PDMA=y
```
This enables the Microchip UIO PDMA driver.

##### Configuring DTS File
In the device tree (dts) file, this example changes the PDMA stanza from:
```
pdma: pdma@3000000 {
    compatible = "sifive,fu540-c000-pdma";
    reg = <0x0 0x3000000 0x0 0x8000>;
    interrupt-parent = <&L1>;
    interrupts = <5 6 7 8 9 10 11 12>;
    #dma-cells = <1>;
};
which binds the SiFive PDMA kernel driver to the PDMA hardware

```
to
```
pdma: pdma@3000000 {
    compatible = "microchip,mpfs-pdma-uio";
    reg = <0x0 0x3000000 0x0 0x8000>;
    interrupt-parent = <&L1>;
    interrupts = <5 6 7 8 9 10 11 12>;
    #dma-cells = <1>;
};
```
which binds the Microchip UIO PDMA driver to the PDMA hardware instead.

If the PDMA UIO driver is correctly configured in the device tree (dts) file, then Linux will display messages like the following when booting:
```
[    2.012814] pdma-uio 3000000.dma: Running Probe
[    2.020249] pdma-uio 3000000.dma: Registered 8 devices
```

### User Space PDMA

This section of the example is focussed on the user space side of the UIO PDMA driver.

Similarly to finding the base address and size of the memory blocks managed by the `u-dma-buf` driver, this sample application needs to locate the names, physical addresses and sizes of the PDMA UIO devices and then open each device using `open()`, then using `mmap()` to map the memory for each device into user space memory along with setting up interrupt-handling code for each of the 8 PDMA-related devices.

#### Locating PDMA UIO devices
This is example commands to browse the `/dev/` directory and the `sysfs` area to locate the 8 PDMA UIO devices that the example application uses to manage the PDMA hardware in user space.

To recap, there are 8 devices; 4 channels (and each channel has two devices; one device associated with the `done` interrupt for that channel and one device associated with the `error` interrupt for that channel.

There may be more UIO devices in the `/dev/` directory than the 8 PDMA UIO devices that this example is concerned with; for example in this listing, there are eleven UIO devices.

```
# ls -la /dev/uio*
crw------- 1 root root 246,  0 Aug  1 02:24 /dev/uio0
crw------- 1 root root 246,  1 Aug  1 02:24 /dev/uio1
crw------- 1 root root 246,  2 Aug  1 02:24 /dev/uio2
crw------- 1 root root 246,  3 Aug  1 02:24 /dev/uio3
crw------- 1 root root 246,  4 Aug  1 02:24 /dev/uio4
crw------- 1 root root 246,  5 Aug  1 02:24 /dev/uio5
crw------- 1 root root 246,  6 Aug  1 02:24 /dev/uio6
crw------- 1 root root 246,  7 Aug  1 02:24 /dev/uio7
crw------- 1 root root 246,  8 Aug  1 02:24 /dev/uio8
crw------- 1 root root 246,  9 Aug  1 02:24 /dev/uio9
crw------- 1 root root 246, 10 Aug  1 02:24 /dev/uio10
```

This code fragment illustrates finding which UIO devices are associated with the PDMA hardware. This example makes use of `sysfs` to locate the information it needs.

UIO drivers present in `sysfs` under the directory `/sys/class/uio/`.

This listing shows that each of the eleven devices on the example system have a corresponding entry in the `/sys/class/uio` directory.

```
# ls -la /sys/class/uio/uio*

lrwxrwxrwx 1 root root 0 Aug  1 02:24 /sys/class/uio/uio0 -> .. soc/61000000.lpddr4 ..
lrwxrwxrwx 1 root root 0 Aug  1 02:24 /sys/class/uio/uio1 -> .. soc/2010c000.can ..
lrwxrwxrwx 1 root root 0 Aug  1 02:24 /sys/class/uio/uio2 -> .. soc/3000000.pdma ..
lrwxrwxrwx 1 root root 0 Aug  1 02:24 /sys/class/uio/uio3 -> .. soc/3000000.pdma ..
lrwxrwxrwx 1 root root 0 Aug  1 02:24 /sys/class/uio/uio4 -> .. soc/3000000.pdma ..
lrwxrwxrwx 1 root root 0 Aug  1 02:24 /sys/class/uio/uio5 -> .. soc/3000000.pdma ..
lrwxrwxrwx 1 root root 0 Aug  1 02:24 /sys/class/uio/uio6 -> .. soc/3000000.pdma ..
lrwxrwxrwx 1 root root 0 Aug  1 02:24 /sys/class/uio/uio7 -> .. soc/3000000.pdma ..
lrwxrwxrwx 1 root root 0 Aug  1 02:24 /sys/class/uio/uio8 -> .. soc/3000000.pdma ..
lrwxrwxrwx 1 root root 0 Aug  1 02:24 /sys/class/uio/uio9 -> .. soc/3000000.pdma ..
lrwxrwxrwx 1 root root 0 Aug  1 02:24 /sys/class/uio/uio10 -> .. soc/60020000.dma ..
```

This listing illustrates the contents of the directory of one of these devices.
This example will browse the `name` file and the `maps` subdirectory for each UIO device in the system.

This example lists the files in the `uio0` sub-directory.

```
# ls -la /sys/class/uio/uio0/
-r--r--r-- 1 root root 4096 Aug  1 05:17 dev
lrwxrwxrwx 1 root root    0 Aug  1 05:17 device
-r--r--r-- 1 root root 4096 Aug  1 05:17 event
drwxr-xr-x 4 root root    0 Aug  1 05:17 maps
-r--r--r-- 1 root root 4096 Aug  1 05:17 name
lrwxrwxrwx 1 root root    0 Aug  1 02:24 subsystem
-rw-r--r-- 1 root root 4096 Aug  1 02:24 uevent
-r--r--r-- 1 root root 4096 Aug  1 05:17 version
```

This example browses the UIO device names.
For example, `uio0` is a driver for a `lpddr4` device and
`uio1` is a driver for a `can0` device.
```
# cat /sys/class/uio/uio0/name
uio_lpddr4

# cat /sys/class/uio/uio1/name
mss_can0
```

In this example, `uio2` is one of the names (`pdma0`) that this example is searching for.
```
# cat /sys/class/uio/uio2/name
pdma0
```
The other part of the `sysfs` sub-system that this example is concerned with is the `maps` sub-directory of each of the PDMA UIO devices.

As an example, `uio2` has a `maps` subdirectory containing one map (`map0`).
`map0` contains the following files.

```
# ls /sys/class/uio/uio2/maps/map0/
addr
name
offset
size
```

Again, using `sysfs`, this example uses the `addr` file to locate the base address of the PDMA hardware's register map and the `size` file to locate the size of the PDMA hardware register map from user space, as illustrated here.

```
# cat /sys/class/uio/uio2/maps/map0/addr
0x0000000003000000

# cat /sys/class/uio/uio2/maps/map0/size
0x0000000000008000

# cat /sys/class/uio/uio2/maps/map0/offset
0x0
```

#### Locating PDMA UIO Devices using C Code
The example can find the UIO devices corresponding to the PDMA hardware, find the base address of the PDMA hardware and the memory size required by the PDMA hardware.

Similarly to the u-dma-buf example code above, the example code will open those UIO devices, use `mmap()` to map those base addresses and sizes into user space.

The example will compile with a PDMA driver based on Microchip's MSS Bare Metal PDMA driver. There are details about Microchip's MSS Bare Metal drivers on [github](https://github.com/polarfire-soc/polarfire-soc-bare-metal-library), including the Bare Metal code for the PDMA driver and a sample Bare Metal application for PDMA.

The example shows an sample `pdmacpy()` function that use this PDMA hardware from user space.

Again, refer to `pdma-ex.c` for details, but this lists the code for locating the UIO devices associated with the PDMA hardware, storing the device name, th base address and the memory size for each device.
```
static int get_pdma_devs(struct pdma_t pdma[])
{
        char root[] = "/sys/class/uio/";
        char bufinfo[128];
        char devname[128];
        char uioname[128];

        DIR *dirp;
        struct dirent *dp;
        uint64_t addr;
        size_t sz;
        int index;

	/* Open /sys/class/uio directory and search for files/directories in it */
        snprintf(bufinfo, 128, "%s", root);
        dirp = opendir(bufinfo);

        while ((dp = readdir(dirp)) != NULL) {
            if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..")) {
                ;
            } else {
                /* Open the 'name' file for this class and get it's name */
                get_uio_devname(dp->d_name, devname, uioname);
                /* Is the name pdma* or pdmaerr* - if it is its ours */
                if(is_pdma_dev(devname)) {
                    /* Which /dev/uioI ? */
                    index = get_pdma_dev_index(devname);
		    /* get phys addr from file */
                    addr = get_pdma_dev_addr(uioname);
		    /* get sz from file */
                    sz = get_pdma_dev_size(uioname);
		    /* store these details */
                    add_pdma_dev(pdmas, index, uioname, devname, addr, sz);
            }
        }
        closedir(dirp);

```

#### Opening, mapping, unmapping and closing PDMA UIO devices

This section illustrates the code that the example uses to
open and memory map the PDMA UIO devices.

```
static int map_pdma_devs(struct pdma_t * pdmas, int num)
{
	char pdma_devname[128];
        int i;

        for (i = 0; i < 8; i++) {
                snprintf(pdma_devname, 128, "%s%s", "/dev/",
                         pdmas[i].uioname);

		/* Open the UIO device and retain its file descriptor */
                pdmas[i].fd = open(pdma_devname, O_RDWR);
                if(pdmas[i].fd < 0) {
			...
                }

		/* Use the file descriptor to memory map the register
		   map for the PDMA hardware into user space */
                pdmas[i].ptr = mmap(NULL, pdmas[i].sz, PROT_READ | PROT_WRITE,
                                    MAP_SHARED, pdmas[i].fd, 0);
                if (pdmas[i].ptr == MAP_FAILED) {
			...
                }
        }

        return 0;
```

When the example finishes with the PDMA UIO devices, it uses `munmap()`  to unmap the mapped memory and `close()` to close the UIO device. 

```
static void unmap_pdma_devs(struct pdma_t pdmas[])
{
        int i;

        for (i = 0; i < 8; i++) {
                munmap(pdmas[i].ptr,  pdmas[i].sz);
                close(pdmas[i].fd);
        }
}
```

#### Linking with MSS Driver
This example adapts the Microchip MSS Bare Metal PDMA driver to run in user space on Linux.

The main change to the MSS Bare Metal PDMA Driver is it is adapted to use a variable to hold the virtual address of the base address of the driver instead of statically defining that base address as a physical address.

The adapted driver is supplied in the example directory as `mss-pdma.c` and `mss-pdma.h`.

#### `pdmacpy()` Example
The example `pdmacpy()` routine consists of adapting the Bare Metal example to pick a PDMA channel, set up a physical address for source buffer and destination buffer and a size field and use the Bare Metal driver to manage the transfer.

The example code waits on two interrupts - either the `done` interrupt or the `error` interrupt associated with the channel in use.

#### Timing `memcpy()` and `pdmacpy()`
The example times each copy - whether `memcpy()` or `pdmacpy()`.  It does this by taking a `timeval` before each copy starts and another `timeval` after the copy ends.  The example converts the two `timeval`s to usec values and subtracting the start `timeval` from the end `timeval`. It pretty prints the result of the subtraction as minutes, seconds, and usecs and displays that result.

#### Results

This table summarises the length of time taken by `memcpy()` and for `pdmacpy()`.  The `pdmacpy()` value is averaged across the time taken for each channel to complete the example copy.

These rates are captured for the three types of memory on PolarFire SoC; cached, non-cached, and non-cached via write-combine buffer.

| Memory Type | Transfer Size | `memcpy()` speed | PDMA speed |
| ----------- | ------------- | ------------ | ------------- |
| DDRC        | 32 MB         | 91.67 MB/sec | 445.51 MB/sec |
| DDR-NC      | 128 MB         | 23.35 MB/sec | 359.04 MB/sec |
| DDR-NC-WCB  | 128 MB         | 42.80 MB/sec | 359.08 MB/sec |

### Conclusions
This pattern is useful for developers moving large amounts of data where those developers control the memory allocation scheme.

Using the PDMA from user space is faster than `memcpy()` for transfer sizes above approximately 1 KB. The advantage varies with memory type:

* PDMA is approximately 5 times faster than `memcpy()` in cached DDR transfers;
* PDMA is approximately 15 times faster than `memcpy()` in non-cached DDR transfers;
* PDMA is approximately 8 times faster than `memcpy()` in non-cached DDR transfers through the write combine buffer.

PDMA-based copies from non-cached DDR and non-cached DDR via write-combined-buffers are approximately 19% slower than PDMA-based copies from cached DDR.

