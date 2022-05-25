// SPDX-License-Identifier: BSD-3-Clause
/*
 * GPIO event capture example -- Detects edge events and keeps count
 *
 */

#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>

#ifndef	CONSUMER
#define	CONSUMER	"Consumer"
#endif

int main(int argc, char **argv)
{
	char *chipname = "gpiochip0";
	unsigned int line_num = 30;	// GPIO Pin <sw2> 
	struct timespec ts = { 1, 0 };
	struct gpiod_line_event event;
	struct gpiod_chip *chip;
	struct gpiod_line *line;
	int i, ret;

	if(argc != 2) {
		perror("Incorrect parameters passed: gpiod-event <gpio pin> \n");
		ret = -1;
		goto end;
	}

	line_num = atoi(argv[1]);
	if(line_num > 31){
		perror("Max <gpio pin> 31\n");
		ret = -1;
		goto end;
	}

	chip = gpiod_chip_open_by_name(chipname);
	if (!chip) {
		perror("Open chip failed\n");
		ret = -1;
		goto end;
	}

	line = gpiod_chip_get_line(chip, line_num);
	if (!line) {
		perror("Get line failed\n");
		ret = -1;
		goto close_chip;
	}

	ret = gpiod_line_request_rising_edge_events(line, CONSUMER);
	if (ret < 0) {
		perror("Request event notification failed\n");
		ret = -1;
		goto release_line;
	}

	/* Run for 20 1 second windows */
	i = 0;
	int count_event = 0;
	printf("Application will process up to 20 events, press ctrl+c to exit early.\n");
	while (i++ < 20) {
		ret = gpiod_line_event_wait(line, &ts);
		if (ret < 0) {
			perror("gpiod_line_event_wait Failed\n");
			ret = -1;
			goto release_line;
		} else if (ret == 0) {
			printf("No event notification received on line  #%u\n", line_num);
			continue;
		}

		ret = gpiod_line_event_read(line, &event);
		count_event++;
		printf("Got event notification on line #%u %d times\n", line_num, count_event);
		if (ret < 0) {
			perror("gpiod_line_event_read failed\n");
			ret = -1;
			goto release_line;
		}
		sleep(1);
	}

	ret = 0;

release_line:
	gpiod_line_release(line);
close_chip:
	gpiod_chip_close(chip);
end:
	return ret;
}
