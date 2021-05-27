/*******************************************************************************
 * Copyright 2021 Microchip FPGA Embedded Systems Solutions.
 *
 * SPDX-License-Identifier: MIT
 *
 * Application code running in Linux user-space
 *
 * PolarFire SoC MSS PDMA Driver example project
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
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include "mss_pdma.h"
#ifdef CHECK_LEAKS 
#include <dmalloc.h>
#endif

#define NUM_PDMA_DEVS	(8)

#define FILENAME_LEN	(256)
#define UDMA_DEVNAMELEN	(FILENAME_LEN)
#define PDMA_DEVNAMELEN (FILENAME_LEN)

struct pool_t {
	uint64_t base;
	uint64_t sz;
	char *name;
	int fd;
	uint8_t *ptr;
	size_t allocated;
	struct pool_t *next;
};

#define NUM_REGIONS (6)

struct region_t {
	uint64_t base;
	size_t sz;
	char name[20];
} regions[NUM_REGIONS] = {
	{   0x80000000,  0x40000000, "DDRC-CACHE1" },
	{   0xC0000000,  0x10000000, "DDRC-NC1" },
	{   0xD0000000,  0x10000000, "DDRC-NC-WCB1" },
	{ 0x1000000000, 0x400000000, "DDRC-CACHE2" },
	{ 0x1400000000, 0x400000000, "DDRC-NC2" },
	{ 0x1800000000, 0x400000000, "DDRC-NC-WCB2" },
};

struct buf_t {
	uint64_t base;
	size_t sz;
	uint8_t *ptr;
	int chan;
};

struct pdma_t {
	uint64_t base;
	uint64_t sz;
	char *uioname;
	char *pdmaname;
	int fd;
	void *ptr;
} pdmas[NUM_PDMA_DEVS];

static struct pool_t * insert_pool(struct pool_t * pool, struct pool_t ** head)
{
	struct pool_t * current;

	if (*head == NULL) {
		*head = pool;
		return *head;
	}

	current = *head;

	while(current->next)
		current = current->next;

	current->next = pool;

	return *head;
}

static void remove_pools(struct pool_t *head)
{
	struct pool_t * cur;
	struct pool_t * prev;

	if (head == NULL)
		return;

	cur = head;

	cur->next = NULL;

	do {
		prev = cur;
		cur = cur->next;
		free(prev->name);
		free(prev);
		prev = NULL;
	} while(cur != NULL);
}

static int get_num_pools(struct pool_t * pools)
{
	int i = 0;

	while(pools) {
		i++;
		pools = pools->next;
	}

	return i;
}

static int get_pools(char * provider, struct pool_t ** pools)
{
	char root[] = "/sys/class";
	char addr[] = "phys_addr";
	char size[] = "size";
	char poolinfo[FILENAME_LEN];
	DIR *dirp;
	struct dirent *dp;
	FILE *fp;
	struct pool_t * pool;
	bool found = false;
	int check;

	snprintf(poolinfo, sizeof(poolinfo), "%s/%s/", root, provider);
	dirp = opendir(poolinfo);
	if(!dirp) {
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

				pool = (struct pool_t *)malloc(sizeof(struct pool_t));
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

				fscanf(fp, "%lu", &pool->sz);

				pool->name = (char *)malloc(FILENAME_LEN);
				strcpy(pool->name, dp->d_name);

				*pools = insert_pool(pool, pools);
				found = true;

				fclose(fp);
			}
		}
	}
	closedir(dirp);

	return found==true?0:-1;

}

static int map_pools(struct pool_t * pool)
{
	char udma_devname[UDMA_DEVNAMELEN];

	do {
		snprintf(udma_devname, sizeof(udma_devname), "%s%s", "/dev/",
			pool->name);
		printf("- opening %s\n", udma_devname);

		pool->fd = open(udma_devname, O_RDWR);
		if(pool->fd < 0) {
			fprintf(stderr, "cannot open %s: %s\n",
				udma_devname, strerror(errno));
			return -1;
		} else {
			printf("- opened %s (r,w)\n", udma_devname);
		}

		if (pool->sz == 0) {
			fprintf(stderr, "bad memory size for %s\n",
				udma_devname);
			return -1;
		}

		pool->ptr = mmap(NULL, pool->sz, PROT_READ | PROT_WRITE,
				MAP_SHARED, pool->fd, 0);
		if (pool->ptr == MAP_FAILED) {
			fprintf(stderr, "cannot mmap %s: %s\n",
				udma_devname, strerror(errno));
			return -1;
		} else {
			printf("- mapped 0x%08lx bytes for %s\n",
			       pool->sz, udma_devname);
		}
		pool->allocated = 0;
		pool  = pool->next;
	} while (pool);

	return 0;
}

static void unmap_pools(struct pool_t *head)
{
	struct pool_t * cur;
	struct pool_t * prev;

	if (head == NULL)
		return;

	cur = head;

	cur->next = NULL;

	do {
		prev = cur;
		cur = cur->next;
		printf("- unmapping 0x%08lx bytes from %s\n", prev->sz,
		       prev->name);
		munmap(prev->ptr, prev->sz);
		close(prev->fd);
		prev = NULL;
	} while(cur != NULL);
}

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

static bool is_pdma_dev(char * buf)
{
	if (strlen(buf) >= 7) {
		if (0 == strncmp("pdmaerr", buf, 7))
			return true;
	}

	if (strlen(buf) >= 4) { 
		if (0 == strncmp("pdma", buf, 4))
			return true;
	}

	return false;
}

static int get_pdma_dev_index(char *buf)
{
	int index = -1;

	if (0 == strncmp("pdmaerr", buf, 7)) {
		buf += 7;
		index = strtol(buf, NULL, 10);
		index = index*2 + 1;
	} else if (0 == strncmp("pdma", buf, 4)) {
		buf += 4;
		index = strtol(buf, NULL, 10);
		index *= 2;
	}

	return index;
}

static int get_pdma_dev_addr(char *buf)
{
	char root[] = "/sys/class/uio";
	char addr[] = "maps/map0/addr";
	char bufinfo[FILENAME_LEN];
	uint64_t add;
	FILE *fp;

	snprintf(bufinfo, sizeof(bufinfo), "%s/%s/%s",
		 root, buf, addr);

	fp = fopen(bufinfo, "r");
	if (!fp) {
		printf("can't open %s\n", bufinfo);
		return 0;
	}

	fscanf(fp, "%lx", &add);

	fclose(fp);

	return add;
}

static int get_pdma_dev_size(char *buf)
{
	char root[] = "/sys/class/uio";
	char size[] = "maps/map0/size";
	char bufinfo[FILENAME_LEN];
	size_t sz;
	FILE *fp;

	snprintf(bufinfo, sizeof(bufinfo), "%s/%s/%s",
		 root, buf, size);

	fp = fopen(bufinfo, "r");
	if (!fp)
		return 0;

	fscanf(fp, "%lx", &sz);

	fclose(fp);

	return sz;
}

static void add_pdma_dev(struct pdma_t *pdmas, int index,
			 char *uioname, char *pdmaname, uint64_t addr,
			 size_t sz)
{
	size_t uionamelen = strlen(uioname);
	size_t pdmanamelen = strlen(pdmaname);

	if ((index < 0) || (index > 7)) {
		fprintf(stderr, "bad index\n");
	}

	pdmas[index].uioname = malloc(uionamelen+1);
	memset(pdmas[index].uioname, 0, uionamelen+1);
	strncpy(pdmas[index].uioname, uioname, uionamelen);
	pdmas[index].pdmaname = malloc(pdmanamelen+1);
	memset(pdmas[index].pdmaname, 0, pdmanamelen+1);
	strncpy(pdmas[index].pdmaname, pdmaname, pdmanamelen);
	pdmas[index].base = addr;
	pdmas[index].sz = sz;
}

static int get_pdma_devs(struct pdma_t pdma[])
{
	char root[] = "/sys/class/uio/";
	char bufinfo[FILENAME_LEN];
	char devname[FILENAME_LEN];
	char uioname[FILENAME_LEN];
	DIR *dirp;
	struct dirent *dp;
	int found = 0;
	uint64_t addr;
	size_t sz;
	int index;

	snprintf(bufinfo, sizeof(bufinfo), "%s", root);
	dirp = opendir(bufinfo);
	if (!dirp) {
		printf("failed to open %s\n", bufinfo);
		return -1;
	}

	while ((dp = readdir(dirp)) != NULL) {
		if (dp->d_name) {
			if (!strcmp(dp->d_name, ".") ||
			    !strcmp(dp->d_name, "..")) {
				;
			} else {
				get_uio_devname(dp->d_name, devname, uioname);
				if (*devname == 0)
					continue;

				if(is_pdma_dev(devname)) {
					index = get_pdma_dev_index(devname);
					addr = get_pdma_dev_addr(uioname);
					sz = get_pdma_dev_size(uioname);
					add_pdma_dev(pdmas, index, uioname,
						     devname, addr, sz);
					found++;
				}
			}
		}
	}
	closedir(dirp);

	return found==NUM_PDMA_DEVS?0:-1;

}

static int map_pdma_devs(struct pdma_t * pdmas, int num)
{
	char pdma_devname[PDMA_DEVNAMELEN];
	int i;

	for (i = 0; i < num; i++) {
		snprintf(pdma_devname, sizeof(pdma_devname), "%s%s", "/dev/",
			 pdmas[i].uioname);
		printf("- opening %s (%s)\n", pdma_devname, pdmas[i].pdmaname);

		pdmas[i].fd = open(pdma_devname, O_RDWR);
		if(pdmas[i].fd < 0) {
			fprintf(stderr, "cannot open %s: %s\n",
				pdma_devname, strerror(errno));
			return -1;
		} else {
			printf("- opened %s (r,w)\n", pdma_devname);
		}

		if (pdmas[i].sz == 0) {
			fprintf(stderr, "bad memory size for %s\n",
				pdma_devname);
			return -1;
		}

		pdmas[i].ptr = mmap(NULL, pdmas[i].sz, PROT_READ | PROT_WRITE,
				    MAP_SHARED, pdmas[i].fd, 0);
		if (pdmas[i].ptr == MAP_FAILED) {
			fprintf(stderr, "cannot mmap %s: %s\n",
				pdma_devname, strerror(errno));
			return -1;
		} else if (pdmas[i].ptr == NULL) {
			fprintf(stderr, "hmm\n");
			return -1;
		} else {
			printf("- mapped 0x%08lx bytes for %s at %p\n",
			       pdmas[i].sz, pdma_devname, pdmas[i].ptr);
		}
	}

	return 0;
}

static void clear_pdma_devs(struct pdma_t pdmas[], int num)
{
	int i;

	for(i = 0; i < num; i++) {
		free(pdmas[i].uioname);
		free(pdmas[i].pdmaname);
	}
}

static void unmap_pdma_devs(struct pdma_t pdmas[], int num)
{
	int i;

	for (i = 0; i < num; i++) {
		munmap(pdmas[i].ptr,  pdmas[i].sz);
		close(pdmas[i].fd);
	}
}

static char *pprint_region(uint64_t base, size_t sz)
{
	static char buf[256];
	int i;

	for(i = 0; i < NUM_REGIONS; i++) {
		if ((base >= regions[i].base) &&
		    ((base + sz) <= (regions[i].base + regions[i].sz))) {
			snprintf(buf, sizeof(buf), "%s", regions[i].name);
			return buf;
		}
	}

	snprintf(buf, sizeof(buf), "unknown");
	return buf;
}

static char *pprint_sz(double sz)
{
	static char buf[256];
	const char *suffixes[] = { "B", "KB", "MB", "GB", "TB", "PB", "EB" };
	int sfx = 0;
	double count = sz;

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
	static char buf[256];
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
		snprintf(buf, sizeof(buf), "%lu hrs, %lu mins, %lu.%06lu secs",
				hours, mins, secs, usecs);
	else if (mins)
		snprintf(buf, sizeof(buf), "%lu mins, %lu.%06lu secs",
				mins, secs, usecs);
	else
		snprintf(buf, sizeof(buf), "%lu.%06lu secs", secs, usecs);

	return buf;
}

static char *pprint_rate(size_t bytes, uint64_t usecs)
{
	static char buf[256];
	double bps = (double)bytes / (double)usecs;
	bps *= (double)1000000;

	snprintf(buf, sizeof(buf), "%s per sec", pprint_sz(bps));

	return buf;
}

static void check_buffer(void * buf1, void * buf2, size_t sz)
{
	int i;
	char *lbuf1 = (char *)buf1;
	char *lbuf2 = (char *)buf2;

	if (!buf1 || !buf2 || !sz) {
		fprintf(stderr, "bad buffer description\n");
		exit(-1);
	}

	if(memcmp(buf1, buf2, sz)) {
		fprintf(stderr, "error: buffers did not match\n");
		for (i = 0; i < sz; i++) {
			if (lbuf1[i] != lbuf2[i]) {
				printf ("%05d (%x vs %x)\n",
					i, lbuf1[i], lbuf2[i]);
				return;
			}
		}
	} else {
		printf("- buffers matched over %s\n",
		       pprint_sz(sz));
	}
}

static int64_t subtract_time(struct timeval *end, struct timeval *start)
{
	int64_t usecs;

	usecs = (end->tv_sec - start->tv_sec) * 1000000;

	usecs += end->tv_usec;
	usecs -= start->tv_usec;

	return usecs;
}

static void pdma_init(struct pdma_t pdmas[])
{
	MSS_PDMA_init(pdmas[0].ptr);
}

static void check_pdma_error(uint8_t pdma_error_code)
{
	if (pdma_error_code == 1)
		printf("Invalid source address\n");
	else if (pdma_error_code == 2)
		printf("Invalid destination address\n");
	else if (pdma_error_code == 3)
		printf("Transaction in progress\n");
	else if (pdma_error_code == 4)
		printf("Invalid Channel ID\n");
	else if (pdma_error_code == 5)
		printf("Invalid write size\n");
	else if (pdma_error_code == 6)
		printf("Invalid Read size\n");
	else if (pdma_error_code == 7)
		printf("Last ID\n");
	else
		printf("Unknown error\n");
}

uint8_t wait_for_int(struct pdma_t pdmas[], mss_pdma_channel_id_t chan)
{
	int reenable = 1;
	int count = 0;
	int i;
	#define NCHAN (2)

	struct pollfd fds[NCHAN] = { 
		{
			.fd = pdmas[chan * 2].fd,
			.events = POLLIN, 
		},
		{
			.fd = pdmas[(chan * 2) + 1].fd,
			.events = POLLIN,
		},
	};

	int ret = -1;

	while(1) {
		count++;
		ret = poll(fds, NCHAN, 100);
		if (ret == -1) {
			fprintf(stderr, "poll error\n");
			exit(-1);
		}
		if (ret != 0) {
			for (i = 0; i < NCHAN; i++) {
				if (fds[i].revents & POLLIN)
				{
					read(pdmas[(chan * 2) + i].fd,
					     &reenable, sizeof(int));
					write(pdmas[(chan * 2) +i].fd,
					      &reenable, sizeof(int));
					if (i == (NCHAN - 1)) {
						fprintf(stderr, "DMA error\n");
						exit(-1);
					}
					return 0;

				}
			}
		}
		if (count > 50) {
			fprintf(stderr, "timeout waiting for interrupt\n");
			exit(-1);
		}
	}

	return ret;
}

static struct buf_t *pdmacpy(struct buf_t *destbuf, struct buf_t *srcbuf,
			     size_t n, struct pdma_t pdmas[])
{
	uint64_t xfersz = (srcbuf->sz > destbuf->sz) ? destbuf->sz : srcbuf->sz;
	mss_pdma_channel_id_t chan;
	uint8_t pdma_error_code;

	chan = srcbuf->chan;
	if (chan == -1)
		chan = MSS_PDMA_first_free_channel();

	mss_pdma_channel_config_t pdma_config;
	pdma_config.src_addr  = srcbuf->base;
	pdma_config.dest_addr = destbuf->base;
	pdma_config.num_bytes = xfersz;
	pdma_config.enable_done_int = 1;
	pdma_config.enable_err_int = 1;
	pdma_config.force_order = 0;
	pdma_config.repeat = 0;
	
	if (chan < 0) {
		fprintf(stderr, "no PDMA channel available\n");
		return NULL;
	}

	pdma_error_code = MSS_PDMA_setup_transfer(chan,
						  &pdma_config);
	if (pdma_error_code != 0) {
		check_pdma_error(pdma_error_code);
		return NULL;
	}

	pdma_error_code = MSS_PDMA_start_transfer(chan);
	if (pdma_error_code != 0) {
		check_pdma_error(pdma_error_code);
		return NULL;
	}

	wait_for_int(pdmas, chan);

	return destbuf;
}

static void init_buf(uint8_t * buf, size_t buflen)
{
	int i;
	int rnum;

	for(i = 0; i < buflen/4; i++) {
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
static bool alloc_buf(struct pool_t *pool, size_t sz, struct buf_t *buf)
{
	if (sz + pool->allocated > pool->sz) {
		printf("failed to allocate buffer: "
		       "0x%lx available, 0x%lx requested\n",
		       pool->sz - pool->allocated,  sz);
		return false;
	}

	buf->base = pool->base + pool->allocated;
	buf->ptr = pool->ptr + pool->allocated;
	buf->sz = sz;
	buf->chan = -1; /* don't care */
	pool->allocated += sz;

	return true;
}

