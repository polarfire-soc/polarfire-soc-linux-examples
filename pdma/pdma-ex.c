// SPDX-License-Identifier: MIT
/*
 * DMA proxy example for the Microchip PolarFire SoC.
 *
 *  This example demonstrates usage of DMA proxy driver and benchmarks
 *  it's transaction speeds against memcpy.
 *
 * Copyright (c) 2022 Microchip Technology Inc. All rights reserved.
 */

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include "mchp-dma-proxy.h"
#ifdef CHECK_LEAKS
#include <dmalloc.h>
#endif

const char *dma_channel_names[] = { "dma-proxy0",
				    /* add unique channel names here */
				  };
#define BUFF_LEN (256u)
#define FILENAME_LEN (256u)
#define UDMA_DEVNAME_LEN (FILENAME_LEN)

struct mem_pool {
	uint64_t base;
	size_t size;
	char *name;
	int32_t fd;
	uint8_t *ptr;
	size_t allocated;
	struct mem_pool *next;
};

#define NUM_REGIONS (6)

struct region {
	uint64_t base;
	size_t size;
	char name[20];
} regions[NUM_REGIONS] = {
	{   0x80000000,  0x40000000, "DDRC-CACHE1" },
	{   0xC0000000,  0x10000000, "DDRC-NC1" },
	{   0xD0000000,  0x10000000, "DDRC-NC-WCB1" },
	{ 0x1000000000, 0x400000000, "DDRC-CACHE2" },
	{ 0x1400000000, 0x400000000, "DDRC-NC2" },
	{ 0x1800000000, 0x400000000, "DDRC-NC-WCB2" },
};

struct buff {
	uint64_t base;
	size_t size;
	uint8_t *ptr;
	int32_t chan;
};

static struct mem_pool *insert_pool(struct mem_pool *pool, struct mem_pool **head)
{
	struct mem_pool *current;

	if (!*head) {
		*head = pool;
		return *head;
	}

	current = *head;

	while (current->next)
		current = current->next;

	current->next = pool;

	return *head;
}

static void remove_pools(struct mem_pool *head)
{
	struct mem_pool *cur;
	struct mem_pool *prev;

	if (!head)
		return;

	cur = head;

	do {
		prev = cur;
		cur = cur->next;
		free(prev->name);
		free(prev);
		prev = NULL;
	} while (cur);
}

static int32_t get_num_pools(struct mem_pool *pools)
{
	int32_t i = 0;

	while (pools) {
		i++;
		pools = pools->next;
	}

	return i;
}

static int32_t get_pools(char *provider, struct mem_pool **pools)
{
	char root[] = "/sys/class";
	char addr[] = "phys_addr";
	char size[] = "size";
	char poolinfo[FILENAME_LEN];
	DIR *dirp;
	struct dirent *dp;
	FILE *fp;
	struct mem_pool *pool;
	bool found = false;
	int32_t check;

	snprintf(poolinfo, sizeof(poolinfo), "%s/%s/", root, provider);
	dirp = opendir(poolinfo);
	if (!dirp) {
		printf("failed to find %s\n", poolinfo);
		return -1;
	}

	while ((dp = readdir(dirp)) != NULL) {
		if (dp->d_name) {
			if (!strcmp(dp->d_name, ".") ||
			    !strcmp(dp->d_name, "..")) {
				;
			} else {
				check = snprintf(poolinfo, sizeof(poolinfo), "%s/%s/%s/%s",
						 root, provider, dp->d_name, addr);
				(void)check;

				fp = fopen(poolinfo, "r");
				if (!fp)
					continue;

				pool = (struct mem_pool *)malloc(sizeof(struct mem_pool));
				pool->next = NULL;

				fscanf(fp, "%lx", &pool->base);
				fclose(fp);

				check = snprintf(poolinfo, sizeof(poolinfo), "%s/%s/%s/%s",
						 root, provider, dp->d_name, size);

				fp = fopen(poolinfo, "r");
				if (!fp) {
					free(pool);
					continue;
				}

				fscanf(fp, "%lu", &pool->size);

				pool->name = (char *)malloc(FILENAME_LEN);
				strcpy(pool->name, dp->d_name);

				*pools = insert_pool(pool, pools);
				found = true;

				fclose(fp);
			}
		}
	}
	closedir(dirp);

	return found ? 0 : -1;
}

static int32_t map_pools(struct mem_pool *pool)
{
	char udma_devname[UDMA_DEVNAME_LEN];

	do {
		snprintf(udma_devname, sizeof(udma_devname), "%s%s", "/dev/",
			 pool->name);
		printf("- opening %s\n", udma_devname);

		pool->fd = open(udma_devname, O_RDWR);
		if (pool->fd < 0) {
			fprintf(stderr, "cannot open %s: %s\n",
				udma_devname, strerror(errno));
			return -1;
		}
		printf("- opened %s (r,w)\n", udma_devname);


		if (pool->size == 0) {
			fprintf(stderr, "bad memory size for %s\n",
				udma_devname);
			return -1;
		}

		pool->ptr = mmap(NULL, pool->size, PROT_READ | PROT_WRITE,
				 MAP_SHARED, pool->fd, 0);
		if (pool->ptr == MAP_FAILED) {
			fprintf(stderr, "cannot mmap %s: %s\n",	udma_devname, strerror(errno));
			return -1;
		}
		printf("- mapped 0x%08lx bytes for %s\n", pool->size, udma_devname);

		pool->allocated = 0;
		pool  = pool->next;
	} while (pool);

	return 0;
}

