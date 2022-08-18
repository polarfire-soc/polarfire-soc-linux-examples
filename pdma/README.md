# PolarFire SoC PDMA example

## Transferring Large Blocks of Data between Linux User Space and FPGA Fabric on PolarFire SoC

There is often a need to transfer large blocks of data between Linux user space
memory and the fabric.

This application demonstrates one way of doing this on PolarFire SoC where the
designer can control the memory allocation. In addition, using the PDMA hardware
the data transfers gain a performance advantage as well.

The design pattern used in this example does the following:

* Allocate blocks of physical memory that are outside Linux's general memory
  allocation strategy.
  * There may be specific requirements, particularly around
    the areas of
    * Allocating from cached memory, non-cached memory, or non-cached memory
      via the write-combine buffer
    * Allocating physically contiguous blocks of memory to simplify design on
      the fabric side

* Use a kernel driver to manage these blocks of memory
* In user space, map those blocks of memory as pools and allocate and free
     buffers from those pools  using strategies, for example, static allocation or
     dynamic allocation analogous to `malloc()/free()`
* Transfer to/from these buffers using `pdmacpy` and benchmark against
     `memcpy()`

## Allocating and freeing pools in user space

[udmabuf](https://github.com/ikwzm/udmabuf) is a Linux device driver that
allocates contiguous memory blocks in the kernel space as DMA buffers and makes
them available from the user space.It is intended that these memory blocks are
used as DMA buffers when a user application implements device driver in user
space using UIO (User space I/O).

This example uses `u-dma-buf` so a user space application can access three
large contiguous buffers as pools.  One pool is located in a cached DDR region,
one pool is located in a non-cached DDR region, and one pool is located in a
non-cached DDR region which uses a write-combine buffer.

## Devices in User Space

This section describes the pieces of information about the pools presented
by `u-dma-buf` in the user space filesystem. This example will use this
information to access the pools.

This example expects to see three devices in `/dev/`, namely:

```sh
crw------- 1 root root 248,  2 Aug  1 02:24 /dev/udmabuf-ddrc-nc-wcb0
crw------- 1 root root 248,  1 Aug  1 02:24 /dev/udmabuf-ddrc-nc0
crw------- 1 root root 248,  0 Aug  1 02:24 /dev/udmabuf-ddrc0
```

This example uses these devices in a user space application by opening each
device, and using `mmap()` to map the physical address of the base of the
memory region associated with each device, along with the size of the memory
region associated with each device into user space.

`u-dma-buf` provides the information needed by `mmap()` in the Linux
file-system via `sysfs`.

### sysfs

`sysfs` is  pseudo file system provided by the Linux kernel that exports
information about various kernel subsystems, hardware devices, and
associated device drivers from the kernel's device model to user space
through virtual files.

The Linux kernel documentation contains more detailed information about [sysfs](https://www.kernel.org/doc/Documentation/filesystems/sysfs.txt).

`u-dma-buf` locates information under this directory.

```sh
/sys/class/u-dma-buf/
```

That directory contains 3 directories, one corresponding to each device.

```sh
lrwxrwxrwx  1 root root 0 Aug  1 02:24 udmabuf-ddrc-nc-wcb0
lrwxrwxrwx  1 root root 0 Aug  1 02:24 udmabuf-ddrc-nc0
lrzxrwxrwx  1 root root 0 Aug  1 02:24 udmabuf-ddrc0
```

Each directory contains the files listed here.

```sh
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

There are a number of other files of interest in this directory, mainly related
to adjusting the synchronisation rules. A guide to use these files for achieving
synchronization is available on [GitHub](https://github.com/ikwzm/udmabuf).

To get `phys_addr`, `cat` the relevant file, in a similar manner to this.

```sh
# cat /sys/class/u-dma-buf/udmabuf-ddrc0/phys_addr
0x0000000080000000
```

And, to see a `size` value for a particular device, `cat` the relevant file,
in a similar manner to this.

```sh
# cat /sys/class/u-dma-buf/udmabuf-ddrc0/size
33554432
```

Note, the `phys_addr` field is in hexadecimal and `size` is in decimal.

## User space C Code

This example highlights the key C code fragments.  Please refer to
`/opt/microchip/pdma` for a thorough walk-through of all the source
code in the example.

### Getting names, base addresses, and sizes of pools

This routine `get_pools()` gets the `phys_addr` and `size` of the three
pools used in this example.

The following listing highlights some key aspects:

```c
    while ((dp = readdir(dirp)) != NULL) {
            ....
            check = snprintf(poolinfo, sizeof(poolinfo), "%s/%s/%s/%s",
            ...
                pool = (struct mem_pool *)malloc(sizeof(struct mem_pool));
                pool->next = NULL;
                ...
                check = snprintf(poolinfo, sizeof(poolinfo), "%s/%s/%s/%s",
                                 root, provider, dp->d_name, size);
                fp = fopen(poolinfo, "r");
                ...
                pool->name = (char *)malloc(FILENAME_LEN);
                strcpy(pool->name, dp->d_name);
                *pools = insert_pool(pool, pools);
                ...
            }
        }
    }
    closedir(dirp);
```

### Opening and mapping pools into user space

After locating each pool's physical address and size, this example uses
`open()` to open the device associated with each pool. The example retains
the file descriptor for that pool and then the example uses that file
descriptor and `mmap()` to map the memory for each pool into user space.

Refer to `map_pools` function in `pdma-ex.c` for details, but the key code
pieces are highlighted here.

```c
    char udma_devname[UDMA_DEVNAME_LEN];

    do {
        /* Open the device associated with a pool */
        snprintf(udma_devname, sizeof(udma_devname), "%s%s",\
                "/dev/",pool->name);
        ...
        if (pool->fd < 0) {
            ...
        }
            ...
        /* Map in the memory associated with that device */
        pool->ptr = mmap(NULL, pool->size, PROT_READ | PROT_WRITE,\
                        MAP_SHARED, pool->fd, 0);
        if (pool->ptr == MAP_FAILED) {
                ...
        }
            ...
    } while (pool);
```

### Allocating and freeing buffers from pools

Any number of increasing complex allocation/free strategies can
be built to manage these pools.

This example uses a trivial strategy :

* allocate a number of bytes by adjusting one single allocation ptr.
* free a number of bytes by reducing to the same single allocation ptr.

This example highlights the physical address and virtual address obtained
from `mmap()` for each pool and how these addresses might be manipulated
by dividing each pool into a number of buffers.

```c
struct buff {
    uint64_t base;
    size_t size;
    uint8_t *ptr;
    int32_t chan;
};

/* example trivial allocator */
static bool alloc_buf(struct mem_pool *pool, size_t size, struct buff *buf)
{
    if (size + pool->allocated > pool->size) {
        printf("failed to allocate buffer: 0x%lx available, 0x%lx requested\n",
               pool->size - pool->allocated,  size);
        return false;
    }

    buf->base = pool->base + pool->allocated;
    buf->ptr = pool->ptr + pool->allocated;
    buf->size = size;
    buf->chan = -1; /* don't care */
    pool->allocated += size;

    return true;
}

/* example trivial de-allocator */
static void free_buf(struct mem_pool *pool, struct buff *buf)
{
    pool->allocated -= buf->size;
}

```

### Unmapping and closing pools

When finished with pools, the example uses `munmap()` to unmap the memory
associated with the pool from user space and uses the `close()` call to
close the file descriptor associated with the device controlling that memory.

Again, refer to `pdma-ex.c` for details, but the use of `munmap()` and `close()`
are shown here.

```c
static void unmap_pools(struct mem_pool *head)
{
    ...
    do {
        prev = cur;
        cur = cur->next;
        printf("- unmapping 0x%08lx bytes from %s\n", prev->size, prev->name);
        munmap(prev->ptr, prev->size);
        close(prev->fd);
        prev = NULL;
    } while (cur);
    ...
}
```

### Using `memcpy()` to copy buffers

In user space, a developer can copy, move, and set up these buffers using
`memcpy()`, `memset()`, etc. on the
virtual addresses of these buffers.

```c
    xfersz = srcbuf.size > destbuf.size ? destbuf.size : srcbuf.size;
    memcpy(destbuf.ptr, srcbuf.ptr, xfersz);
```

### Using PDMA proxy driver to copy buffers

Another way to interact with these buffers is to use their physical addresses
in conjunction with the PolarFire SoC Platform DMA engine.

Generally, this will be significantly faster than using `memcpy()`. For buffers
of size less than approximately 4 KB, the static setuptime for PDMA may
outweigh the more efficient 'per-byte' transfer time. For larger buffer size
the PDMA is usually faster than `memcpy()` by approximately 5 times.

#### PDMA in the userspace

One way to interact with the PDMA from a User application is to expose
the device using the PDMA proxy driver giving proper system call
interfaces like `open`, `ioctl`, `close`.

#### PDMA Hardware

The SiFive Platform DMA (PDMA) engine has memory mapped control registers
accessed over a slave interface to allow software to set up DMA transfers.
It also has a bus master port into the bus fabric to allow it to autonously
transfer data between slave devices and main memory or to rapidly copy data
between two locations in memory. The PDMA unit can support 4 independent
simultaneous DMA transfers using different PDMA channels and can generate
PLIC interrupts.

More details about the SiFive PDMA used on PolarFire SoC, including its
register map are available in the DMA engine section 4.1.9 of the
[PolarFire SoC MSS Technical Reference Manual](https://www.microsemi.com/document-portal/doc_view/1245725-polarfire-soc-fpga-mss-technical-reference-manual#unique_11).

#### `pdmacpy()` Example

PDMA has 4 independent DMA channels, which operate concurrently to support
multiple simultaneous transfers. Depending on the Linux device tree
configuration, the available channels to userspace may vary. For example SEV
kit has 2 channels available to userspace and 2 reserved for kernel. Whereas
Icicle kit has all 4 channels available to userspace. The dma channels are
available as character devices in Linux with the base name `dma-proxy`.
For example dma-proxy0, dma-proxy1, etc. Using the driver is similar to any
other device in Linux i.e., open, configure through ioctl and close. Relevant
lines of the `pdmacpy`function is explained in the below code snippet.

```c
/* opens the device. Here the channel_name is  dma-proxy0 */
chn_fd = open(channel_name, O_RDWR);

struct mpfs_dma_proxy_channel_config channel_config_t;
channel_config_t.src = srcbuf->base; /* physical address of the source */
channel_config_t.dst = destbuf->base; /* physical address of the destination */
channel_config_t.length = n; /* transfer size in bytes */

/* configuring for the data transfer */
ioctl(chn_fd, MPFS_DMA_PROXY_START_XFER, &channel_config_t);
/* wait till transfer finishes */
ioctl(chn_fd, MPFS_DMA_PROXY_FINISH_XFER, &proxy_status_t);

/* closes the device */
close(chn_fd);
```

Appropriate `errno`s are set for any errors that occur in the above system
calls and the client code can take appropriate action. In the current example,
if an error occurs it is printed and the application is exited.

## Running the Application

The `pdma-ex` application is present under the path `/opt/microchip/pdma` in
the rootfs along with the source code and Makefile. It can be run by typing
`./pdma-ex` in the terminal from its directory as shown below.

```sh
root@sev-kit-es:~# cd /opt/microchip/pdma
root@sev-kit-es:/opt/microchip/pdma# ./pdma-ex
```

When run successfully, it prints details of the test cases like, the data
transfer size, speeds and the memory regions used in transactions for
memcpy vs. pdmacpy. The summary of the results is shown in the Results
section.

If needed it can be rebuilt by issuing `make clean` and then `make` command
from the same directory.

## Results

The following table summarizes the transfer speeds of PDMA and `memcpy()`
for three types of memory on PolarFire SoC i.e., cached, non-cached, and
non-cached via write-combine buffer.

|Memory Type|Transfer Size|`memcpy()` speed| PDMA speed  |Speed gain in PDMA|
|-----------|-------------|----------------|-------------|------------------|
| DDRC      |     16 MB   |  89.96 MBps    | 428.48 MBps |        5x        |
| DDR-NC    |     32 MB   |  16.99 MBps    | 358.69 MBps |        21x       |
| DDR-NC-WCB|     32 MB   |  29.42 MBps    | 358.74 MBps |        12x       |