/* trivial de-allocator */
static void free_buf(struct pool_t *pool, struct buf_t *buf)
{
	pool->allocated -= buf->sz;
}

int main(void)
{
	struct pool_t * dma_pools =  NULL;
	struct pool_t * pool;
	char buf_provider[] = "u-dma-buf";
	int ret;
	struct timeval start_time;
	struct timeval end_time;
	size_t xfersz;
	uint64_t usecs;
	struct buf_t srcbuf;
	struct buf_t destbuf;
	int i;

	printf("locating contiguous buffer using %s\n", buf_provider);
	ret = get_pools(buf_provider, &dma_pools);
	if (ret < 0) {
		fprintf(stderr, "can't locate buffer for %s\n", buf_provider);
		return -1;
	}
	printf("- located %d contiguous buffers\n", get_num_pools(dma_pools));

	pool = dma_pools;
	printf("\n%-20s\tBase Address\t%-18s\t%-10s\tRegion\n", "Device Name", "Size", "Size");
	while(pool) {
		printf("%-20s\t0x%08lx\t0x%08lx bytes\t%-10s\t%s\n",
		       pool->name, pool->base, pool->sz,
		       pprint_sz(pool->sz),
		       pprint_region(pool->base, pool->sz));
		pool = pool->next;
	}

	printf("\nmapping contigous buffer using mmap()\n");
	ret = map_pools(dma_pools);
	if (ret < 0) {
		remove_pools(dma_pools);
		fprintf(stderr, "can't map buffers\n");
		return -1;
	} else {
		printf("- mapped all buffers\n");
	}

	printf("\nlocating PDMA devices\n");
	ret = get_pdma_devs(pdmas);
	if (ret < 0) {
		fprintf(stderr, "can't locate PDMA devices\n");
		return -1;
	}

	printf("- located %d devices\n", NUM_PDMA_DEVS);
	printf("\nDevice Name\tPDMA Channel\tAddress\t\tSize\n");
	for(i = 0; i < NUM_PDMA_DEVS; i++) {
		printf("%-12s\t%-12s\t0x%lx\t0x%lx\n",
		       pdmas[i].uioname, pdmas[i].pdmaname,
		       pdmas[i].base, pdmas[i].sz);
	}

	printf("\nmapping PDMA devices using mmap()\n");
	ret = map_pdma_devs(pdmas, NUM_PDMA_DEVS);
	if (ret < 0) {
		clear_pdma_devs(pdmas, NUM_PDMA_DEVS);
		fprintf(stderr, "can't map PDMA devices\n");
		return -1;
	} else {
		printf("- mapped all PDMA devices\n");
	}

	printf("\nDevice Name\tPDMA Channel\tAddress\t\tVirtual Address\tSize\n");
	for(i = 0; i < NUM_PDMA_DEVS; i++) {
		printf("%-12s\t%-12s\t0x%lx\t%p\t0x%lx\n",
		       pdmas[i].uioname, pdmas[i].pdmaname, pdmas[i].base,
		       pdmas[i].ptr, pdmas[i].sz);
	}

	pdma_init(pdmas);

	pool = dma_pools;
	while (pool) {

		if (false == alloc_buf(pool, pool->sz >> 1, &destbuf))
			exit(-1);
		if (false == alloc_buf(pool, pool->sz >> 1, &srcbuf))
			exit(-1);

		printf("\nPreparing buffers from %s\n", pool->name);
		printf("- Setting destination buffer (%s) to 0\n",
	       	       pprint_sz(destbuf.sz));
		memset(destbuf.ptr, 0x0, destbuf.sz);
		printf("- Initialising source buffer (%s) with PRBS\n",
		       pprint_sz(srcbuf.sz));
		init_buf(srcbuf.ptr, srcbuf.sz);

		xfersz = srcbuf.sz > destbuf.sz ? destbuf.sz : srcbuf.sz;

		/* test 1 */
		printf("\ntest 1 - %s\n", pool->name);
		fflush(stdout);
		gettimeofday(&start_time, NULL);
		memcpy(destbuf.ptr, srcbuf.ptr, xfersz);
		gettimeofday(&end_time, NULL);
		usecs = subtract_time(&end_time, &start_time);
		printf("- moved %s to 0x%08lx using memcpy() in",
		       pprint_sz(xfersz),
		       srcbuf.base);
		printf(" %s", 
			pprint_usecs(usecs));
		printf(" (%s)\n",
			pprint_rate(xfersz, usecs));
		check_buffer(srcbuf.ptr, destbuf.ptr, xfersz);

		/* test 2 */
		for (i = 0; i < 4; i++) {
			printf("\ntest 2.%d - %s\n", i, pool->name);
			printf("- Setting destination buffer %s to 0\n",
		       		pprint_sz(destbuf.sz));
			memset(destbuf.ptr, 0x0, destbuf.sz);
			srcbuf.chan = i;
			fflush(stdout);
			gettimeofday(&start_time, NULL);
			pdmacpy(&destbuf, &srcbuf, xfersz, pdmas);
			gettimeofday(&end_time, NULL);
			usecs = subtract_time(&end_time, &start_time);
			printf("- moved %s from 0x%08lx "
		       		"using pdmacpy (chan: %d) in",
		       		pprint_sz(xfersz),
		       		srcbuf.base,
		       		srcbuf.chan);
			printf(" %s", 
				pprint_usecs(usecs));
			printf(" (%s)\n",
				pprint_rate(xfersz, usecs));
			check_buffer(srcbuf.ptr, destbuf.ptr, xfersz);
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

	printf("- unmapping all PDMA devices\n");
	unmap_pdma_devs(pdmas, NUM_PDMA_DEVS);

	printf("- freeing references to PDMA devices\n");
	clear_pdma_devs(pdmas, NUM_PDMA_DEVS);

	return 0;
}