static void unmap_pools(struct mem_pool *head)
{
	struct mem_pool *cur;
	struct mem_pool *prev;

	if (!head)
		return;

	cur = head;

	do {
		prev = cur;
		cur = cur->next;
		printf("- unmapping 0x%08lx bytes from %s\n", prev->size, prev->name);
		munmap(prev->ptr, prev->size);
		close(prev->fd);
		prev = NULL;
	} while (cur);
}

static char *pprint_region(uint64_t base, size_t size)
{
	static char buf[BUFF_LEN];
	int32_t i;

	for (i = 0; i < NUM_REGIONS; i++) {
		if (base >= regions[i].base &&
		    ((base + size) < (regions[i].base + regions[i].size))) {
			snprintf(buf, sizeof(buf), "%s", regions[i].name);
			return buf;
		}
	}

	snprintf(buf, sizeof(buf), "Unknown");
	return buf;
}

static char *pprint_sz(double size)
{
	static char buf[BUFF_LEN];
	const char *suffixes[] = { "B", "KB", "MB", "GB", "TB", "PB", "EB" };
	int32_t sfx = 0;
	double count = size;

	while (count >= (double)1024 && sfx < 7) {
		sfx++;
		count /= (double)1024;
	}

	if (count - floor(count) == 0.0)
		snprintf(buf, sizeof(buf), "%lu %s", (uint64_t)count, suffixes[sfx]);
	else
		snprintf(buf, sizeof(buf), "%.3lf %s", count, suffixes[sfx]);

	return buf;
}

static char *pprint_usecs(uint64_t tm_usec)
{
	static char buf[BUFF_LEN];
	uint64_t usecs;
	uint64_t secs;
	uint64_t mins;
	uint64_t hours;

	usecs = tm_usec % 1000000;
	tm_usec /= 1000000;

	secs = tm_usec % 60;
	tm_usec /= 60;

	mins = tm_usec % 60;
	tm_usec /= 60;

	hours = tm_usec;

	if (hours)
		snprintf(buf, sizeof(buf), "%lu hrs, %lu mins, %lu.%06lu secs", hours, mins, secs, usecs);
	else if (mins)
		snprintf(buf, sizeof(buf), "%lu mins, %lu.%06lu secs", mins, secs, usecs);
	else
		snprintf(buf, sizeof(buf), "%lu.%06lu secs", secs, usecs);

	return buf;
}

static char *pprint_rate(size_t bytes, uint64_t usecs)
{
	static char buf[BUFF_LEN];
	double bps = (double)bytes / (double)usecs;

	bps *= (double)1000000;

	snprintf(buf, sizeof(buf), "%s per sec", pprint_sz(bps));

	return buf;
}

static bool check_buffer(void *buf1, void *buf2, size_t size)
{
	int32_t i;
	char *lbuf1 = (char *)buf1;
	char *lbuf2 = (char *)buf2;

	if (!buf1 || !buf2 || !size) {
		fprintf(stderr, "bad buffer description\n");
		return false;
	}

	if (memcmp(buf1, buf2, size)) {
		fprintf(stderr, "error: buffers did not match\n");
		for (i = 0; i < size; i++) {
			if (lbuf1[i] != lbuf2[i]) {
				printf("%05d (%x vs %x)\n", i, lbuf1[i], lbuf2[i]);
				return false;
			}
		}
	} else {
		printf("- buffers matched over %s\n", pprint_sz(size));
	}

	return true;
}

static int64_t subtract_time(struct timeval *end, struct timeval *start)
{
	int64_t usecs;

	usecs = (end->tv_sec - start->tv_sec) * 1000000;

	usecs += end->tv_usec;
	usecs -= start->tv_usec;

	return usecs;
}

static struct buff *pdmacpy(struct buff *destbuf, struct buff *srcbuf, size_t n)
{

	char channel_name[64] = {0};
	int32_t chn_fd;

	snprintf(channel_name, sizeof(channel_name) - sizeof("/dev/"), "/dev/%s", dma_channel_names[srcbuf->chan]);
	chn_fd = open(channel_name, O_RDWR);
	if (chn_fd == -1)
		return NULL;

	struct mpfs_dma_proxy_channel_config channel_config;

	channel_config.src = srcbuf->base;
	channel_config.dst = destbuf->base;
	channel_config.length = n;

	if (ioctl(chn_fd, MPFS_DMA_PROXY_START_XFER, &channel_config) != 0)
		return NULL;

	if (ioctl(chn_fd, MPFS_DMA_PROXY_FINISH_XFER, &proxy_status_e) != 0)
		return NULL;

	if (close(chn_fd) != 0)
		return NULL;

	return destbuf;
}

