// SPDX-License-Identifier: MIT
/**
 * Microchip CoreTSN CLI example
 *
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * Author: Pallela Venkat Karthik <pallela.karthik@microchip.com>
 *
 */

#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <cjson/cJSON.h>
#include "microchip-tsn-lib.h"

#define REGEX				0
#define MAX_TSN_CONFIG_NAME_SIZE	32
#define MAX_GCL_CSV_LEN			64
#define MAX_USER_INPUT_LEN		128
#define MAX_USER_INPUT_YN_LEN		20
#define MAX_USER_INPUT_MACSTR_LEN	64
#define MAX_PRIOQ_STR_SIZE		64
#define MAX_GCL_STR_SIZE		128
#define MAX_PERCENT_STR_SIZE		32
#define CHARS_FOR_HEX_BYTE		2
#define MAX_JSON_DATA_SIZE		32768
#ifdef REGEX
#include<regex.h>
#define MAX_REGEX_MSGBUFF_LEN		100
#endif

#define TSN_NUM_CONFIFS 6
#define BIT(val, x) (!!((1 << x) & val))
#define PERCENTAGE(a, b) ((uint32_t)((100 * ((uint64_t)a)) / b))

#define JSON_KEY_QBV_CONF				"qbvconf"
#define JSON_KEY_CYCLE_TIME				"cycletime"
#define JSON_KEY_BASE_TIME_SEC				"basetimesec"
#define JSON_KEY_BASE_TIME_NSEC				"basetimensec"
#define JSON_KEY_BASE_TIME_ADJUST			"basetimeadjust"
#define JSON_KEY_GATE_CONTROL_LIST_COUNT		"gclcount"
#define JSON_KEY_INITIAL_GATE_STATE			"initgatestate"
#define JSON_KEY_PRIORITY_ENABLE			"priorityenable"
#define JSON_KEY_GATE_ENABLE				"gateenable"
#define JSON_KEY_PRIO_QUEUES				"prioqueues"
#define JSON_KEY_PRIO_QUEUE_NUM				"prioq"
#define JSON_KEY_PRIO_QUEUE_ENABLE			"enable"
#define JSON_KEY_PRIO_QUEUE_PRIORITY			"priority"
#define JSON_KEY_GATE_CONTROL_LIST			"gatecontrollist"
#define JSON_KEY_GATE_CONTROL_LIST_INDEX		"index"
#define JSON_KEY_GATE_CONTROL_LIST_GATE_STATE		"gatestate"
#define JSON_KEY_GATE_CONTROL_LIST_TIME_INTERVAL	"timeinterval"

#define JSON_KEY_QBU_CONF				"qbuconf"
#define JSON_KEY_PRE_EMPTION_ENABLE			"pre_empt_enable"
#define JSON_KEY_PRE_EMPTION_SIZE			"pre_empt_size"

#define JSON_KEY_QCI_CONF				"qciconf"
#define JSON_KEY_SOURCE_MAC_ADDRESS_CHECK		"sa_check"
#define JSON_KEY_DESTINATION_MAC_ADDRESS_CHECK		"da_check"
#define JSON_KEY_SOURCE_MAC_ADDRESS			"sourcemacaddr"
#define JSON_KEY_DESTINATION_MAC_ADDRESS		"destinationmacaddr"

#define JSON_KEY_PTP_CONF				"ptp_conf"
#define JSON_KEY_PTP_XMIT_PRIORITY_QUEUE		"ptp_tx_prioq"

#define JSON_KEY_RX_CONF				"rx_conf"
#define JSON_KEY_PORT_ID_RECV_CHECK			"port_id_rx_check"
#define JSON_KEY_PORT_ID_RECV				"port_id_rx"

#define JSON_KEY_MAC_PACKET				"macpacket"
#define JSON_KEY_LENGTH_DEDUCT_BYTES			"lengthdeductbytes"

#define NUM_PRIO_QUEUES					7

/* ANSI escape codes for colors */
#define RESET   "\x1b[0m"
#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"

#define COLOR_BIT(val, x) BIT(val, x) ? GREEN "1" RESET : RED "0" RESET

/* Q0 to Q6, Default do not allow setting prio to Q7 */
static int num_prio_queues = NUM_PRIO_QUEUES;

/* Get multiple configs */
static int get_qbv_conf;
static int get_qbu_conf;
static int get_qci_conf;
static int get_misc_ptp_tx_prioq_conf;
static int get_misc_rx_port_id_conf;
static int get_misc_length_deduct_byte_conf;
static int get_all_conf;
static int edit_mode;
static int json_show_gcl_csv;
static int show_gcl_remaining_cycle_time;

struct unsigned_integer_range {
	unsigned int min;
	unsigned int max;
};

struct unsigned_integer_ranges {
	unsigned int num_ranges;
	struct unsigned_integer_range range[];
};

struct unsigned_integer_ranges uint8_range = {
	.num_ranges = 1,
	.range = {
		{0, 0xff}
	}
};

struct unsigned_integer_ranges uint16_range = {
	.num_ranges = 1,
	.range = {
		{0, 0xffff}
	}
};

struct unsigned_integer_ranges uint32_range = {
	.num_ranges = 1,
	.range = {
		{0, 0xffffffff}
	}
};

struct unsigned_integer_ranges time_adjust_range = {
	.num_ranges = 1,
	.range = {
		{0, 0x3f}
	}
};

struct unsigned_integer_ranges pcp_range = {
	.num_ranges = 1,
	.range = {
		{0, 7}
	}
};

struct unsigned_integer_ranges control_list_length_range = {
	.num_ranges = 1,
	.range = {
		{0, 32}
	}
};

struct unsigned_integer_ranges pre_empt_size_range = {
	.num_ranges = 4,
	.range = {
		{60, 60},
		{124, 124},
		{188, 188},
		{252, 252},
	}
};

struct unsigned_integer_ranges crc_deduct_len_range = {
	.num_ranges = 1,
	.range = {
		{0, 2047},
	}
};

struct unsigned_integer_ranges *UINT8_RANGE = &uint8_range;
struct unsigned_integer_ranges *UINT16_RANGE = &uint16_range;
struct unsigned_integer_ranges *UINT32_RANGE = &uint32_range;
struct unsigned_integer_ranges *TIMEADJUST_RANGE = &time_adjust_range;
struct unsigned_integer_ranges *PCP_RANGE = &pcp_range;
struct unsigned_integer_ranges *CONTROL_LIST_LENGTH_RANGE = &control_list_length_range;
struct unsigned_integer_ranges *PREMPT_SIZE_RANGE = &pre_empt_size_range;
struct unsigned_integer_ranges *CRC_DEDUCT_LEN_RANGE = &crc_deduct_len_range;

struct option long_options[] = {
	{"show-devices", no_argument, 0, 0 },
	{"device", required_argument, 0, 0 },
	{"get", required_argument, 0, 0 },
	{"set", required_argument, 0, 0 },
	{"json", no_argument, 0, 0 },
	{"conf-file", required_argument, 0, 0 },
	{"export-conf-file", required_argument, 0, 0 },
	{"edit-mode", no_argument, 0, 0 },
	{"dispsel", no_argument, 0, 0 },
	{"show-gcl-csv", no_argument, 0, 0 },
	{"show-gcl-rct", no_argument, 0, 0 },
	{"enableprioq7", no_argument, 0, 0 },
};

typedef enum {
	SHOW_DEVICES,
	DEVICE,
	GET,
	SET,
	JSON,
	CONF_FILE,
	EXPORT_CONF_FILE,
	EDIT_MODE,
	DISPLAY_SELECTIONS,
	JSON_SHOW_GCL_CSV,
	SHOW_GCL_REMAINING_CYCLE_TIME,
	ENABLE_PRIOQ7_CONFIG,

} CMDLINE_OPTIONS;

/* Function to set the terminal text color to yellow */
void setYellowColor(void)
{
	printf("\033[43m"); /* ANSI escape code for yellow background */
}

/* Function to reset the terminal text color to default */
void resetColor(void)
{
	printf("\033[0m"); /* ANSI escape code to reset color */
}

/* Function to display the progress bar */
void displayProgressBar(int percentage)
{
	int width = 50; /* Width of the progress bar */
	int pos = (percentage * width) / 100;

	printf("[");
	setYellowColor();
	for (int i = 0; i < width; ++i) {
		if (i < pos) {
			printf(" ");
		} else {
			resetColor();
			printf(" ");
		}
	}
	resetColor();
	printf("] %d%%\n\n", percentage);
	fflush(stdout);
}

void percentage_string(char *result, unsigned int num1, unsigned int num2)
{
	double percentage;

	if (num2 != 0) {
		percentage = ((double)num1 / num2) * 100;
		snprintf(result, MAX_PERCENT_STR_SIZE, "%.2f%%", round(percentage * 100) / 100);
	} else {
		snprintf(result, MAX_PERCENT_STR_SIZE, "undefined (division by zero)");
	}
}

/* Function to return the address of the last line in a multi-line C string */
char *last_line(char *str)
{
	char *lastLine = str;
	char *current = str;

	if (!str)
		return NULL;


	while (*current != '\0') {
		if (*current == '\n')
			lastLine = current + 1;

		current++;
	}

	return lastLine;
}

/* Function to print colored text */
void colored_printf(const char *color, const char *format, ...)
{
	va_list args;

	va_start(args, format);

	/* Print the color code */
	printf("%s", color);

	/* Print the formatted string */
	vprintf(format, args);

	/* Reset the color */
	printf("%s", RESET);

	va_end(args);
}

static void print_usage(void)
{
	printf("\nCommand Line Arguments for microchip-tsn-cli\n");
	printf("microchip-tsn-cli [OPTIONS]\n");
	printf("--show-devices                                Display Available Devices\n");
	printf("--device=<devid> [--set | --get]=<conftype>   TSN Device id for set or get tsn config\n");
	printf("--get=[qbvconf | qbuconf | qciconf | misc_ptp_tx_prioq_conf | misc_rx_port_id_conf | misc_length_deduct_byte_conf | all ] [ --json | --export-conf-file=<file path> ] [--show-gcl-csv]     Get TSN Conf by type\n");
	printf("--set=[qbvconf | qbuconf | qciconf | misc_ptp_tx_prioq_conf | misc_rx_port_id_conf | misc_length_deduct_byte_conf]  [--conf-file=<json format conf>] [--edit-mode]   Set TSN Conf by type\n");
}

