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

#define UIO_LSRAM_DEVNAME	"fpga_lpddr4"
#define UIO_DMA_DEVNAME		"fpga_dma0"

#define MMAP_SIZE		0x10000

#define FILENAME_LEN		(256)

#define LSRAM_BASE		(0x61000000)
#define UNCACHED_DDR_BASE	(0xc0000000)


static void get_uio_devname(char *uio, char *devname, char *uioname)
{
    char root[] = "/sys/class/uio/";
    char name[] = "name";
    char bufinfo[FILENAME_LEN];
    FILE *fp;

    snprintf(bufinfo, sizeof(bufinfo), "%s/%s/%s",
         root, uio, name);

    fp = fopen(bufinfo, "r");
    if (!fp) {
        fprintf(stderr, "failed to open %s\n", bufinfo);
        devname = NULL;
        uioname = NULL;
        return;
    }

    fscanf(fp, "%16s", devname);
    strcpy(uioname, uio);
    fclose(fp);
}

static int get_uio_dev(char * id, char * uiodev)
{
    char root[] = "/sys/class/uio/";
    char bufinfo[FILENAME_LEN];
    char devname[FILENAME_LEN];
    char uioname[FILENAME_LEN];
    DIR *dirp;
    struct dirent *dp;
    char *bufname;
    int found = 0;

    snprintf(bufinfo, sizeof(bufinfo), "%s", root);
    dirp = opendir(bufinfo);

    while ((dp = readdir(dirp)) != NULL) {
        if (dp->d_name) {
            if (!strcmp(dp->d_name, ".") ||
                !strcmp(dp->d_name, "..")) {
                ;
            } else {
                bufname = (char *)malloc(strlen(dp->d_name)+1);
                strncpy(bufname, dp->d_name,
                    strlen(dp->d_name)+1);

                get_uio_devname(bufname, devname, uioname);
                if (*devname == 0) {
                    free(bufname);
                    continue;
                }

                if (strcmp(id, devname) == 0) {
                    found = 1;
                    sprintf(uiodev, "/dev/%s", uioname);
                }

                free(bufname);
            }
        }
    }
    closedir(dirp);

    return found;
}