static void init_buf(uint8_t *buf, size_t buflen)
{
	int32_t i;
	int32_t rnum;

	for (i = 0; i < buflen / 4; i++) {
		rnum = rand();
		buf[0] = (rnum & 0xff);
		buf[1] = (rnum & 0xff00) >> 8;
		buf[2] = (rnum & 0xff0000) >> 16;
		buf[3] = (rnum & 0xff000000) >> 24;
		buf += 4;
		if ((i % 400000) == 0) {
			printf(".");
			fflush(stdout);
		}
	}

	printf("\n");
}

/* trivial allocator */
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

/* trivial de-allocator */
static void free_buf(struct mem_pool *pool, struct buff *buf)
{
	pool->allocated -= buf->size;
}

int32_t main(void)
{
	struct mem_pool *dma_pools =  NULL;
	struct mem_pool *pool;
	char buf_provider[] = "u-dma-buf";
	int32_t ret;
	struct timeval start_time;
	struct timeval end_time;
	size_t xfersz;
	uint64_t usecs;
	struct buff srcbuf;
	struct buff destbuf;
	int32_t i;

	int32_t NUM_DMA_CHNLS = 1;

	printf("locating contiguous buffer using %s\n", buf_provider);
	ret = get_pools(buf_provider, &dma_pools);
	if (ret < 0) {
		fprintf(stderr, "can't locate buffer for %s\n", buf_provider);
		return -1;
	}
	printf("- located %d contiguous buffers\n", get_num_pools(dma_pools));

	pool = dma_pools;
	printf("\n%-20s\tBase Address\t%-18s\t%-10s\tRegion\n", "Device Name", "Size", "Size");
	while (pool) {
		printf("%-20s\t0x%08lx\t0x%08lx bytes\t%-10s\t%s\n",
		       pool->name, pool->base, pool->size,
		       pprint_sz(pool->size),
		       pprint_region(pool->base, pool->size));
		pool = pool->next;
	}

	printf("\nmapping contigous buffer using mmap()\n");
	ret = map_pools(dma_pools);
	if (ret < 0) {
		remove_pools(dma_pools);
		fprintf(stderr, "can't map buffers\n");
		return -1;
	}
	printf("- mapped all buffers\n");

	pool = dma_pools;
	while (pool) {

		if (!alloc_buf(pool, pool->size >> 1, &destbuf))
			exit(-1);

		if (!alloc_buf(pool, pool->size >> 1, &srcbuf))
			exit(-1);

		printf("\nPreparing buffers from %s\n", pool->name);
		printf("- Setting destination buffer (%s) to 0\n",
		       pprint_sz(destbuf.size));
		memset(destbuf.ptr, 0x0, destbuf.size);
		printf("- Initialising source buffer (%s) with PRBS\n",
		       pprint_sz(srcbuf.size));
		init_buf(srcbuf.ptr, srcbuf.size);

		xfersz = srcbuf.size > destbuf.size ? destbuf.size : srcbuf.size;

		/* test 1 */
		printf("\ntest 1.0 - %s\n", pool->name);
		fflush(stdout);
		gettimeofday(&start_time, NULL);
		memcpy(destbuf.ptr, srcbuf.ptr, xfersz);
		gettimeofday(&end_time, NULL);
		usecs = subtract_time(&end_time, &start_time);
		printf("- moved %s to 0x%08lx using memcpy() in",
		       pprint_sz(xfersz),
		       srcbuf.base);
		printf(" %s", pprint_usecs(usecs));
		printf(" (%s)\n", pprint_rate(xfersz, usecs));
		if (!check_buffer(srcbuf.ptr, destbuf.ptr, xfersz))
			exit(-1);

		/* test 2 */
		for (i = 0; i < NUM_DMA_CHNLS; i++) {

			printf("\ntest 2.%d - %s\n", i, pool->name);
			printf("- Setting destination buffer %s to 0\n",
			       pprint_sz(destbuf.size));
			memset(destbuf.ptr, 0x0, destbuf.size);
			srcbuf.chan = i;
			fflush(stdout);
			gettimeofday(&start_time, NULL);
			if (!pdmacpy(&destbuf, &srcbuf, xfersz)) {
				printf("PDMA ERROR : %s\n", strerror(errno));
				exit(-1);
			}
			gettimeofday(&end_time, NULL);
			usecs = subtract_time(&end_time, &start_time);
			printf("- moved %s from 0x%08lx using pdmacpy (chan: %d) in",
			       pprint_sz(xfersz),
			       srcbuf.base,
			       srcbuf.chan);
			printf(" %s", pprint_usecs(usecs));
			printf(" (%s)\n", pprint_rate(xfersz, usecs));
			if (!check_buffer(srcbuf.ptr, destbuf.ptr, xfersz))
				exit(-1);

		}
		pool = pool->next;
	}

	printf("\nCleaning up\n");
	free_buf(dma_pools, &srcbuf);
	free_buf(dma_pools, &destbuf);

	printf("- unmapping all buffers\n");
	unmap_pools(dma_pools);

	printf("- deallocating buffer descriptors\n");
	remove_pools(dma_pools);

	return 0;
}