static void mac_addr_to_str(__u8 *mac_addr, __u8 *mac_str)
{
	sprintf((char *)mac_str, "%02x:%02x:%02x:%02x:%02x:%02x",
		mac_addr[0],
		mac_addr[1],
		mac_addr[2],
		mac_addr[3],
		mac_addr[4],
		mac_addr[5]);
}

typedef enum {
	QBVCONF,
	QBUCONF,
	QCICONF,
	MISC_PTP_TX_PRIOQ,
	MISC_RX_PORT_ID,
	MISC_LENGTH_DEDUCT_BYTE,
} TSN_CONFIGS;

static char tsn_configs[][MAX_TSN_CONFIG_NAME_SIZE] = {
	"qbvconf",
	"qbuconf",
	"qciconf",
	"misc_ptp_tx_prioq_conf",
	"misc_rx_port_id_conf",
	"misc_length_deduct_byte_conf",
};

#define NUM_TSN_CONFIGS (sizeof(tsn_configs) / sizeof(tsn_configs[0]))

int is_valid_mac_address(char *mac)
{
	int len = strlen(mac);
	int i;

	/* Check if the length is valid (12 characters without separators, 17 with separators) */
	if (len != 12 && len != 17)
		return 0;

	/* Check for valid characters and separators */
	for (i = 0; i < len; i++) {
		if (len == 17 && (i % 3 == 2)) {
			if (mac[i] != ':')
				return 0;

		} else {
			if (!isxdigit(mac[i]))
				return 0;
		}
	}

	return 1;
}

/* Function to trim leading and trailing spaces and tabs */
static void trim(char *str, char *num)
{
	char *start;
	char *end;

	start  = str;
	end = str + strlen(str) - 1;

	/* Trim leading space */
	while (isspace((unsigned char)*start))
		start++;

	if (*start == '\0') {
		num[0] = '\0';
		return;
	}

	/* Trim trailing space */
	while (isspace((unsigned char)*end))
		end--;

	/* Write new null terminator */
	*(end + 1) = 0;

	strcpy(num, start);
}

static void set_bits_from_string(const char *str, uint8_t *bitfield)
{
	char *token;
	/* Make a copy of the input string to avoid modifying the original */
	char *input_copy = strdup(str);

	if (!input_copy) {
		perror("strdup");
		return;
	}

	token = strtok(input_copy, ",");
	while (token) {
		int bit_position = atoi(token);

		if (bit_position >= 0 && bit_position <= 7)
			*bitfield |= (1 << bit_position);

		token = strtok(NULL, ",");
	}

	free(input_copy);
}

uint8_t gcl_csv_to_uint(char *csv_str)
{
	uint8_t gcl_state = 0;

	set_bits_from_string(csv_str, &gcl_state);

	return gcl_state;
}

static char gcl_csv_str[MAX_GCL_CSV_LEN];
char *gcl_state_to_csv(uint8_t gcl_state)
{
	int first = 1; /* Flag to handle comma placement */
	char temp[4];

	gcl_csv_str[0] = '\0';

	for (int i = 7; i >= 0; i--) {
		if (gcl_state & (1 << i)) {
			if (!first)
				strcat(gcl_csv_str, ", ");

			sprintf(temp, "%d", i);
			strcat(gcl_csv_str, temp);
			first = 0;
		}
	}

	return gcl_csv_str;
}

#if REGEX

regex_t regex_hexdec;
regex_t regex_dec;
regex_t regex_gcl_state_csv;

const char *hexdec_pattern = "^(0[xX])[0-9a-fA-F]+$";
const char *dec_pattern = "^[0-9]+$";
const char *gcl_state_csv_pattern = "^[0-7](,[0-7])*$";

int regex_compile(void)
{
	int ret;

	/* Compile the regular expression */
	ret = regcomp(&regex_hexdec, hexdec_pattern, REG_EXTENDED);
	if (ret) {
		fprintf(stderr, "Could not compile regex_hexdec\n");
		return 0;
	}

	ret = regcomp(&regex_dec, dec_pattern, REG_EXTENDED);
	if (ret) {
		fprintf(stderr, "Could not compile regex_dec\n");
		return 0;
	}

	ret = regcomp(&regex_gcl_state_csv, gcl_state_csv_pattern, REG_EXTENDED);
	if (ret) {
		fprintf(stderr, "Could not compile regex_gcl_state_csv\n");
		return 0;
	}

	return 0;
}

int isHexadecimal(const char *str)
{
	int ret;
	char msgbuf[MAX_REGEX_MSGBUFF_LEN];

	ret = regexec(&regex_hexdec, str, 0, NULL, 0);
	if (!ret)
		return 1; /* Match found */
	else if (ret == REG_NOMATCH)
		return 0; /* No match */

	regerror(ret, &regex_hexdec, msgbuf, sizeof(msgbuf));
	fprintf(stderr, "Regex match failed: %s\n", msgbuf);
	return 0;
}

int isDecimal(const char *str)
{
	int ret;
	char msgbuf[MAX_REGEX_MSGBUFF_LEN];

	/* Execute the regular expression */
	ret = regexec(&regex_dec, str, 0, NULL, 0);

	if (!ret)
		return 1; /* Match */
	else if (ret == REG_NOMATCH)
		return 0; /* No match */

	regerror(ret, &regex_dec, msgbuf, sizeof(msgbuf));
	fprintf(stderr, "Regex match failed: %s\n", msgbuf);
	return 0;
}

int is_gcl_state_csv(const char *str)
{
	int ret;
	char msgbuf[MAX_REGEX_MSGBUFF_LEN];

	ret = regexec(&regex_gcl_state_csv, str, 0, NULL, 0);
	if (!ret)
		return 1; /* Match found */
	else if (ret == REG_NOMATCH)
		return 0; /* No match */

	regerror(ret, &regex_hexdec, msgbuf, sizeof(msgbuf));
	fprintf(stderr, "Regex match failed: %s\n", msgbuf);
	return 0;
}

#else
/* Function to check if the string is a valid hexadecimal integer */
static int isHexadecimal(const char *str)
{
	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
		str += 2; /* Skip the "0x" or "0X" prefix */
		if (*str == '\0')
			return 0; /* Ensure there's something after "0x" */
		while (*str) {
			if (!isxdigit((unsigned char)*str))  {
				/*printf("isHexadecimal char : %u\n", *str); */
				return 0;
			}
			str++;
		}
		return 1;
	}
	return 0;
}

/* Function to check if the string is a valid decimal integer */
static int isDecimal(const char *str)
{
	if (*str == '-' || *str == '+')
		str++; /* Skip the sign if present */
	if (*str == '\0')
		return 0; /* Ensure there's something after the sign */
	while (*str) {
		if (!isdigit((unsigned char)*str)) {
			/*printf("isDecimal char : %u\n", *str); */
			return 0;
		}
		str++;
	}
	return 1;
}

int is_gcl_state_csv(const char *str)
{
	/* Check if the string is empty */
	if (!str || *str == '\0')
		return 0;

	/* Check the first character */
	if (*str < '0' || *str > '7')
		return 0;
	str++;

	/* Check the rest of the string */
	while (*str != '\0') {
		if (*str != ',')
			return 0;
		str++;
		if (*str < '0' || *str > '7')
			return 0;
		str++;
	}

	return 1;
}

#endif

static int print_unsigned_integer_ranges(struct unsigned_integer_ranges *intr)
{
	int i;

	printf("\nPlease Enter value within the ranges\n");
	for (i = 0; i < intr->num_ranges; i++) {
		if (intr->range[i].min < intr->range[i].max)
			printf("dec : %u to %u, hex : %x to %x\n",
			       intr->range[i].min, intr->range[i].max,
			       intr->range[i].min, intr->range[i].max);
		else
			printf("%u\n", intr->range[i].min); /* For single valued */
	}

	return 0;
}

static int verify_unsigned_integer_range(int num, struct unsigned_integer_ranges *intr)
{
	int i;

	for (i = 0; i < intr->num_ranges; i++) {
		if (intr->range[i].min <= num && intr->range[i].max >= num)
			return 1;
	}
	return 0;
}

static int unsigned_integer_input(char *display, int cur_value,
				  struct unsigned_integer_ranges *intr)
{
	char input[MAX_USER_INPUT_LEN];
	char numstr[MAX_USER_INPUT_LEN];
	int number;
	char *endptr;
	int dec, hex, csv = 0;

	number = cur_value;

	while (1) {
		if (edit_mode) {
			if (((strncmp(display, "GCL", 3)  == 0) && (strstr(display, "State"))) ||
			    (strcmp(display, "initial_gate_state") == 0))
				colored_printf(YELLOW, "%s (dec=%d, hex=0x%x, csv : %s) : ",
					       display, cur_value, cur_value,
					       gcl_state_to_csv(cur_value));
			else
				colored_printf(YELLOW, "%s (dec=%d, hex=0x%x) : ", display,
					       cur_value, cur_value);
		} else {
			colored_printf(YELLOW, "%s : ", display);
		}
		memset(input, 0, sizeof(input));
		if (fgets(input, sizeof(input), stdin) != NULL) {
			trim(input, numstr);
			if (numstr[0] == '\0') {
				if (edit_mode) {
					colored_printf(RED,
						       "No input given using existing value\n");
					break;
				}
				colored_printf(RED, "No input given, provide some input\n");
				continue;
			} else {
				if (strncmp(display, "GCL", 3)  == 0) {
					if (strstr(display, "State"))
						csv = is_gcl_state_csv(numstr);
				}
				dec = isDecimal(numstr);
				hex = isHexadecimal(numstr);
			}
			/* Convert input to integer */
			if (dec) {
				number = strtol(numstr, &endptr, 10);
				if (!verify_unsigned_integer_range(number, intr)) {
					print_unsigned_integer_ranges(intr);
					continue;
				}
				break;
			} else if (hex) {
				number = strtol(numstr, &endptr, 16);
				if (!verify_unsigned_integer_range(number, intr)) {
					print_unsigned_integer_ranges(intr);
					continue;
				}
				break;
			} else if (csv) {
				number = gcl_csv_to_uint(numstr);
				break;
			}
			colored_printf(RED, "Invalid input. Please enter a valid integer.\n");
			continue;
		} else {
			colored_printf(RED, "Error reading input. Please try again.\n");
		}
	}

