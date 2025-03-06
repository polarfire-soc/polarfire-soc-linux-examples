// SPDX-License-Identifier: MIT
/**
 * Microchip Application to modify base time in
 * TSN Configuration file using PHC and specified
 * time offset
 *
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * Author: Pallela Venkat Karthik <pallela.karthik@microchip.com>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <getopt.h>
#include <sys/syscall.h>
#include <sys/timex.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cjson/cJSON.h>

#define JSON_KEY_QBV_CONF	"qbvconf"
#define JSON_KEY_BASE_TIME_SEC	"basetimesec"
#define JSON_KEY_BASE_TIME_NSEC	"basetimensec"

#define CLOCKFD			3
#define CLOCK_INVALID		-1
#define FD_TO_CLOCKID(fd)	((clockid_t)((((unsigned int)~fd) << 3) | CLOCKFD))

#ifndef HAVE_CLOCK_ADJTIME
static inline int clock_adjtime(clockid_t id, struct timex *tx)
{
	return syscall(__NR_clock_adjtime, id, tx);
}
#endif

#define NSEC_PER_SEC 1000000000

struct timespec timespec_normalise(struct timespec ts)
{
	while (ts.tv_nsec >= NSEC_PER_SEC) {
		++(ts.tv_sec);
		ts.tv_nsec -= NSEC_PER_SEC;
	}

	while (ts.tv_nsec <= -NSEC_PER_SEC) {
		--(ts.tv_sec);
		ts.tv_nsec += NSEC_PER_SEC;
	}

	if (ts.tv_nsec < 0) {
		/* Negative nanoseconds isn't valid according to POSIX.
		 * Decrement tv_sec and roll tv_nsec over.
		 */

		--(ts.tv_sec);
		ts.tv_nsec = (NSEC_PER_SEC + ts.tv_nsec);
	}

	return ts;
}

struct timespec timespec_add(struct timespec ts1, struct timespec ts2)
{
	/* Normalise inputs to prevent tv_nsec rollover if whole-second values
	 * are packed in it.
	 */
	ts1 = timespec_normalise(ts1);
	ts2 = timespec_normalise(ts2);

	ts1.tv_sec  += ts2.tv_sec;
	ts1.tv_nsec += ts2.tv_nsec;

	return timespec_normalise(ts1);
}

clockid_t phc_open(const char *phc)
{
	clockid_t clkid;
	struct timespec ts;
	struct timex tx;
	int fd;

	memset(&tx, 0, sizeof(tx));

	fd = open(phc, O_RDWR);
	if (fd < 0)
		return CLOCK_INVALID;

	clkid = FD_TO_CLOCKID(fd);
	/* check if clkid is valid */
	if (clock_gettime(clkid, &ts)) {
		close(fd);
		return CLOCK_INVALID;
	}
	if (clock_adjtime(clkid, &tx)) {
		close(fd);
		return CLOCK_INVALID;
	}

	return clkid;
}

static void print_usage(void)
{
	printf("microchip-tsn-replace-tsngentime --ptpfile=/dev/<ptpfile> --addtimesec=<addsecs> --addtimensec=<addnsec> --infile=inputfile --outfile=outputfile [--secroundoff]\n");
	printf("Example without seconds roundoff\n");
	printf("microchip-tsn-replace-tsngentime --ptpfile=/dev/ptp0 --addtimesec=5 --addtimensec=0 --infile=gen.json --outfile=gen2.json\n");
	printf("Example with seconds roundoff\n");
	printf("microchip-tsn-replace-tsngentime --ptpfile=/dev/ptp0 --addtimesec=5 --addtimensec=0 --infile=gen.json --outfile=gen2.json --secroundoff\n");
	printf("Example with seconds roundoff updating same config file\n");
	printf("microchip-tsn-replace-tsngentime --ptpfile=/dev/ptp0 --addtimesec=5 --addtimensec=0 --infile=gen.json --outfile=gen.json --secroundoff\n");
}

