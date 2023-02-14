// SPDX-License-Identifier: MIT
/*
 * LPDDR4 DMA example for the Microchip PolarFire SoC
 *
 * Copyright (c) 2021 Microchip Technology Inc. All rights reserved.
 */

#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>

#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <poll.h>
#include <stdlib.h>
#include "fdma.h"

#define UIO_LSRAM_DEVNAME      "fpga_lsram"
#define UIO_DMA_DEVNAME        "dma-controller@60010000"

#define FILENAME_LEN           (256)

#define LSRAM_BASE             0x60000000U
#define UNCACHED_DDR_BASE      0xC8000000U
#define SYSFS_PATH_LEN         (128)
#define ID_STR_LEN             (32)
#define UIO_DEVICE_PATH_LEN    (32)
#define NUM_UIO_DEVICES        (32)

static char uio_id_str_fdma[] = UIO_DMA_DEVNAME;
static char uio_id_str_lsram[] = UIO_LSRAM_DEVNAME;
static char sysfs_template[] = "/sys/class/uio/uio%d/%s";

static uint32_t get_memory_size(char *sysfs_path, char *uio_device)
{
    FILE *fp;
    uint32_t sz;

    /*
     * open the file the describes the memory range size.
     * this is set up by the reg property of the node in the device tree
     */
    fp = fopen(sysfs_path, "r");
    if (fp == NULL) {
        fprintf(stderr, "unable to determine size for %s\n", uio_device);
        exit(0);
    }
    fscanf(fp, "0x%016X", &sz);
    fclose(fp);
    return sz;
}

static int get_uio_device(char * id)
{
    FILE *fp;
    int i;
    size_t len;
    char file_id[ID_STR_LEN];
    char sysfs_path[SYSFS_PATH_LEN];

    for (i = 0; i < NUM_UIO_DEVICES; i++) {
        snprintf(sysfs_path, SYSFS_PATH_LEN, sysfs_template, i, "/name");
        fp = fopen(sysfs_path, "r");
        if (fp == NULL)
            break;
        fscanf(fp, "%32s", file_id);
        len = strlen(id);
        if (len > ID_STR_LEN-1)
            len = ID_STR_LEN-1;
        if (strncmp(file_id, id, len) == 0) {
            return i;
        }
    }
    return -1;
}