	display = last_line(display);

	colored_printf(GREEN, "%s dec=%d, hex=0x%x\n\n", display, number, number);

	return number;
}

static int isyesorno(char *ynstr)
{
	char str[MAX_USER_INPUT_YN_LEN];
	int i = 0;
	int ret = 0;

	strncpy(str, ynstr, MAX_USER_INPUT_YN_LEN - 1);
	str[MAX_USER_INPUT_YN_LEN - 1] = '\0';

	while (str[i]) {
		str[i] = tolower(str[i]);
		i++;
	}

	if (strcmp(str, "y") == 0)
		ret = 1;
	else if (strcmp(str, "n") == 0)
		ret = 1;
	else if (strcmp(str, "yes") == 0)
		ret = 1;
	else if (strcmp(str, "no") == 0)
		ret = 1;

	return ret;
}

static int yesno_to_num(char *ynstr)
{
	char str[MAX_USER_INPUT_YN_LEN];
	int i = 0;
	int ret = 0;

	strncpy(str, ynstr, MAX_USER_INPUT_YN_LEN - 1);
	str[MAX_USER_INPUT_YN_LEN - 1] = '\0';

	while (str[i]) {
		str[i] = tolower(str[i]);
		i++;
	}

	if (strcmp(str, "y") == 0)
		ret = 1;
	else if (strcmp(str, "n") == 0)
		ret = 0;
	else if (strcmp(str, "yes") == 0)
		ret = 1;
	else if (strcmp(str, "no") == 0)
		ret = 0;

	return ret;
}

static int y_n_input(char *display, int cur_value)
{
	char input[MAX_USER_INPUT_YN_LEN];
	char ynstr[MAX_USER_INPUT_YN_LEN];
	int number;
	int valid;

	number = cur_value;

	while (1) {
		if (edit_mode)
			colored_printf(YELLOW, "%s (%s) [y/n] : ", display, cur_value ? "y" : "n");
		else
			colored_printf(YELLOW, "%s [y/n] : ", display);
		memset(input, 0, sizeof(input));
		if (fgets(input, sizeof(input), stdin)) {
			trim(input, ynstr);
			if (ynstr[0] == '\0') {
				if (edit_mode) {
					colored_printf(RED,
						       "No input given using existing value\n");
					break;
				}
				colored_printf(RED, "No input given, provide some input\n");
				continue;

			} else {
				valid = isyesorno(ynstr);
			}
			if (valid) {
				number = yesno_to_num(ynstr);
				break;
			}
			colored_printf(RED, "Invalid input. Please enter valid inputs\n");
			colored_printf(YELLOW, "yes : y, yes\n");
			colored_printf(YELLOW, "no  : n, no\n");
			continue;
		} else {
			colored_printf(RED, "Error reading input. Please try again.\n");
		}
	}

	colored_printf(GREEN, "%s=%s\n\n", display, number ? "y" : "n");

	return number;
}

static int hex_char_to_int(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	else if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	else if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	else
		return -1;
}

static int mac_str_to_bytes(const char *mac_str, unsigned char *mac_bytes)
{
	int len = strlen(mac_str);
	int byte_index = 0;
	int nibble = 0;
	int value;

	for (int i = 0; i < len; i++) {
		if (mac_str[i] == ':')
			continue; /* Skip colons */

		value = hex_char_to_int(mac_str[i]);
		if (value == -1)
			return -1; /* Invalid character */

		if (nibble == 0) {
			mac_bytes[byte_index] = value << 4;
			nibble = 1;
		} else {
			mac_bytes[byte_index] |= value;
			byte_index++;
			nibble = 0;
		}

		if (byte_index > 6)
			return -1; /* Too many bytes */
	}

	return (byte_index == 6) ? 0 : -1; /* Ensure exactly 6 bytes */
}

static int mac_addr_input(char *display, unsigned char *cur_mac_addr)
{
	char input[MAX_USER_INPUT_MACSTR_LEN];
	char macstr[MAX_USER_INPUT_MACSTR_LEN];
	int valid;

	while (1) {
		if (edit_mode)
			colored_printf(YELLOW, "%s (%02x:%02x:%02x:%02x:%02x:%02x): ", display,
				       cur_mac_addr[0],
				       cur_mac_addr[1],
				       cur_mac_addr[2],
				       cur_mac_addr[3],
				       cur_mac_addr[4],
				       cur_mac_addr[5]);
		else
			colored_printf(YELLOW, "%s : ", display);
		memset(input, 0, sizeof(input));
		if (fgets(input, sizeof(input), stdin)) {
			trim(input, macstr);
			if (macstr[0] == '\0') {
				if (edit_mode) {
					colored_printf(RED,
						       "No input given using existing value\n");
					break;
				}
				colored_printf(RED, "No input given, provide some input\n");
				continue;
			} else {
				valid = is_valid_mac_address(macstr);
			}
			if (valid) {
				mac_str_to_bytes(macstr, cur_mac_addr);
				break;
			}
			colored_printf(RED, "Invalid input. Please enter valid inputs\n");
			continue;
		} else {
			colored_printf(RED, "Error reading input. Please try again.\n");
		}
	}

	colored_printf(GREEN, "%s=%02x:%02x:%02x:%02x:%02x:%02x\n\n", display,
		       cur_mac_addr[0],
		       cur_mac_addr[1],
		       cur_mac_addr[2],
		       cur_mac_addr[3],
		       cur_mac_addr[4],
		       cur_mac_addr[5]);

	return 0;
}

static void *all_config_adjust(void *conf_mem, char *tsnspec)
{
	int offset = 0;

	if (strcmp(tsnspec, "qbvconf") == 0)
		offset = 0;
	else if (strcmp(tsnspec, "qbuconf") == 0)
		offset = MAX_TSN_CONFIG_SIZE;
	else if (strcmp(tsnspec, "qciconf") == 0)
		offset = 2 * MAX_TSN_CONFIG_SIZE;
	else if (strcmp(tsnspec, "misc_ptp_tx_prioq_conf") == 0)
		offset = 3 * MAX_TSN_CONFIG_SIZE;
	else if (strcmp(tsnspec, "misc_rx_port_id_conf") == 0)
		offset = 4 * MAX_TSN_CONFIG_SIZE;
	else if (strcmp(tsnspec, "misc_length_deduct_byte_conf") == 0)
		offset = 5 * MAX_TSN_CONFIG_SIZE;
	else
		return NULL;

	return (conf_mem + offset);
}

static void microchip_tsn_display_devices(void)
{
	int i, devcnt;
	struct microchip_tsn_device dev[10];

	devcnt = microchip_tsn_get_device_list(dev, 10);

	if (devcnt > 0) {
		printf("'-----------------------------'\n");
		printf("| Serialno |      devid       |\n");
		printf("'-----------------------------'\n");
		for (i = 0; i < devcnt; i++)
			printf("| %05u    | %16llx |\n", i, dev[i].tsn_dev_id);

		printf("'-----------------------------'\n");
	} else if (devcnt < 0)  {
		if (devcnt == -ENOENT)
			printf("Driver missing for coretsn\n");
		else
			printf("tsn display devices failed with err  :%d\n", devcnt);
	} else {
		printf("No tsn devices available\n");
	}
}

