// SPDX-License-Identifier: MIT
/*
 *	GPIOD example -- blink PolarFire SoC Icicle kit LEDs and read SW2 switch state
 *
 *	Copyright (c) 2021 Microchip Inc.
 *
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>

#include <gpiod.h>

#define led_GPIO16 16
#define led_GPIO17 17
#define led_GPIO18 18
#define led_GPIO19 19
#define sw2_GPIO30 30

/* libgpiod public API */
/* Reference: https://git.kernel.org/pub/scm/libs/libgpiod/libgpiod.git/about/ */
/**
 * @brief Open a gpiochip by number.
 * @param num Number of the gpiochip.
 * @return GPIO chip handle or NULL if an error occurred.
 *
 * This routine appends num to '/dev/gpiochip' to create the path.
 */
//struct gpiod_chip *gpiod_chip_open_by_number(unsigned int num) GPIOD_API;


/**
 * @brief Get the handle to the GPIO line at given offset.
 * @param chip The GPIO chip object.
 * @param offset The offset of the GPIO line.
 * @return Pointer to the GPIO line handle or NULL if an error occured.
 */
//struct gpiod_line *
//gpiod_chip_get_line(struct gpiod_chip *chip, unsigned int offset) GPIOD_API;

/**
 * @brief Set the value of a single GPIO line.
 * @param line GPIO line object.
 * @param value New value.
 * @return 0 is the operation succeeds. In case of an error this routine
 *         returns -1 and sets the last error number.
 */
//int gpiod_line_set_value(struct gpiod_line *line, int value) GPIOD_API;

/**
 * @brief Read current value of a single GPIO line.
 * @param line GPIO line object.
 * @return 0 or 1 if the operation succeeds. On error this routine returns -1
 *         and sets the last error number.
 */
//int gpiod_line_get_value(struct gpiod_line *line) GPIOD_API;

	  

int main()
{
	char runcmd;
	char *chipname = "/dev/gpiochip0";
	struct gpiod_chip *chip;
	int counter = 0;

	//chip = gpiod_chip_open_by_name(chipname);
	chip = gpiod_chip_open(chipname);
	if (!chip) {
		perror("Open chip failed\n");
		goto end;
	}
  

	while(1){
	    printf("\n\t# Choose one of  the following options:");
	    printf("\n\tEnter '1' to blink LEDs for 10 seconds (GPIO16)1, (GPIO17)2, (GPIO18)3 and (GPIO19)4");
	    printf("\n\tEnter '2' to read the state of SW2 connected to GPIO30");
	    printf("\n\tEnter 'any key' to exit: ");
		
		scanf("%c%*c",&runcmd);
		if(runcmd=='1'){
			struct gpiod_line *led16;
			struct gpiod_line *led17;
			struct gpiod_line *led18;
			struct gpiod_line *led19;
			
			/* open the GPIO line */
			led16 = gpiod_chip_get_line(chip, led_GPIO16);
			if (!led16) {
				perror("led16 Get line failed\n");
				goto close_chip;
			}
			led17 = gpiod_chip_get_line(chip, led_GPIO17);
			if (!led17) {
				perror("led17 Get line failed\n");
				goto close_chip;
			}
			led18 = gpiod_chip_get_line(chip, led_GPIO18);
			if (!led18) {
				perror("led18 Get line failed\n");
				goto close_chip;
			}
			led19 = gpiod_chip_get_line(chip, led_GPIO19);
			if (!led19) {
				perror("led19 Get line failed\n");
				goto close_chip;
			}
	
		 
			/* config as output and set a description */
			gpiod_line_request_output(led16, "gpio-led16", GPIOD_LINE_ACTIVE_STATE_HIGH);
			gpiod_line_request_output(led17, "gpio-led17", GPIOD_LINE_ACTIVE_STATE_HIGH);
			gpiod_line_request_output(led18, "gpio-led18", GPIOD_LINE_ACTIVE_STATE_HIGH);
			gpiod_line_request_output(led19, "gpio-led19", GPIOD_LINE_ACTIVE_STATE_HIGH);
		
			
			counter = 10;
			int line_value = 0;
			printf("\n Check that LEDs are blinking, every 2 seconds \n");
			while(counter--){    
			    /* Set */
			    line_value = 1;
			    gpiod_line_set_value(led16, line_value);
			    gpiod_line_set_value(led17, line_value);
			    gpiod_line_set_value(led18, line_value);
			    gpiod_line_set_value(led19, line_value);
			    printf("\n LED's ON \n");
			    sleep(2);
			    
			    /* Clear */
			    line_value = 0;			    
			    gpiod_line_set_value(led16, line_value);
			    gpiod_line_set_value(led17, line_value);
			    gpiod_line_set_value(led18, line_value);
			    gpiod_line_set_value(led19, line_value);
			    printf("\n LED's Off \n");
			    sleep(2);
			}
			
			gpiod_line_release(led16);
			gpiod_line_release(led17);
			gpiod_line_release(led18);
			gpiod_line_release(led19);

		}
		else if(runcmd == '2') {
			struct gpiod_line *sw2_30;
			sw2_30 = gpiod_chip_get_line(chip, sw2_GPIO30);
			gpiod_line_request_input(sw2_30, "gpio30-sw2");
			
			int value = -1;
			int previous_value = -1;
			printf("\n Loop for 20 seconds reading the state of SW2, connected as an input to GPIO30 \n");
			counter = 20;
			while(counter--) {
			
			    sleep(1); 
			    value = gpiod_line_get_value(sw2_30);
			    if(value == -1) {
					printf("\n\t Error reading GPIO value: GPIO# %d \n", sw2_GPIO30);
				}
				else {
					if(value != previous_value)
						printf("\n\t SW2 switch Value is now: %d \n", value);
					previous_value = value;
				}			
			}
			gpiod_line_release(sw2_30);
				
		}
		else {
			printf("\n\t Exiting GPIO Example program...  \n");
			goto close_chip;
		
		}
		
   }
close_chip:   
   gpiod_chip_close(chip);
 end:  
   return 0;  
   
}