int main(void)
{
    char cmd;
    uint32_t rval = 0;
    uint32_t cval = 0;
    volatile uint32_t i =0;
    ssize_t readSize;
    uint32_t pending = 0;
    int uioFd_0;
    int uioFd_1;
    int uioFd_2;
    int ret=0;
    volatile fdma_t * fdma_dev;
    volatile uint32_t *ddr_mem, *lsram_mem;
    char uio_device[UIO_DEVICE_PATH_LEN];
    char sysfs_path[SYSFS_PATH_LEN];
    uint32_t fdma_mmap_size, lsram_mmap_size;
    int index;

    printf("locating device for %s\n", uio_id_str_fdma);
    index = get_uio_device(uio_id_str_fdma);
    if (index < 0) {
        fprintf(stderr, "can't locate uio device for %s\n", uio_id_str_fdma);
        return -1;
    }

    snprintf(uio_device, UIO_DEVICE_PATH_LEN, "/dev/uio%d", index);
    printf("located %s\n", uio_device);

    uioFd_0 = open(uio_device, O_RDWR);
    if(uioFd_0 < 0) {
        fprintf(stderr, "cannot open %s: %s\n", uio_device, strerror(errno));
        return -1;
    } else {
        printf("opened %s (r,w)\n", uio_device);
    }
    snprintf(sysfs_path, SYSFS_PATH_LEN, sysfs_template, index, "maps/map0/size");
    fdma_mmap_size = get_memory_size(sysfs_path, uio_device);
    if (fdma_mmap_size == 0) {
        fprintf(stderr, "bad memory size for %s\n", uio_device);
        return -1;
    }
    fdma_dev = mmap(NULL, fdma_mmap_size, PROT_READ|PROT_WRITE, MAP_SHARED, uioFd_0, 0);
    if (fdma_dev == MAP_FAILED) {
        fprintf(stderr, "cannot mmap %s: %s\n", uio_device, strerror(errno));
        return -1;
    } else {
        printf("mapped 0x%x bytes for %s\n", fdma_mmap_size, uio_device);
    }

    printf("locating device for %s\n", uio_id_str_lsram);
    index = get_uio_device(uio_id_str_lsram);
    if (index < 0) {
        fprintf(stderr, "can't locate uio device for %s\n", uio_id_str_fdma);
        return -1;
    }

    snprintf(uio_device, UIO_DEVICE_PATH_LEN, "/dev/uio%d", index);
    printf("located %s\n", uio_device);

    uioFd_1 = open(uio_device, O_RDWR);
    if(uioFd_1 < 0) {
        fprintf(stderr, "cannot open %s: %s\n", uio_device, strerror(errno));
        return -1;
    } else {
        printf("opened %s (r,w)\n", uio_device);
    }
    snprintf(sysfs_path, SYSFS_PATH_LEN, sysfs_template, index, "maps/map0/size");
    lsram_mmap_size = get_memory_size(sysfs_path, uio_device);
    if (lsram_mmap_size == 0) {
        fprintf(stderr, "bad memory size for %s\n", uio_device);
        return -1;
    }
    lsram_mem = mmap(NULL, lsram_mmap_size, PROT_READ|PROT_WRITE, MAP_SHARED, uioFd_1, 0);
    if (lsram_mem == MAP_FAILED) {
        fprintf(stderr, "cannot mmap %s: %s\n", uio_device, strerror(errno));
        return -1;
    } else {
        printf("mapped 0x%x bytes for %s\n", lsram_mmap_size, uio_device);
    }

    /* Map in uncached DDR */
    uioFd_2 = open("/dev/mem", O_RDWR);

    ddr_mem = mmap(NULL, FDMA_TR_SIZE, PROT_READ | PROT_WRITE,
            MAP_SHARED, uioFd_2, UNCACHED_DDR_BASE);
    if (ddr_mem == MAP_FAILED) {
        fprintf(stderr, "Cannot mmap: %s\n", strerror(errno));
        close(uioFd_2);
        exit(0);
    } else {
        printf("mmap at %x successful\n", UNCACHED_DDR_BASE);
    }

    while(1){
        printf("\n\t # Choose one of  the following options: \n\t Enter 1 to perform memory test on LSRAM \n\t Enter 2 to write data from LSRAM to LPDD4 via DMA access  \n\t Enter 3 to Exit\n");
        scanf("%c%*c",&cmd);
        if ((cmd == '3') || (cmd == 'q')) {
            break;
        } else if (cmd == '1') {
            printf("\nWriting incremental pattern starting from address %x\n", LSRAM_BASE);
            printf("\nReading data starting from address %x \n", LSRAM_BASE);
            printf("\nComparing data \n");

            for (i = 0; i < (lsram_mmap_size / 4); i++) {
                *(lsram_mem + i) = i;
            }
            for (i = 0; i < (lsram_mmap_size / 4); i++) {
                rval = *(lsram_mem + i);
                 if(rval != i) {
                    printf("\n\n\r ***** LSRAM memory test Failed***** \n\r");
                    break;
                }
                 else if (i % 600 == 0) {
                    printf(".");
                }
            }

            if (i == (lsram_mmap_size / 4))
                printf("\n\n\n**** LSRAM memory test Passed with incremental pattern *****\n\n");
        }
        else if (cmd == '2') {
            /* Initialize LSRAM */
            for (i = 0; i < 64; i++) {
                *(lsram_mem + i) = 0x12345678 + i;
                rval = *(lsram_mem + i);
                if (rval == 0x12345678 + i) {
                    printf(".");
                } else {
                    printf("\n\n\r *****LSRAM memory test Failed***** \n\r");
                    break;
                }
            }
            printf("\nInitialized LSRAM (64KB) with incremental pattern.\n");
            printf("\nFabric DMA controller configured for LSRAM to LPDDR4 data transfer.\n");

            printf("DMAC Version = 0x%x \n\r", fdma_dev->version_reg);

            fdma_dev->irq_mask_reg = FDMA_IRQ_MASK;

            /*0x68  Source current address. */
            fdma_dev->exec_source = LSRAM_BASE;
            printf("\n\r\tSource Address (LSRAM) - 0x%x \n\r", LSRAM_BASE);

            /*0x6C  Destination current address. */
            fdma_dev->exec_destination = UNCACHED_DDR_BASE;
            printf("\n\r\tDestination Address (LPDDR4) - %x \n\r", UNCACHED_DDR_BASE);

            fdma_dev->exec_bytes = FDMA_TR_SIZE;
            printf("\n\r\tByte count - 0x%x \n\r", FDMA_TR_SIZE);

            fdma_dev->exec_config = FDMA_CONF_VAL;

            /*0x04  Start Register */
            fdma_dev->start_op_reg = FDMA_START;
            printf("\n\r\tDMA Transfer Initiated... \n\r");

            readSize = read(uioFd_0, &pending, sizeof(pending));
            if(readSize < 0){
                fprintf(stderr, "Cannot wait for uio device interrupt: %s\n",
                strerror(errno));
                break;
            }
               
        printf("\n****DMA Transfer completed and interrupt generated.*****\n");
        printf("\n\tCleared DMA interrupt. \n");
        printf("\n\tComparing LSRAM data with LPDDR4 data... \n");

        for (i = 0; i < 8 ; i++) {
            rval = *(ddr_mem + i);
            cval = *(lsram_mem + i);
            if (rval == cval) {
                  printf(".");
            } else {
                  printf("\nLPDDR4 data verification failed\n");
                  break;
            }
         }

         printf("\n***** Data Verification Passed *****\n");

        printf("\n Displaying interrupt count by executing \"cat /proc/interrupts \":\n");
        ret = system("cat /proc/interrupts");
        if(ret < 0) {
            printf("unable to run system cmd\n");
        }
        printf("\n\n");
    } else {
        printf("Enter either 1, 2, and 3\n");
    }
    }
    ret = munmap((void*)lsram_mem, lsram_mmap_size);
    if(ret < 0) {
        printf("unable to unmap the lsram_mem\n");
    }
    ret = munmap((void*)ddr_mem, FDMA_TR_SIZE);
    if(ret < 0) {
        printf("unable to unmap the ddr_mem\n");
    }
    ret = munmap((void*)fdma_dev, fdma_mmap_size);
    if(ret < 0) {
        printf("unable to unmap the fdma mem\n");
    }
    close(uioFd_0);
    close(uioFd_1);
    close(uioFd_2);
    return 0;
}