static void display_tsn_conf(void *conf_mem, char *tsnspec, char *json_data)
{
	struct qbv_conf *qbvconf;
	struct qbu_conf *qbuconf;
	struct qci_conf *qciconf;
	struct misc_rx_port_id_conf *miscrxportidconf;
	struct misc_ptp_tx_prioq_conf *miscptptxprioqconf;
	struct misc_length_deduct_byte_conf *miscldbconf;
	int i;
	int all = 0;
	unsigned int gate_state = 0;
	unsigned char mac_str[MAX_USER_INPUT_MACSTR_LEN];
	cJSON *json = NULL;
	cJSON *jqbvconf = NULL;
	cJSON *jqbvconf_prioq = NULL;
	cJSON *jqbvconf_prioq_elem = NULL;
	cJSON *jqbvconf_gcl = NULL;
	cJSON *jqbvconf_gcle = NULL;
	cJSON *jqbuconf = NULL;
	cJSON *jqciconf = NULL;
	cJSON *jmiscrxportidconf = NULL;
	cJSON *jmiscptptxprioqconf = NULL;
	cJSON *jmiscldbconf = NULL;

	if (strcmp("all", tsnspec) == 0) {
		all = 1;
		printf("Displaying all configurations\n");
	}

	if (json_data)
		json =  cJSON_CreateObject();

	if (all || strcmp(tsnspec, "qbvconf") == 0) {
		qbvconf =  (struct qbv_conf *)
			   (all ? all_config_adjust(conf_mem, "qbvconf") : conf_mem);
		if (json_data) {
			jqbvconf = cJSON_AddObjectToObject(json, JSON_KEY_QBV_CONF);
			cJSON_AddNumberToObject(jqbvconf, JSON_KEY_CYCLE_TIME,
						qbvconf->cycle_time);
			cJSON_AddNumberToObject(jqbvconf, JSON_KEY_BASE_TIME_SEC,
						qbvconf->basetime_sec);
			cJSON_AddNumberToObject(jqbvconf, JSON_KEY_BASE_TIME_NSEC,
						qbvconf->basetime_nsec);
			cJSON_AddNumberToObject(jqbvconf, JSON_KEY_BASE_TIME_ADJUST,
						qbvconf->basetime_adjust);
			cJSON_AddNumberToObject(jqbvconf, JSON_KEY_GATE_CONTROL_LIST_COUNT,
						qbvconf->control_list_length);
			if (json_show_gcl_csv)
				cJSON_AddItemToObject(jqbvconf, "initial_gate_state",
						      cJSON_CreateString
						      ((char *)gcl_state_to_csv
						      (qbvconf->initial_gate_state)));
			else
				cJSON_AddNumberToObject(jqbvconf, JSON_KEY_INITIAL_GATE_STATE,
							qbvconf->initial_gate_state);

			if (qbvconf->priority_enable)
				cJSON_AddTrueToObject(jqbvconf, JSON_KEY_PRIORITY_ENABLE);
			else
				cJSON_AddFalseToObject(jqbvconf, JSON_KEY_PRIORITY_ENABLE);

			if (qbvconf->gate_enable)
				cJSON_AddTrueToObject(jqbvconf, JSON_KEY_GATE_ENABLE);
			else
				cJSON_AddFalseToObject(jqbvconf, JSON_KEY_GATE_ENABLE);

			jqbvconf_prioq = cJSON_CreateArray();
			jqbvconf_gcl = cJSON_CreateArray();
			for (i = 0; i < num_prio_queues; i++) {
				jqbvconf_prioq_elem = cJSON_CreateObject();
				cJSON_AddItemToObject(jqbvconf_prioq_elem, JSON_KEY_PRIO_QUEUE_NUM,
						      cJSON_CreateNumber(i));
				if (qbvconf->priority_queue_enable & (1 << i))
					cJSON_AddTrueToObject(jqbvconf_prioq_elem,
							      JSON_KEY_PRIO_QUEUE_ENABLE);
				else
					cJSON_AddFalseToObject(jqbvconf_prioq_elem,
							       JSON_KEY_PRIO_QUEUE_ENABLE);
				cJSON_AddItemToObject(jqbvconf_prioq_elem,
						      JSON_KEY_PRIO_QUEUE_PRIORITY,
						      cJSON_CreateNumber
						      (qbvconf->priority_queue_prios[i]));
				cJSON_AddItemToArray(jqbvconf_prioq, jqbvconf_prioq_elem);
			}
			cJSON_AddItemToObject(jqbvconf, JSON_KEY_PRIO_QUEUES, jqbvconf_prioq);
			for (i = 0; i < qbvconf->control_list_length; i++) {
				jqbvconf_gcle = cJSON_CreateObject();
				cJSON_AddItemToObject(jqbvconf_gcle,
						      JSON_KEY_GATE_CONTROL_LIST_INDEX,
						      cJSON_CreateNumber(i));
				if (json_show_gcl_csv)
					cJSON_AddItemToObject(jqbvconf_gcle,
							      JSON_KEY_GATE_CONTROL_LIST_GATE_STATE,
							      cJSON_CreateString
							      ((char *)gcl_state_to_csv
							      (qbvconf->gcle[i].gate_state)));
				else
					cJSON_AddItemToObject(jqbvconf_gcle,
							      JSON_KEY_GATE_CONTROL_LIST_GATE_STATE,
							      cJSON_CreateNumber
							      (qbvconf->gcle[i].gate_state));
				cJSON_AddItemToObject(jqbvconf_gcle,
						      JSON_KEY_GATE_CONTROL_LIST_TIME_INTERVAL,
						      cJSON_CreateNumber
						      (qbvconf->gcle[i].time_interval));
				cJSON_AddItemToArray(jqbvconf_gcl, jqbvconf_gcle);
			}
			cJSON_AddItemToObject(jqbvconf, JSON_KEY_GATE_CONTROL_LIST, jqbvconf_gcl);
		} else {
			printf("\n<<<<<<<<QBVCONF>>>>>>>>>>>>>>>>>>>\n");
			printf("cycle_time : %u [0x%x]\n", qbvconf->cycle_time,
			       qbvconf->cycle_time);
			printf("basetime_sec : %llu [0x%llx]\n", qbvconf->basetime_sec,
			       qbvconf->basetime_sec);
			printf("basetime_nsec : %u [0x%x]\n", qbvconf->basetime_nsec,
			       qbvconf->basetime_nsec);
			printf("basetime_adjust : %u [0x%x]\n", qbvconf->basetime_adjust,
			       qbvconf->basetime_adjust);
			printf("gate_enable : %s\n",
			       qbvconf->gate_enable ? GREEN "Enabled" RESET : RED "Disabled" RESET);
			printf("priority_enable : %s\n",
			       qbvconf->priority_enable ? GREEN "Enabled" RESET :
			       RED "Disabled" RESET);
			for (i = 0; i < num_prio_queues; i++)
				printf("priority Queue %d : %s\n", i,
				       qbvconf->priority_queue_enable & (1 << i)
				       ? GREEN "Enabled" RESET : RED "Disabled" RESET);

			for (i = 0; i < num_prio_queues; i++)
				printf("priority Queue %d prio : %d\n", i,
				       qbvconf->priority_queue_prios[i]);

			printf("control_list_length : %u [0x%x]\n", qbvconf->control_list_length,
			       qbvconf->control_list_length);
			printf("initial_gate_state : %u [0x%x] [%s]\n",
			       qbvconf->initial_gate_state,
			       qbvconf->initial_gate_state,
			       gcl_state_to_csv(qbvconf->initial_gate_state));
			if (qbvconf->control_list_length > 0) {
				printf("Entry    Gate                  Time Interval\n");
				for (i = 0; i < qbvconf->control_list_length; i++) {
					gate_state = qbvconf->gcle[i].gate_state;
					printf(YELLOW" %02x       %02x "RESET"[%s %s %s %s %s %s %s %s]     %" PRIu32 "\n",
					       i, gate_state,
					       COLOR_BIT(gate_state, 7),
					       COLOR_BIT(gate_state, 6),
					       COLOR_BIT(gate_state, 5),
					       COLOR_BIT(gate_state, 4),
					       COLOR_BIT(gate_state, 3),
					       COLOR_BIT(gate_state, 2),
					       COLOR_BIT(gate_state, 1),
					       COLOR_BIT(gate_state, 0),
					       qbvconf->gcle[i].time_interval);
				}
				printf("____________________________________\n");
			}
		}
	}
	if (all || strcmp(tsnspec, "qbuconf") == 0) {
		qbuconf =  (struct qbu_conf *)
			   (all ? all_config_adjust(conf_mem, "qbuconf") : conf_mem);
		if (json_data) {
			jqbuconf = cJSON_AddObjectToObject(json, JSON_KEY_QBU_CONF);
			if (qbuconf->pre_empt_en)
				cJSON_AddTrueToObject(jqbuconf, JSON_KEY_PRE_EMPTION_ENABLE);
			else
				cJSON_AddFalseToObject(jqbuconf, JSON_KEY_PRE_EMPTION_ENABLE);
			cJSON_AddNumberToObject(jqbuconf, JSON_KEY_PRE_EMPTION_SIZE,
						qbuconf->pre_empt_size);
		} else {
			printf("\n<<<<<<<<QBUCONF>>>>>>>>>>>>>>>>>>>\n");
			printf("pre_empt enable : %s\n", qbuconf->pre_empt_en ? "True" : "False");
			printf("pre_empt size : %d\n", qbuconf->pre_empt_size);
			printf("____________________________________\n");
		}
	}
	if (all || strcmp(tsnspec, "qciconf") == 0) {
		qciconf =  (struct qci_conf *)
			   (all ? all_config_adjust(conf_mem, "qciconf") : conf_mem);
		if (json_data) {
			jqciconf = cJSON_AddObjectToObject(json, JSON_KEY_QCI_CONF);
			if (qciconf->sa_check)
				cJSON_AddTrueToObject(jqciconf, JSON_KEY_SOURCE_MAC_ADDRESS_CHECK);
			else
				cJSON_AddFalseToObject(jqciconf, JSON_KEY_SOURCE_MAC_ADDRESS_CHECK);
			if (qciconf->da_check)
				cJSON_AddTrueToObject(jqciconf,
						      JSON_KEY_DESTINATION_MAC_ADDRESS_CHECK);
			else
				cJSON_AddFalseToObject(jqciconf,
						       JSON_KEY_DESTINATION_MAC_ADDRESS_CHECK);
			mac_addr_to_str(qciconf->source_mac_addr, mac_str);
			cJSON_AddStringToObject(jqciconf, JSON_KEY_SOURCE_MAC_ADDRESS,
						(char *)mac_str);
			mac_addr_to_str(qciconf->destination_mac_addr, mac_str);
			cJSON_AddStringToObject(jqciconf, JSON_KEY_DESTINATION_MAC_ADDRESS,
						(char *)mac_str);
		} else {
			printf("\n<<<<<<<<QCICONF>>>>>>>>>>>>>>>>>>>\n");
			printf("sa_check : %s\n", qciconf->sa_check ? "True" : "False");
			printf("da_check : %s\n", qciconf->da_check ? "True" : "False");
			mac_addr_to_str(qciconf->destination_mac_addr, mac_str);
			printf("Destination MAC Addr : %s\n", mac_str);
			mac_addr_to_str(qciconf->source_mac_addr, mac_str);
			printf("Source MAC Addr : %s\n", mac_str);
			printf("____________________________________\n");
		}
	}
	if (all || strcmp(tsnspec, "misc_ptp_tx_prioq_conf") == 0) {
		miscptptxprioqconf =  (struct misc_ptp_tx_prioq_conf *)
				      (all ? all_config_adjust
				      (conf_mem, "misc_ptp_tx_prioq_conf") : conf_mem);
		if (json_data) {
			jmiscptptxprioqconf = cJSON_AddObjectToObject(json, JSON_KEY_PTP_CONF);
			cJSON_AddNumberToObject(jmiscptptxprioqconf,
						JSON_KEY_PTP_XMIT_PRIORITY_QUEUE,
						miscptptxprioqconf->ptp_tx_prioq);
		} else {
			printf("\n<<<<MISC_PTP_TX_PRIOQ_CONF>>>>>>>>\n");
			printf("ptp tx prioq : %d\n", miscptptxprioqconf->ptp_tx_prioq);
			printf("____________________________________\n");
		}
	}
	if (all || strcmp(tsnspec, "misc_rx_port_id_conf") == 0) {
		miscrxportidconf = (struct misc_rx_port_id_conf *)
				   (all ?
				   all_config_adjust(conf_mem, "misc_rx_port_id_conf") : conf_mem);
		if (json_data) {
			jmiscrxportidconf = cJSON_AddObjectToObject(json, JSON_KEY_RX_CONF);
			if (miscrxportidconf->port_id_rx_check)
				cJSON_AddTrueToObject(jmiscrxportidconf,
						      JSON_KEY_PORT_ID_RECV_CHECK);
			else
				cJSON_AddFalseToObject(jmiscrxportidconf,
						       JSON_KEY_PORT_ID_RECV_CHECK);
			cJSON_AddNumberToObject(jmiscrxportidconf, JSON_KEY_PORT_ID_RECV,
						miscrxportidconf->port_id_rx);
		} else {
			printf("\n<<<<MISC_RX_PORT_ID_CONF>>>>>>>>>>\n");
			printf("port_id_rx_check : %s\n",
			       miscrxportidconf->port_id_rx_check ? "True" : "False");
			printf("port_id_rx : %d\n", miscrxportidconf->port_id_rx);
			printf("____________________________________\n");
		}
	}
	if (all || strcmp(tsnspec, "misc_length_deduct_byte_conf") == 0) {
		miscldbconf = (struct misc_length_deduct_byte_conf *)
			      (all ? all_config_adjust
			      (conf_mem, "misc_length_deduct_byte_conf") : conf_mem);
		if (json_data) {
			jmiscldbconf = cJSON_AddObjectToObject(json, JSON_KEY_MAC_PACKET);
			cJSON_AddNumberToObject(jmiscldbconf, JSON_KEY_LENGTH_DEDUCT_BYTES,
						miscldbconf->crc_deduct_len);
		} else {
			printf("\n<<<<MISC_LENGTH_DEDUCT_BYTE_CONF>>\n");
			printf("MAC packet length deduct bytes : %u\n",
			       miscldbconf->crc_deduct_len);
			printf("____________________________________\n");
		}
	}

	if (json_data) {
		strcpy(json_data, cJSON_Print(json));
		cJSON_Delete(json);
	}
}