int main(int argc, char **argv)
{
	FILE *fp;
	int size, ret;
	char *jsondata;
	char *ptpdevpath;
	cJSON *json;
	cJSON *qbvconf;
	cJSON *tmp;
	clockid_t phc_clockid;
	struct timespec ts;
	struct timespec ts_add;
	struct timespec ts_new;

	int opt;
	int secroundoff = 0;
	int option_index = 0;
	char *ptpfile = NULL;
	char *addtimesec = NULL;
	char *addtimensec = NULL;
	char *infile = NULL;
	char *outfile = NULL;

	static struct option long_options[] = {
		{"ptpfile", required_argument, 0, 0},
		{"addtimesec", required_argument, 0, 0},
		{"addtimensec", required_argument, 0, 0},
		{"infile", required_argument, 0, 0},
		{"outfile", required_argument, 0, 0},
		{"secroundoff", no_argument, 0, 0}, // ceil round of seconds
		{0, 0, 0, 0}
	};

	while ((opt = getopt_long(argc, argv, "", long_options, &option_index)) != -1) {
		switch (opt) {
		case 0:
			if (strcmp(long_options[option_index].name, "ptpfile") == 0)
				ptpfile = optarg;
			else if (strcmp(long_options[option_index].name, "addtimesec") == 0)
				addtimesec = optarg;
			else if (strcmp(long_options[option_index].name, "addtimensec") == 0)
				addtimensec = optarg;
			else if (strcmp(long_options[option_index].name, "infile") == 0)
				infile = optarg;
			else if (strcmp(long_options[option_index].name, "outfile") == 0)
				outfile = optarg;
			else if (strcmp(long_options[option_index].name, "secroundoff") == 0)
				secroundoff = 1;
			break;
		case '?':
			print_usage();
				exit(1);
		default:
			print_usage();
				exit(1);
		}
	}

	if (!ptpfile || !addtimesec || !addtimensec || !infile) {
		print_usage();
		exit(1);
	}

	fp = fopen(infile, "r");

	if (!fp) {
		perror("fopen :");
		return errno;
	}

	ret  = fseek(fp, 0, SEEK_END);
	if (ret < 0) {
		perror("fseek :");
		fclose(fp);
		return errno;
	}
	size = ftell(fp);

	if (size < 0) {
		perror("ftell :");
		fclose(fp);
		return errno;
	}

	ret = fseek(fp, 0, SEEK_SET);
	if (ret < 0) {
		perror("fseek :");
		fclose(fp);
		return errno;
	}

	jsondata = malloc(size + 1);

	if (!jsondata) {
		printf("Failed to allocate memory\n");
		fclose(fp);
		return -ENOMEM;
	}

	jsondata[size] = '\0';

	ret = fread(jsondata, size, 1, fp);

	if (!ret) {
		printf("Failed to read file\n");
		fclose(fp);
		return 0;
	}

	json = cJSON_Parse(jsondata);

	if (!json) {
		printf("Failed to parse json data\n");
		free(jsondata);
		fclose(fp);
		return 0;
	}

	fclose(fp);
	ptpdevpath = ptpfile;
	ts_add.tv_sec = strtoul(addtimesec, NULL, 10);
	ts_add.tv_nsec = strtoul(addtimensec, NULL, 10);
	phc_clockid = phc_open(ptpdevpath);
	clock_gettime(phc_clockid, &ts);
	ts_new = timespec_add(ts, ts_add);

	if (secroundoff) {
		if (ts_new.tv_nsec) {
			ts_new.tv_nsec = 0;
			ts_new.tv_sec++;
		}
	}

	qbvconf = cJSON_GetObjectItemCaseSensitive(json, JSON_KEY_QBV_CONF);
	if (!qbvconf) {
		printf("qbvconf is not available, cannot change basetime\n");
		return 0;
	}
	tmp = cJSON_GetObjectItemCaseSensitive(qbvconf, JSON_KEY_BASE_TIME_SEC);
	if (!tmp) {
		printf("basetimesec is not available, cannot change basetime\n");
		return 0;
	}

	printf("current sec : %u\n", tmp->valueint);
	cJSON_SetIntValue(tmp, ts_new.tv_sec);
	printf("set sec : %lu\n", ts_new.tv_sec);
	tmp = cJSON_GetObjectItemCaseSensitive(qbvconf, JSON_KEY_BASE_TIME_NSEC);
	printf("current nsec : %u\n", tmp->valueint);
	cJSON_SetIntValue(tmp, ts_new.tv_nsec);
	printf("set nsec : %lu\n", ts_new.tv_nsec);

	if (outfile) {
		fp = fopen(outfile, "w+");
		if (!fp) {
			perror("fopen :");
			return errno;
		}

		ret = fprintf(fp, "%s", cJSON_Print(json));
		if (ret < 0)
			printf("writing to json outfile failed\n");

		fclose(fp);
	}
	cJSON_Delete(json);

	return 0;
}