int main(void)
{
    char cmd;	
    uint32_t rval = 0;
    uint32_t cval = 0;
    volatile uint32_t i =0;
    int32_t intInfo;
    ssize_t readSize;
    uint32_t pending = 0;
    int uioFd_0;
    int uioFd_1;
    int uioFd_2;
    volatile uint32_t *ddr_mem, *dma_mem, *lsram_mem;
    int res;
    char uio_lsram_devname[FILENAME_LEN];
    char uio_dma_devname[FILENAME_LEN];

    // Find the UIO device correpoding to LSRAM
    res = get_uio_dev(UIO_LSRAM_DEVNAME, uio_lsram_devname);
    if (res <= 0) {
        printf("failed to find entry for %s in /sys/class\n",
               UIO_LSRAM_DEVNAME);
        printf("can't locate device for %s\n", UIO_LSRAM_DEVNAME);
        exit(0);
    }

    // Find the UIO device correpoding to DMA
    res = get_uio_dev(UIO_DMA_DEVNAME, uio_dma_devname);
    if (res <= 0) {
        printf("failed to find entry for %s in /sys/class\n",
               UIO_DMA_DEVNAME);
        printf("can't locate device for %s\n", UIO_DMA_DEVNAME);
        exit(0);
    }

    // Open LSRAM device
    uioFd_0 = open(uio_lsram_devname, O_RDWR);
    if (uioFd_0 < 0) {
        fprintf(stderr, "Cannot open %s (%s): %s\n",
            uio_lsram_devname, UIO_LSRAM_DEVNAME, strerror(errno));
        exit(0);
    } else {
        printf("Opened %s (%s)\n", uio_lsram_devname, UIO_LSRAM_DEVNAME );
    }

    /* Map in LSRAM */
    lsram_mem = mmap(NULL, MMAP_SIZE, PROT_READ | PROT_WRITE,
             MAP_SHARED, uioFd_0, 0 * getpagesize());
    if (lsram_mem == MAP_FAILED){
        fprintf(stderr, "Cannot mmap: %s\n", strerror(errno));
        close(uioFd_0);
        exit(0);
    } else {
        printf("mmap for %s (%s) successful\n",
               uio_lsram_devname, UIO_LSRAM_DEVNAME);
    }

    /* Map in uncached DDR */
    uioFd_2 = open("/dev/mem", O_RDWR);

    ddr_mem = mmap(NULL, MMAP_SIZE, PROT_READ | PROT_WRITE,
            MAP_SHARED, uioFd_2, UNCACHED_DDR_BASE);
    if (ddr_mem == MAP_FAILED) {
        fprintf(stderr, "Cannot mmap: %s\n", strerror(errno));
        close(uioFd_2);
        exit(0);
    } else {
        printf("mmap at %x successful\n", UNCACHED_DDR_BASE);
    }

    //  Open the DMA device
    uioFd_1 = open(uio_dma_devname, O_RDWR);
    if (uioFd_1 < 0) {
        fprintf(stderr, "Cannot open %s (%s): %s\n",
            uio_dma_devname,
            UIO_DMA_DEVNAME,
            strerror(errno));
        return -1;
    } else {
        printf("Opened %s (%s)\n",
            uio_dma_devname,
            UIO_DMA_DEVNAME);
    }

    // Map in DMA device memory
    dma_mem = mmap(NULL, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED,
               uioFd_1, 0 * getpagesize());
    if (dma_mem == MAP_FAILED) {
        fprintf(stderr, "Cannot mmap %s (%s) : %s\n",
            uio_dma_devname,
            UIO_DMA_DEVNAME,
            strerror(errno));
        close(uioFd_1);
        return -1;
    } else {
        printf("mmap for %s (%s) successful\n",
            uio_dma_devname,
            UIO_DMA_DEVNAME);
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

            for (i = 0; i < (MMAP_SIZE / 4); i++) {
                *(lsram_mem + i) = i;
                rval = *(lsram_mem + i);
                 if(rval != i) {
                    printf("\n\n\r ***** LSRAM memory test Failed***** \n\r");
                    break;
                }
                 else if (i % 600 == 0) {
                    printf(".");
                }
            }

            if (i == (MMAP_SIZE / 4))
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
            printf("DMAC Version: %x\n", *(dma_mem));   //version control register

            *(dma_mem + (0x14 / 4))  = 0x00000001;  //Interrupt mask

            *(dma_mem + (0x68 / 4))  = LSRAM_BASE;  // Source address
            printf("\n\r\tSource Address (LSRAM) - %x \n\r", LSRAM_BASE);

            *(dma_mem + (0x6C / 4))  = UNCACHED_DDR_BASE;  //destination address
            printf("\n\r\tDestination Address (LPDDR4) - %x \n\r", UNCACHED_DDR_BASE);

            *(dma_mem + (0x64 / 4))  = 0x00010000;  //byte count
            printf("\n\r\tByte count - 0x10000 \n\r");

            *(dma_mem + (0x60 / 4))  = 0x0000F005;  //data ready,descriptor valid,sop,dop,chain

            *(dma_mem + (0x04 / 4))  = 0x00000001;  //start operation register
            printf("\n\r\tDMA Transfer Initiated... \n\r");

            readSize = read(uioFd_1, &pending, sizeof(intInfo));
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
        system("cat /proc/interrupts");
        printf("\n\n");
    } else {
        printf("Enter either 1, 2, and 3\n");
    }

    }	
    munmap((void*)lsram_mem, MMAP_SIZE);
    munmap((void*)ddr_mem, MMAP_SIZE);
    munmap((void*)dma_mem, 0x1000);
    close(uioFd_0);
    close(uioFd_1);
    return 0;
}