static void microchip_tsn_get_config(__u64 devid, char *tsnspec, int json_format,
				     char *json_file_path)
{
	void *conf_mem;
	struct qbv_conf *qbvconf;
	struct qbu_conf *qbuconf;
	struct qci_conf *qciconf;
	struct misc_rx_port_id_conf *miscrxportidconf;
	struct misc_ptp_tx_prioq_conf *miscptptxprioqconf;
	struct misc_length_deduct_byte_conf *miscldbconf;
	struct microchip_tsn_device dev;
	char *json_data = NULL;
	FILE *fp;
	int malloc_size;
	int ret = EINVAL;

	printf("tsn get devid : %llx, tsnspec :%s\n", devid, tsnspec);
	if (strcmp(tsnspec, "all") == 0)
		malloc_size  = TSN_NUM_CONFIFS * MAX_TSN_CONFIG_SIZE;
	else
		malloc_size = MAX_TSN_CONFIG_SIZE;

	conf_mem = malloc(malloc_size);
	memset(conf_mem, 0, malloc_size);

	if (!conf_mem)
		return;

	if (json_format) {
		json_data = malloc(MAX_JSON_DATA_SIZE);
		if (!json_data) {
			free(conf_mem);
			return;
		}
	}

	qbvconf = (struct qbv_conf *)conf_mem;
	qbuconf = (struct qbu_conf *)conf_mem;
	qciconf = (struct qci_conf *)conf_mem;
	miscrxportidconf = (struct misc_rx_port_id_conf *)conf_mem;
	miscptptxprioqconf = (struct misc_ptp_tx_prioq_conf *)conf_mem;
	miscldbconf = (struct misc_length_deduct_byte_conf *)conf_mem;

	if (strcmp(tsnspec, "all") == 0) {
		qbvconf = (struct qbv_conf *)all_config_adjust(conf_mem, "qbvconf");
		qbuconf = (struct qbu_conf *)all_config_adjust(conf_mem, "qbuconf");
		qciconf = (struct qci_conf *)all_config_adjust(conf_mem, "qciconf");
		miscrxportidconf = (struct misc_rx_port_id_conf *)all_config_adjust(conf_mem,
				   "misc_rx_port_id_conf");
		miscptptxprioqconf = (struct misc_ptp_tx_prioq_conf *)all_config_adjust(conf_mem,
				     "misc_ptp_tx_prioq_conf");
		miscldbconf = (struct misc_length_deduct_byte_conf *)all_config_adjust(conf_mem,
			      "misc_length_deduct_byte_conf");
	}

	dev.tsn_dev_id = devid;

	if (strcmp(tsnspec, "qbvconf") == 0) {
		ret = microchip_tsn_device_get_qbv_conf(&dev, qbvconf);
	} else if (strcmp(tsnspec, "qbuconf") == 0) {
		ret = microchip_tsn_device_get_qbu_conf(&dev, qbuconf);
	} else if (strcmp(tsnspec, "qciconf") == 0) {
		ret = microchip_tsn_device_get_qci_conf(&dev, qciconf);
	} else if (strcmp(tsnspec, "misc_rx_port_id_conf") == 0) {
		ret = microchip_tsn_misc_get_rx_port_id(&dev, miscrxportidconf);
	} else if (strcmp(tsnspec, "misc_ptp_tx_prioq_conf") == 0) {
		ret = microchip_tsn_misc_get_tx_ptp_prioq(&dev, miscptptxprioqconf);
	} else if (strcmp(tsnspec, "misc_length_deduct_byte_conf") == 0) {
		ret = microchip_tsn_misc_get_length_deduct_byte(&dev, miscldbconf);
	} else if (strcmp(tsnspec, "all") == 0) {
		ret = microchip_tsn_device_get_qbv_conf(&dev, qbvconf);
		if (ret)
			printf("Getting Config %s failed with error : %d\n", "qbvconf", ret);

		ret = microchip_tsn_device_get_qbu_conf(&dev, qbuconf);
		if (ret)
			printf("Getting Config %s failed with error : %d\n", "qbuconf", ret);

		ret = microchip_tsn_device_get_qci_conf(&dev, qciconf);
		if (ret)
			printf("Getting Config %s failed with error : %d\n", "qciconf", ret);

		ret = microchip_tsn_misc_get_rx_port_id(&dev, miscrxportidconf);
		if (ret)
			printf("Getting Config %s failed with error : %d\n", "misc_rx_port_id_conf",
			       ret);

		ret = microchip_tsn_misc_get_tx_ptp_prioq(&dev, miscptptxprioqconf);
		if (ret)
			printf("Getting Config %s failed with error : %d\n",
			       "misc_ptp_tx_prioq_conf", ret);

		ret = microchip_tsn_misc_get_length_deduct_byte(&dev, miscldbconf);
		if (ret)
			printf("Getting Config %s failed with error : %d\n",
			       "misc_length_deduct_byte_conf", ret);
	}
	printf("Get Config done\n");

	if (ret) {
		printf("Getting Config %s failed with error : %d\n", tsnspec, ret);
		free(conf_mem);
		return;
	}

	display_tsn_conf(conf_mem, tsnspec, json_data);

	if (json_format) {
		if (json_file_path) {
			/* Write to file path */
			fp = fopen(json_file_path, "w+");
			if (!fp) {
				perror("export to file");
			} else {
				ret = fwrite(json_data, strlen(json_data), 1, fp);
				if (ret)
					perror("export to file");
				fclose(fp);
			}
		} else {
			printf("%s\n", json_data);
		}
	}

	free(conf_mem);
}

static void input_tsn_conf(void *conf_mem, char *tsnspec)
{
	struct qbv_conf *qbvconf;
	struct qbu_conf *qbuconf;
	struct qci_conf *qciconf;
	struct misc_rx_port_id_conf *miscrxportidconf;
	struct misc_ptp_tx_prioq_conf *miscptptxprioqconf;
	struct misc_length_deduct_byte_conf *miscldbconf;
	int i;
	int prioqen;
	unsigned int uint_input;
	unsigned int remaining_cycle_time;
	char prioq_str[MAX_PRIOQ_STR_SIZE];
	char gcl_str[MAX_GCL_STR_SIZE];
	char percent_str[MAX_PERCENT_STR_SIZE];

	if (strcmp(tsnspec, "qbvconf") == 0) {
		qbvconf = (struct qbv_conf *)conf_mem;
		uint_input = unsigned_integer_input("cycle_time", qbvconf->cycle_time,
						    UINT32_RANGE);
		qbvconf->cycle_time = uint_input;

		uint_input = unsigned_integer_input("basetime_sec", qbvconf->basetime_sec,
						    UINT32_RANGE);
		qbvconf->basetime_sec = uint_input;

		uint_input = unsigned_integer_input("basetime_nsec", qbvconf->basetime_nsec,
						    UINT32_RANGE);
		qbvconf->basetime_nsec = uint_input;

		uint_input = unsigned_integer_input("basetime_adjust", qbvconf->basetime_adjust,
						    TIMEADJUST_RANGE);
		qbvconf->basetime_adjust = uint_input;

		uint_input = y_n_input("priority_enable", qbvconf->priority_enable);
		qbvconf->priority_enable = uint_input;

		uint_input = y_n_input("gate_enable", qbvconf->gate_enable);
		qbvconf->gate_enable = uint_input;

		for (i = 0; i < num_prio_queues; i++) {
			sprintf(prioq_str, "priority Queue %d Enable", i);
			prioqen = y_n_input(prioq_str, qbvconf->priority_queue_enable & (1 << i));
			qbvconf->priority_queue_enable &=  ~(1U << i);
			qbvconf->priority_queue_enable |= prioqen ? (1U << i) : 0;
		}
		printf("Priority Queue Enable : %02x\n", qbvconf->priority_queue_enable);

		for (i = 0; i < num_prio_queues; i++) {
			sprintf(prioq_str, "priority Queue %d prio", i);
			uint_input = unsigned_integer_input(prioq_str,
							    qbvconf->priority_queue_prios[i],
							    PCP_RANGE);
			qbvconf->priority_queue_prios[i] = uint_input;
		}
		uint_input = unsigned_integer_input("initial_gate_state",
						    qbvconf->initial_gate_state, UINT8_RANGE);
		qbvconf->initial_gate_state = uint_input;

		uint_input = unsigned_integer_input("control_list_length",
						    qbvconf->control_list_length,
						    CONTROL_LIST_LENGTH_RANGE);
		qbvconf->control_list_length = uint_input;

		remaining_cycle_time = qbvconf->cycle_time;
		for (i = 0; i < qbvconf->control_list_length; i++) {
			if (show_gcl_remaining_cycle_time && remaining_cycle_time < 0)
				colored_printf(RED, "Warning, cycle time has been utilized\n");

			sprintf(gcl_str, "GCL %02d State", i);
			uint_input = unsigned_integer_input(gcl_str,
							    qbvconf->gcle[i].gate_state,
							    UINT8_RANGE);
			qbvconf->gcle[i].gate_state = uint_input;

			if (show_gcl_remaining_cycle_time) {
				percentage_string(percent_str, remaining_cycle_time,
						  qbvconf->cycle_time);
				sprintf(gcl_str,
					"GCL %02d, Remaining cycle time : %u[%s]\nTime Interval",
					i, remaining_cycle_time, percent_str);
			} else {
				sprintf(gcl_str, "GCL%02d Time Interval", i);
			}
			uint_input = unsigned_integer_input(gcl_str, qbvconf->gcle[i].time_interval,
							    UINT32_RANGE);
			remaining_cycle_time -= uint_input;
			qbvconf->gcle[i].time_interval = uint_input;

			if (show_gcl_remaining_cycle_time && remaining_cycle_time > 0) {
				percentage_string(percent_str,
						  (qbvconf->cycle_time - remaining_cycle_time),
						  qbvconf->cycle_time);
				colored_printf(RED, "cycle time utilization %u/%u [%s]\n",
					       (qbvconf->cycle_time - remaining_cycle_time),
					       qbvconf->cycle_time, percent_str);
				displayProgressBar(PERCENTAGE
						   ((qbvconf->cycle_time - remaining_cycle_time),
						   qbvconf->cycle_time));
			}
		}
		if (show_gcl_remaining_cycle_time) {
			percentage_string(percent_str, remaining_cycle_time, qbvconf->cycle_time);
			colored_printf(YELLOW,
				       "Remaining cycle time (guard time) : %u[%s]\nTime Interval",
				       remaining_cycle_time, percent_str);
		}
	} else if (strcmp(tsnspec, "qbuconf") == 0) {
		qbuconf = (struct qbu_conf *)conf_mem;
		uint_input = y_n_input("pre_empt enable", qbuconf->pre_empt_en);
		qbuconf->pre_empt_en = uint_input;
		if (qbuconf->pre_empt_en) {
			uint_input = unsigned_integer_input("pre_empt size", qbuconf->pre_empt_size,
							    PREMPT_SIZE_RANGE);
			qbuconf->pre_empt_size = uint_input;
		}
	} else if (strcmp(tsnspec, "qciconf") == 0) {
		qciconf = (struct qci_conf *)conf_mem;
		uint_input = y_n_input("sa_check", qciconf->sa_check);
		qciconf->sa_check = uint_input;
		uint_input = y_n_input("da_check", qciconf->da_check);
		qciconf->da_check = uint_input;

		if (qciconf->da_check)
			mac_addr_input("Dest MAC Addr", qciconf->destination_mac_addr);

		if (qciconf->sa_check)
			mac_addr_input("Source MAC Addr", qciconf->source_mac_addr);

	} else if (strcmp(tsnspec, "misc_rx_port_id_conf") == 0) {
		miscrxportidconf = (struct misc_rx_port_id_conf *)conf_mem;
		uint_input = y_n_input("RX port ID Enable", miscrxportidconf->port_id_rx_check);
		miscrxportidconf->port_id_rx_check = uint_input;
		if (miscrxportidconf->port_id_rx_check)
			uint_input = unsigned_integer_input("Port ID for RX port",
							    miscrxportidconf->port_id_rx,
							    UINT16_RANGE);
		miscrxportidconf->port_id_rx = uint_input;

	} else if (strcmp(tsnspec, "misc_ptp_tx_prioq_conf") == 0) {
		miscptptxprioqconf = (struct misc_ptp_tx_prioq_conf *)conf_mem;
		uint_input = unsigned_integer_input("PRIOQ for PTP TX",
						    miscptptxprioqconf->ptp_tx_prioq, PCP_RANGE);
		miscptptxprioqconf->ptp_tx_prioq = uint_input;
	} else if (strcmp(tsnspec, "misc_length_deduct_byte_conf") == 0) {
		miscldbconf = (struct misc_length_deduct_byte_conf *)conf_mem;
		uint_input = unsigned_integer_input("MAC Packet Length deduct bytes",
						    miscldbconf->crc_deduct_len,
						    CRC_DEDUCT_LEN_RANGE);
		miscldbconf->crc_deduct_len = uint_input;
	}
}

static int microchip_tsn_set_config_json(__u64 devid, char *tsnspec, char *json_file_path)
{
	void *conf_mem;
	struct qbv_conf *qbvconf;
	struct qbu_conf *qbuconf;
	struct qci_conf *qciconf;
	struct misc_rx_port_id_conf *miscrxportidconf;
	struct misc_ptp_tx_prioq_conf *miscptptxprioqconf;
	struct misc_length_deduct_byte_conf *miscldbconf;
	struct microchip_tsn_device dev;
	cJSON *json;
	cJSON *jqciconf;
	cJSON *jqciconf_field;
	cJSON *jqbuconf;
	cJSON *jqbuconf_field;
	cJSON *jqbvconf;
	cJSON *jqbvconf_field;
	cJSON *jqbvconf_prioq_entry;
	cJSON *jqbvconf_prioq_entry_field;
	cJSON *jqbvconf_gcl_entry;
	cJSON *jqbvconf_gcl_entry_field;
	cJSON *jmiscrxportidconf = NULL;
	cJSON *jmiscrxportidconf_port_id_rx_check = NULL;
	cJSON *jmiscrxportidconf_port_id_rx = NULL;
	cJSON *jmiscptptxprioqconf = NULL;
	cJSON *jmiscptptxprioqconf_ptp_prioq = NULL;
	cJSON *jmiscldbconf = NULL;
	cJSON *jmiscldbconf_length_deduct_bytes = NULL;
	FILE *fp;
	char *buffer;
	int size, len;
	int ret, i;
	char *mac_addr_str;
	char hex_num[CHARS_FOR_HEX_BYTE + 1];

	fp = fopen(json_file_path, "r");
	if (!fp) {
		printf("Error: Unable to open the file.\n");
		return -ENOENT;
	}

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	buffer = malloc(size);
	if (!buffer)
		return -ENOMEM;

	len = fread(buffer, 1, size, fp);
	fclose(fp);
	if (len != size) {
		printf("Error: Unable to read the file.\n");
		return -ENOENT;
	}

	json = cJSON_Parse(buffer);

	if (!json) {
		const char *error_ptr = cJSON_GetErrorPtr();

		if (error_ptr)
			printf("Error: %s\n", error_ptr);

		cJSON_Delete(json);
		free(buffer);
		return -EFAULT;
	}

	conf_mem = malloc(MAX_TSN_CONFIG_SIZE);
	if (!conf_mem) {
		free(buffer);
		return -ENOMEM;
	}
	dev.tsn_dev_id = devid;

	jqciconf = cJSON_GetObjectItemCaseSensitive(json, JSON_KEY_QCI_CONF);
	if (jqciconf) {
		qciconf = (struct qci_conf *)conf_mem;
		jqciconf_field = cJSON_GetObjectItemCaseSensitive(jqciconf,
								  JSON_KEY_SOURCE_MAC_ADDRESS_CHECK
								  );
		printf("sa_check : %d\n", cJSON_IsTrue(jqciconf_field));
		qciconf->sa_check = cJSON_IsTrue(jqciconf_field);

		jqciconf_field = cJSON_GetObjectItemCaseSensitive(jqciconf,
								  JSON_KEY_SOURCE_MAC_ADDRESS);
		printf("sourceaddr: %s\n", jqciconf_field->valuestring);
		mac_addr_str = jqciconf_field->valuestring;
		for (i = 0; i < 6; i++) {
			hex_num[0] = mac_addr_str[3 * i];
			hex_num[1] = mac_addr_str[3 * i + 1];
			hex_num[2] = '\0';
			qciconf->source_mac_addr[i] = strtoul(hex_num, NULL, 16);
		}
		printf("source mac addr : %02x:%02x:%02x:%02x:%02x:%02x\n",
		       qciconf->source_mac_addr[0],
		       qciconf->source_mac_addr[1],
		       qciconf->source_mac_addr[2],
		       qciconf->source_mac_addr[3],
		       qciconf->source_mac_addr[4],
		       qciconf->source_mac_addr[5]);

		jqciconf_field = cJSON_GetObjectItemCaseSensitive
				(jqciconf,
				 JSON_KEY_DESTINATION_MAC_ADDRESS_CHECK);
		printf("da_check : %d\n", cJSON_IsTrue(jqciconf_field));
		qciconf->da_check = cJSON_IsTrue(jqciconf_field);

		jqciconf_field = cJSON_GetObjectItemCaseSensitive(jqciconf,
								  JSON_KEY_DESTINATION_MAC_ADDRESS);
		printf("destination: %s\n", jqciconf_field->valuestring);
		mac_addr_str = jqciconf_field->valuestring;
		for (i = 0; i < 6; i++) {
			hex_num[0] = mac_addr_str[3 * i];
			hex_num[1] = mac_addr_str[3 * i + 1];
			hex_num[2] = '\0';
			qciconf->destination_mac_addr[i] = strtoul(hex_num, NULL, 16);
		}
		printf("destination mac addr : %02x:%02x:%02x:%02x:%02x:%02x\n",
		       qciconf->destination_mac_addr[0],
		       qciconf->destination_mac_addr[1],
		       qciconf->destination_mac_addr[2],
		       qciconf->destination_mac_addr[3],
		       qciconf->destination_mac_addr[4],
		       qciconf->destination_mac_addr[5]);

		ret = microchip_tsn_device_set_qci_conf(&dev, qciconf);
		printf("ret = %d\n", ret);
	}

	jqbuconf = cJSON_GetObjectItemCaseSensitive(json, JSON_KEY_QBU_CONF);
	if (jqbuconf) {
		qbuconf = (struct qbu_conf *)conf_mem;

		jqbuconf_field = cJSON_GetObjectItemCaseSensitive(jqbuconf,
								  JSON_KEY_PRE_EMPTION_ENABLE);
		qbuconf->pre_empt_en = cJSON_IsTrue(jqbuconf_field);
		printf("pre_empt enable : %d\n", cJSON_IsTrue(jqbuconf_field));

		jqbuconf_field = cJSON_GetObjectItemCaseSensitive(jqbuconf,
								  JSON_KEY_PRE_EMPTION_SIZE);
		qbuconf->pre_empt_size = jqbuconf_field->valueint;
		printf("pre_empt size: %d\n", jqbuconf_field->valueint);

		ret = microchip_tsn_device_set_qbu_conf(&dev, qbuconf);
		printf("ret = %d\n", ret);
	}

	jqbvconf = cJSON_GetObjectItemCaseSensitive(json, JSON_KEY_QBV_CONF);
	if (jqbvconf) {
		qbvconf = (struct qbv_conf *)conf_mem;

		jqbvconf_field = cJSON_GetObjectItemCaseSensitive(jqbvconf,
								  JSON_KEY_BASE_TIME_SEC);
		printf("basetimesec: %d\n", jqbvconf_field->valueint);
		qbvconf->basetime_sec =  jqbvconf_field->valueint;

		jqbvconf_field = cJSON_GetObjectItemCaseSensitive(jqbvconf,
								  JSON_KEY_BASE_TIME_NSEC);
		printf("basetimensec: %d\n", jqbvconf_field->valueint);
		qbvconf->basetime_nsec =  jqbvconf_field->valueint;

		jqbvconf_field = cJSON_GetObjectItemCaseSensitive(jqbvconf,
								  JSON_KEY_BASE_TIME_ADJUST);
		printf("basetime adjust: %d\n", jqbvconf_field->valueint);
		qbvconf->basetime_adjust =  jqbvconf_field->valueint;

		jqbvconf_field = cJSON_GetObjectItemCaseSensitive(jqbvconf,
								  JSON_KEY_CYCLE_TIME);
		printf("cycletime: %d\n", jqbvconf_field->valueint);
		qbvconf->cycle_time =  jqbvconf_field->valueint;

		jqbvconf_field = cJSON_GetObjectItemCaseSensitive(jqbvconf,
								  JSON_KEY_GATE_CONTROL_LIST_COUNT);
		printf("gclcount: %d\n", jqbvconf_field->valueint);
		qbvconf->control_list_length =  jqbvconf_field->valueint;

		jqbvconf_field = cJSON_GetObjectItemCaseSensitive(jqbvconf,
								  JSON_KEY_INITIAL_GATE_STATE);
		printf("initgatestate: %d\n", jqbvconf_field->valueint);
		qbvconf->initial_gate_state =  jqbvconf_field->valueint;

		jqbvconf_field = cJSON_GetObjectItemCaseSensitive(jqbvconf,
								  JSON_KEY_PRIORITY_ENABLE);
		printf("priorityenable: %d\n", cJSON_IsTrue(jqbvconf_field));
		qbvconf->priority_enable = cJSON_IsTrue(jqbvconf_field);

		jqbvconf_field = cJSON_GetObjectItemCaseSensitive(jqbvconf, JSON_KEY_GATE_ENABLE);
		printf("gateenable: %d\n", cJSON_IsTrue(jqbvconf_field));
		qbvconf->gate_enable = cJSON_IsTrue(jqbvconf_field);

		jqbvconf_field = cJSON_GetObjectItemCaseSensitive(jqbvconf, JSON_KEY_PRIO_QUEUES);
		printf("prioqueues: %d isarray : %d\n", jqbvconf_field->type,
		       cJSON_IsArray(jqbvconf_field));

		cJSON_ArrayForEach(jqbvconf_prioq_entry, jqbvconf_field) {
			jqbvconf_prioq_entry_field = cJSON_GetObjectItemCaseSensitive
						     (jqbvconf_prioq_entry,
						      JSON_KEY_PRIO_QUEUE_NUM);
			printf("prioq : %d\n", jqbvconf_prioq_entry_field->valueint);
			i = jqbvconf_prioq_entry_field->valueint;

			if (num_prio_queues == 7 && i == 7) {
				printf("PRIOQ7 Not configurable, ignoring config for PRIOQ7\n");
				continue;
			}

			jqbvconf_prioq_entry_field = cJSON_GetObjectItemCaseSensitive
						     (jqbvconf_prioq_entry,
						      JSON_KEY_PRIO_QUEUE_ENABLE);
			printf("priority for index %d: %d\n", i,
			       cJSON_IsTrue(jqbvconf_prioq_entry_field));
			if (cJSON_IsTrue(jqbvconf_prioq_entry_field))
				qbvconf->priority_queue_enable |= (1 << i);
			else
				qbvconf->priority_queue_enable &= ~(1 << i);

			jqbvconf_prioq_entry_field = cJSON_GetObjectItemCaseSensitive
						     (jqbvconf_prioq_entry,
						      JSON_KEY_PRIO_QUEUE_PRIORITY);
			printf("priority for index %d : %d\n", i,
			       jqbvconf_prioq_entry_field->valueint);
			qbvconf->priority_queue_prios[i] = jqbvconf_prioq_entry_field->valueint;
		}

		jqbvconf_field = cJSON_GetObjectItemCaseSensitive(jqbvconf,
								  JSON_KEY_GATE_CONTROL_LIST);
		printf("gatecontrollist: %d isarray : %d\n", jqbvconf_field->type,
		       cJSON_IsArray(jqbvconf_field));

		cJSON_ArrayForEach(jqbvconf_gcl_entry, jqbvconf_field) {
			jqbvconf_gcl_entry_field = cJSON_GetObjectItemCaseSensitive
						   (jqbvconf_gcl_entry,
						    JSON_KEY_GATE_CONTROL_LIST_INDEX);
			printf("index : %d\n", jqbvconf_gcl_entry_field->valueint);
			i = jqbvconf_gcl_entry_field->valueint;

			jqbvconf_gcl_entry_field = cJSON_GetObjectItemCaseSensitive
						   (jqbvconf_gcl_entry,
						    JSON_KEY_GATE_CONTROL_LIST_GATE_STATE);
			if (cJSON_IsString(jqbvconf_gcl_entry_field)) {
				printf("gatestate for index %d : %d\n", i,
				       gcl_csv_to_uint(jqbvconf_gcl_entry_field->valuestring));
				qbvconf->gcle[i].gate_state =
				gcl_csv_to_uint(jqbvconf_gcl_entry_field->valuestring);
			} else {
				printf("gatestate for index %d : %d\n", i,
				       jqbvconf_gcl_entry_field->valueint);
				qbvconf->gcle[i].gate_state = jqbvconf_gcl_entry_field->valueint;
			}

			jqbvconf_gcl_entry_field = cJSON_GetObjectItemCaseSensitive
						   (jqbvconf_gcl_entry,
						    JSON_KEY_GATE_CONTROL_LIST_TIME_INTERVAL);
			printf("timeinterval for index %d : %d\n", i,
			       jqbvconf_gcl_entry_field->valueint);
			qbvconf->gcle[i].time_interval = jqbvconf_gcl_entry_field->valueint;
		}
		ret = microchip_tsn_device_set_qbv_conf(&dev, qbvconf);
		printf("ret = %d\n", ret);
	}

	jmiscrxportidconf = cJSON_GetObjectItemCaseSensitive(json, JSON_KEY_RX_CONF);
	if (jmiscrxportidconf) {
		miscrxportidconf = (struct misc_rx_port_id_conf *)conf_mem;
		jmiscrxportidconf_port_id_rx_check = cJSON_GetObjectItemCaseSensitive
						     (jmiscrxportidconf,
						      JSON_KEY_PORT_ID_RECV_CHECK);
		miscrxportidconf->port_id_rx_check = cJSON_IsTrue
						     (jmiscrxportidconf_port_id_rx_check);
		jmiscrxportidconf_port_id_rx = cJSON_GetObjectItemCaseSensitive
					       (jmiscrxportidconf,
						JSON_KEY_PORT_ID_RECV);
		miscrxportidconf->port_id_rx = jmiscrxportidconf_port_id_rx->valueint;
		ret = microchip_tsn_misc_set_rx_port_id(&dev, miscrxportidconf);
		printf("ret = %d\n", ret);
	}

	jmiscptptxprioqconf = cJSON_GetObjectItemCaseSensitive(json, JSON_KEY_PTP_CONF);
	if (jmiscptptxprioqconf) {
		miscptptxprioqconf = (struct  misc_ptp_tx_prioq_conf *)conf_mem;
		jmiscptptxprioqconf_ptp_prioq = cJSON_GetObjectItemCaseSensitive
						(jmiscptptxprioqconf,
						 JSON_KEY_PTP_XMIT_PRIORITY_QUEUE);
		miscptptxprioqconf->ptp_tx_prioq = jmiscptptxprioqconf_ptp_prioq->valueint;
		ret = microchip_tsn_misc_set_tx_ptp_prioq(&dev, miscptptxprioqconf);
		printf("ret = %d\n", ret);
	}

	jmiscldbconf = cJSON_GetObjectItemCaseSensitive(json, JSON_KEY_MAC_PACKET);
	if (jmiscldbconf) {
		miscldbconf = (struct misc_length_deduct_byte_conf *)conf_mem;
		jmiscldbconf_length_deduct_bytes = cJSON_GetObjectItemCaseSensitive
						   (jmiscldbconf, JSON_KEY_LENGTH_DEDUCT_BYTES);
		miscldbconf->crc_deduct_len = jmiscldbconf_length_deduct_bytes->valueint;
		ret = microchip_tsn_misc_set_length_deduct_byte(&dev, miscldbconf);
		printf("ret = %d\n", ret);
	}

	free(buffer);
	free(conf_mem);

	return 0;
}

static void microchip_tsn_set_config(__u64 devid, char *tsnspec)
{
	void *conf_mem;
	struct qbv_conf *qbvconf;
	struct qbu_conf *qbuconf;
	struct qci_conf *qciconf;
	struct misc_rx_port_id_conf *miscrxportidconf;
	struct misc_ptp_tx_prioq_conf *miscptptxprioqconf;
	struct misc_length_deduct_byte_conf *miscldbconf;
	struct microchip_tsn_device dev;
	int ret;

	printf("tsn set devid : %llx, tsnspec :%s\n", devid, tsnspec);

	conf_mem = malloc(MAX_TSN_CONFIG_SIZE);

	if (!conf_mem)
		return;

	qbvconf = (struct qbv_conf *)conf_mem;
	qbuconf = (struct qbu_conf *)conf_mem;
	qciconf = (struct qci_conf *)conf_mem;
	miscrxportidconf = (struct misc_rx_port_id_conf *)conf_mem;
	miscptptxprioqconf = (struct misc_ptp_tx_prioq_conf *)conf_mem;
	miscldbconf = (struct misc_length_deduct_byte_conf *)conf_mem;
	dev.tsn_dev_id = devid;

	if (edit_mode) {
		if (strcmp(tsnspec, "qbvconf") == 0)
			microchip_tsn_device_get_qbv_conf(&dev, qbvconf);
		if (strcmp(tsnspec, "qbuconf") == 0)
			microchip_tsn_device_get_qbu_conf(&dev, qbuconf);
		if (strcmp(tsnspec, "qciconf") == 0)
			microchip_tsn_device_get_qci_conf(&dev, qciconf);
		if (strcmp(tsnspec, "misc_ptp_tx_prioq_conf") == 0)
			microchip_tsn_misc_get_tx_ptp_prioq(&dev, miscptptxprioqconf);
		if (strcmp(tsnspec, "misc_rx_port_id_conf") == 0)
			microchip_tsn_misc_get_rx_port_id(&dev, miscrxportidconf);
		if (strcmp(tsnspec, "misc_length_deduct_byte_conf") == 0)
			microchip_tsn_misc_get_length_deduct_byte(&dev, miscldbconf);
	}

	input_tsn_conf(conf_mem, tsnspec);

	if (strcmp(tsnspec, "qbvconf") == 0) {
		ret = microchip_tsn_device_set_qbv_conf(&dev, qbvconf);
	} else if (strcmp(tsnspec, "qbuconf") == 0) {
		ret = microchip_tsn_device_set_qbu_conf(&dev, qbuconf);
	} else if (strcmp(tsnspec, "qciconf") == 0) {
		printf("qciconf da check : %d\n", qciconf->da_check);
		printf("qciconf sa check : %d\n", qciconf->sa_check);
		printf("qciconf src mac addr : %02x:%02x:%02x:%02x:%02x:%02x\n",
		       qciconf->source_mac_addr[0],
		       qciconf->source_mac_addr[1],
		       qciconf->source_mac_addr[2],
		       qciconf->source_mac_addr[3],
		       qciconf->source_mac_addr[4],
		       qciconf->source_mac_addr[5]);
		printf("qciconf dst mac addr : %02x:%02x:%02x:%02x:%02x:%02x\n",
		       qciconf->destination_mac_addr[0],
		       qciconf->destination_mac_addr[1],
		       qciconf->destination_mac_addr[2],
		       qciconf->destination_mac_addr[3],
		       qciconf->destination_mac_addr[4],
		       qciconf->destination_mac_addr[5]);
		ret = microchip_tsn_device_set_qci_conf(&dev, qciconf);
	} else if (strcmp(tsnspec, "misc_ptp_tx_prioq_conf") == 0) {
		ret = microchip_tsn_misc_set_tx_ptp_prioq(&dev, miscptptxprioqconf);

	} else if (strcmp(tsnspec, "misc_rx_port_id_conf") == 0) {
		ret = microchip_tsn_misc_set_rx_port_id(&dev, miscrxportidconf);
	} else if (strcmp(tsnspec, "misc_length_deduct_byte_conf") == 0) {
		ret = microchip_tsn_misc_set_length_deduct_byte(&dev, miscldbconf);
	}

	if (ret) {
		printf("tsnset: %s\n", strerror(ret));
		printf("ret = %d\n", ret);
	}

	free(conf_mem);
}

static int get_enable_config(char *tsnspec)
{
	int i;
	int ret = -EINVAL;

	if (strcmp(tsnspec, "all") == 0) {
		get_qbv_conf = 1;
		get_qbu_conf = 1;
		get_qci_conf = 1;
		get_misc_ptp_tx_prioq_conf = 1;
		get_misc_rx_port_id_conf = 1;
		get_misc_length_deduct_byte_conf = 1;
		get_all_conf = 1;
		return 0;
	}

	for (i = 0; i < NUM_TSN_CONFIGS; i++) {
		if (strcmp(tsnspec, tsn_configs[i]) == 0) {
			ret = 0;
			break;
		}
	}

	if (!ret) {
		switch (i) {
		case QBVCONF:
			get_qbv_conf = 1;
			break;
		case QBUCONF:
			get_qbu_conf = 1;
			break;
		case QCICONF:
			get_qci_conf = 1;
			break;
		case MISC_PTP_TX_PRIOQ:
			get_misc_ptp_tx_prioq_conf = 1;
			break;
		case MISC_RX_PORT_ID:
			get_misc_rx_port_id_conf = 1;
			break;
		case MISC_LENGTH_DEDUCT_BYTE:
			get_misc_length_deduct_byte_conf = 1;
			break;
		}
	}
	return ret;
}

static int parse_get_config_names(char *tsnspec)
{
	char *token;
	char *saveptr;
	int ret;

	token = strtok_r(tsnspec, ",", &saveptr);

	while (token) {
		ret = get_enable_config(token);
		if (ret) {
			printf("Invalid config name : %s\n", token);
			return ret;
		}
		token = strtok_r(NULL, ",", &saveptr);
	}

	return 0;
}

int main(int argc, char **argv)
{
	int c;
	int option_index;
	int show_devices = 0;
	int get = 0;
	int set = 0;
	int device = 0;
	__u64 devid;
	int json_format = 0;
	int export_to_config_file = 0;
	int import_from_config_file = 0;
	int display_selections = 0;
	char *tsnspec = NULL;
	char *json_file_path = NULL;

#if REGEX
	regex_compile();
#endif
	while (1) {
		c = getopt_long(argc, argv, "", long_options, &option_index);

		if (c == -1)
			break;

		switch (c) {
		case 0:
			if (option_index == SHOW_DEVICES) {
				show_devices = 1;
			} else if (option_index == DEVICE) {
				sscanf(optarg, "%llx", &devid);
				device = 1;
			} else if (option_index == GET) {
				tsnspec = optarg;
				get = 1;
			} else if (option_index == SET) {
				tsnspec = optarg;
				set = 1;
			} else if (option_index == JSON) {
				json_format = 1;
			} else if (option_index == CONF_FILE) {
				import_from_config_file = 1;
				json_file_path = optarg;
				set = 1; /* Importing is for set config */
			} else if (option_index == EXPORT_CONF_FILE) {
				export_to_config_file = 1;
				json_file_path = optarg;
			} else if (option_index == EDIT_MODE) {
				edit_mode = 1;
			} else if (option_index == DISPLAY_SELECTIONS) {
				display_selections = 1;
			} else if (option_index == JSON_SHOW_GCL_CSV) {
				json_show_gcl_csv = 1;
			} else if (option_index == SHOW_GCL_REMAINING_CYCLE_TIME) {
				show_gcl_remaining_cycle_time = 1;
			} else if (option_index == ENABLE_PRIOQ7_CONFIG) {
				num_prio_queues = 8; /* Allow all Prioq to config priority */
			}
			break;
		default:
			printf("getopt char code 0%o\n", c);
		}
	}

	if (display_selections) {
		printf("show_devices = %d\n", show_devices);
		printf("device = %d\n", device);
		printf("get = %d\n", get);
		printf("set = %d\n", set);
		printf("tsnspec = %s\n", tsnspec);
		printf("json_format = %d\n", json_format);
		printf("json_file_path = %s\n", json_file_path);
		printf("import_from_config_file = %d\n", import_from_config_file);
		printf("export_to_config_file = %d\n", export_to_config_file);
	}

	if (!show_devices && !device) {
		print_usage();
		return -EINVAL;
	}

	if (show_devices) {
		if (device || get || set) {
			print_usage();
			return -EINVAL;
		}
	}

	if (device) {
		if ((!get && !set) || (get && set)) {
			print_usage();
			return -EINVAL;
		}
	}

	if (export_to_config_file && import_from_config_file) {
		printf("Import and export are mutually exclusive\n");
		print_usage();
		return -EINVAL;
	}

	if (set && import_from_config_file) { /* Set needs to be done through json file */
		if (!json_file_path)
			return -EINVAL;
		if (edit_mode) {
			printf("Edit mode only for interactive input\n");
			return -EINVAL;
		}
	}

	if (get && edit_mode) {
		printf("Edit mode is for set only\n");
		return -EINVAL;
	}

	if (get && export_to_config_file)
		json_format = 1; /* export file only in json format */

	if (show_devices == 1) {
		microchip_tsn_display_devices();
	} else if (device) {
		if (get) {
			if (parse_get_config_names(tsnspec))
				return 0;
			if (get_all_conf) {
				microchip_tsn_get_config(devid, "all", json_format,
							 json_file_path);
			} else {
				if (get_qbv_conf) {
					microchip_tsn_get_config(devid, "qbvconf", json_format,
								 json_file_path);
					printf("\n\n");
				}
				if (get_qbu_conf) {
					microchip_tsn_get_config(devid, "qbuconf", json_format,
								 json_file_path);
					printf("\n\n");
				}
				if (get_qci_conf) {
					microchip_tsn_get_config(devid, "qciconf", json_format,
								 json_file_path);
					printf("\n\n");
				}
				if (get_misc_ptp_tx_prioq_conf) {
					microchip_tsn_get_config(devid, "misc_ptp_tx_prioq_conf",
								 json_format, json_file_path);
					printf("\n\n");
				}
				if (get_misc_rx_port_id_conf) {
					microchip_tsn_get_config(devid, "misc_rx_port_id_conf",
								 json_format, json_file_path);
					printf("\n\n");
				}
				if (get_misc_length_deduct_byte_conf) {
					microchip_tsn_get_config(devid,
								 "misc_length_deduct_byte_conf",
								 json_format, json_file_path);
					printf("\n\n");
				}
			}
		} else if (set) {
			if (json_file_path)
			/* tsnspec is not used as json file can contain more than one tsnspec */
				microchip_tsn_set_config_json(devid, tsnspec, json_file_path);
			else
				microchip_tsn_set_config(devid, tsnspec);
		}
	}

#if REGEX
	regfree(&regex_hexdec);
	regfree(&regex_dec);
#endif

	return 0;
}
