// SPDX-License-Identifier: MIT
/*
 * @file japll-pi.c
 * @brief adjust ppb using PI controller
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <stdbool.h>
#include <limits.h>
#include <signal.h>

#include <sys/stat.h>
#include <sys/mman.h>

#include "print.h"

/******************************************************************
 * Configuration File parameters and PI Controller related
 ******************************************************************/
#define INITIAL_PPB_SETTLE_COUNT      1
#define APPLY_PPB_STEP_SIZE           1

struct g_pi_conf_t {
	bool   is_read;             /* indicate whether to read configuration file or not */
	int    pi_enable;           /* indicate use of MCHP pi controller*/
	double k_proportional;      /* constant for proportion */
	double k_integral;          /* constant for integration */
	double set_point;           /* target value to achieve */
	double delta_time;          /* time instance for integral : 1 / ptp packets per sec */
	float  board_ref_clk_freq;  /* reference clock frequency to JAPLL */
	char   eth_interface[10];   /* ethernet driver interface for input to ptp4l command */
	char   ptp4l_config[50];    /* filename along with path for input to ptp4l command */
};

struct g_pi_conf_t g_pi_conf = {
	.is_read = false,
	.pi_enable = false,
	.k_proportional = 0.0,
	.k_integral = 0.0,
	.set_point = 0.0,
	.delta_time = 0.0,
	.board_ref_clk_freq = 0.0,
	.eth_interface[0] = '\0',
	.ptp4l_config[0] = '\0'
};

/******************************************************************
 * JAPLL configuration parameters and related
 ******************************************************************/
#define UIO_0_DEVICE                   "/dev/uio0"
#define MMAP_SIZE                      0x100

#define JAPLL_9_INT_PRESET             0xE         /* Reg Offset:0x38 / sizeof(int) */
#define JAPLL_8_FRAC_PRESET            0xD         /* Reg Offset:0x34 / sizeof(int) */
#define JAPLL_8_PRESET_DISABLE         0xFEFFFFFF  /* Disabling TXPLL_JA_PRESET_EN (bit:24) Reg */
#define JAPLL_8_HOLD_ENABLE            0x02000000  /* Enabling TXPLL_JA_HOLD_EN (bit:25) Reg */
/* Enabling TXPLL_JA_HOLD (bit:25) and TXPLL_JA_PRESET_EN (bit:24) Regs */
#define JAPLL_8_HOLD_PRESET_ENABLE     0x03000000
/* To read only 24-29 bits from register */
#define TXPLL_REF_DIV(x)               ((x & 0x3F000000) >> 24)
#define TXPLL_DIV_2                    0x5         /* Reg Offset : 0x14 / sizeof(int) */
#define FRAC_DIVIDER                   1000000000.0
#define TSU_CLK_FREQ                   125.0
#define BIT_CLK_FREQ_MULT              (10 * 2)
#define FRAC_TO_JAPLL_FRAC(x)          ((x) * pow(2, JAPLL_FRAC_WIDTH_BITS))
#define JAPLL_INT_WIDTH_BITS           12
#define JAPLL_FRAC_WIDTH_BITS          24

struct g_japll_t {
	int      uioFd_0;             /* to store file descriptor for UIO device */
	uint32_t *mem_ptr0;           /* to map JAPLL UIO device memory */
	int      integer;             /* to store integer part of PPB value */
	int      fraction;            /* to store fraction part of PPB value */
	bool     is_int_written;      /* ensure integer has been written earlier */
	bool     is_frac_written;     /* ensure fraction has been written earlier */
	double   japll_preset_ppb_0;  /* indicate preset value of JAPLL for PPB value 0 */
	int      japll_wr_enable;     /* indicate use of JAPLL functionality */
};

struct g_japll_t g_japll = {
	.uioFd_0 = -1,
	.mem_ptr0 = NULL,
	.integer = 0,
	.fraction = 0,
	.is_int_written = false,
	.is_frac_written = false,
	.japll_preset_ppb_0 = 0.0,
	.japll_wr_enable = 1
};

/*
 *  Function to fetch parameters from configuration file
 */
int get_pi_configuration(void)
{
	FILE *fp;
	char word[50];

	fp = fopen("/opt/microchip/japll-pi-controller/configs/japll-pi.cfg", "r");
	if (!fp) {
		pr_err("Cannot open japll-pi.cfg: %m");
		return errno;
	}

	while (fscanf(fp, "%49s", word) != EOF) {
		if (!(strcmp(word, "k_proportional"))) {
			fscanf(fp, "%s", word);
			g_pi_conf.k_proportional = strtod(word, NULL);
			pr_info("k_proportional : %lf", g_pi_conf.k_proportional);
		} else if (!(strcmp(word, "k_integral"))) {
			fscanf(fp, "%49s", word);
			g_pi_conf.k_integral = strtod(word, NULL);
			pr_info("k_integral : %lf", g_pi_conf.k_integral);
		} else if (!(strcmp(word, "delta_time"))) {
			fscanf(fp, "%49s", word);
			g_pi_conf.delta_time = strtod(word, NULL);
			pr_info("delta_time : %lf", g_pi_conf.delta_time);
		} else if (!(strcmp(word, "set_point"))) {
			fscanf(fp, "%49s", word);
			g_pi_conf.set_point = strtod(word, NULL);
			pr_info("set_point : %lf", g_pi_conf.set_point);
		} else if (!(strcmp(word, "pi_enable"))) {
			fscanf(fp, "%49s", word);
			g_pi_conf.pi_enable = atoi(word);
			pr_info("pi_enable : %d", g_pi_conf.pi_enable);
		} else if (!(strcmp(word, "japll_wr_enable"))) {
			fscanf(fp, "%49s", word);
			g_japll.japll_wr_enable = atoi(word);
			pr_info("japll_wr_enable : %d", g_japll.japll_wr_enable);
		} else if (!(strcmp(word, "board_ref_clk_freq"))) {
			fscanf(fp, "%49s", word);
			g_pi_conf.board_ref_clk_freq = atoi(word);
			pr_info("board_ref_clk_freq : %f", g_pi_conf.board_ref_clk_freq);
		} else if (!(strcmp(word, "eth_interface"))) {
			fscanf(fp, "%49s", word);
			strncat(g_pi_conf.eth_interface, word, sizeof(g_pi_conf.eth_interface) - 1);
			pr_info("eth_interface : %s", g_pi_conf.eth_interface);
		} else if (!(strcmp(word, "ptp4l_config"))) {
			fscanf(fp, "%49s", word);
			strncat(g_pi_conf.ptp4l_config, word, sizeof(g_pi_conf.ptp4l_config) - 1);
			pr_info("ptp4l_config : %s", g_pi_conf.ptp4l_config);
		}
	}

	if (fclose(fp) != 0) {
		pr_err("Error closing japll-pi.cfg: %m");
		return errno;
	}

	g_pi_conf.is_read = true;

	return 0;
}

/*
 * calculate Proportional term
 */
double p_cal(double error)
{
	double p_val;

	p_val = g_pi_conf.k_proportional * error;

	return p_val;
}

/*
 * calculate Integral term
 */
double i_cal(double error)
{
	static double integral; // Integral term
	double i_val;

	i_val = integral + (error * g_pi_conf.k_integral * g_pi_conf.delta_time);
	integral = i_val;

	return i_val;
}

/**
 * PI controller function
 */
double pi_controller(double input_val)
{
	static double input_err;
	double output_val = 0.0;

	/* Calculate error signal */
	input_err = g_pi_conf.set_point - input_val;

	/* Calculate controller output */
	output_val = p_cal(input_err) + i_cal(input_err);

	/* Let the below be printed both in logcat */
	pr_debug("ip_val : %lf op_val : %lf\n", input_val, output_val);

	return output_val;
}

/*
 * Called per packet or per PPB basis.
 */
void calculate_japll_presets(double ppb)
{
	double japll_preset_total = 0.0;
	int japll_preset_int = 0;

	/* Step-1: Calculate total preset */
	japll_preset_total = (g_japll.japll_preset_ppb_0) * ((ppb / FRAC_DIVIDER) + 1);

	/* Step-2: Calculate int preset */
	/* int preset is obtained through trucation of the total preset */
	japll_preset_int = japll_preset_total;

	if (g_japll.integer != japll_preset_int) {
		g_japll.integer = japll_preset_int;
		pr_info("New g_japll.integer = 0x%x", g_japll.integer);
		g_japll.is_int_written = false;
	}

	/* Step-3: Calculate frac preset */
	/* Typically frac preset will change every time */
	g_japll.fraction = FRAC_TO_JAPLL_FRAC(japll_preset_total - g_japll.integer);
	g_japll.is_frac_written = false;

	pr_debug("ppb = %.8f, japll_preset_total = %.8f, int = %d, frac = 0x%x",
			ppb, japll_preset_total, g_japll.integer, g_japll.fraction);
}

/*
 * Should be called only once during the application's lifetime.
 */
void calculate_japll_initial_presets(void)
{
	int japll_ref_div = 0;

	/* Step-1: Calculate total preset for ppb_0 */
	/* read TXPLL_REF_DIV register */
	japll_ref_div = *(g_japll.mem_ptr0 + TXPLL_DIV_2);

	/* JAPLL Base Preset value is the present value (integer + frac) for PPB = 0 */
	g_japll.japll_preset_ppb_0 =
		(TSU_CLK_FREQ * BIT_CLK_FREQ_MULT * TXPLL_REF_DIV(japll_ref_div))
		/ g_pi_conf.board_ref_clk_freq;
	pr_info("step 1: japll_preset_total_ppb_0 = %.8f", g_japll.japll_preset_ppb_0);

	/* Step-2: Calculate int preset for ppb_0 */
	/* int preset is obtained through trucation of the total preset */
	g_japll.integer = g_japll.japll_preset_ppb_0;
	pr_info("step 2: japll_preset_int = 0x%x", g_japll.integer);

	/* Step-3: Calculate frac preset for ppb_0 */
	/* Remaining part is frac preset, in the order of 24 bits */
	g_japll.fraction = FRAC_TO_JAPLL_FRAC(g_japll.japll_preset_ppb_0 - g_japll.integer);
	pr_info("step 3: japll_preset_frac = 0x%x", g_japll.fraction);
}

int configure_japll(void)
{
	if (!(g_japll.is_int_written)) {
		if (g_japll.integer >> JAPLL_INT_WIDTH_BITS)
			pr_warning("g_japll.integer: 0x%x exceeding %d bits, msb's will be discarded!",
					g_japll.integer, JAPLL_INT_WIDTH_BITS);

		*(g_japll.mem_ptr0 + JAPLL_9_INT_PRESET) = g_japll.integer;
		pr_debug("[Write][JAPLL_9_INT_PRESET Reg]: 0x%x", g_japll.integer);

		g_japll.is_int_written = true;
	}

	if (!(g_japll.is_frac_written)) {
		if (g_japll.fraction >> JAPLL_FRAC_WIDTH_BITS) {
			pr_warning("g_japll.fraction: 0x%x exceeding %d bits, msb's will be discarded!",
					g_japll.fraction, JAPLL_FRAC_WIDTH_BITS);
		}

		/* Step 1: Ensure TXPLL_JA_PRESET_EN is disabled, TXPLL_JA_HOLD is enabled,
		 * then copy the fractional value.
		 */
		*(g_japll.mem_ptr0 + JAPLL_8_FRAC_PRESET) =
			((g_japll.fraction & JAPLL_8_PRESET_DISABLE) | JAPLL_8_HOLD_ENABLE);
		pr_debug("[Write][JAPLL_8_FRAC_PRESET Reg] Preset Disabled : 0x%x",
				(g_japll.fraction & JAPLL_8_PRESET_DISABLE));
		pr_debug("[Write][JAPLL_8_FRAC_PRESET Reg] Preset Disabled, Hold Enabled: 0x%x",
				((g_japll.fraction & JAPLL_8_PRESET_DISABLE)
				 | JAPLL_8_HOLD_ENABLE));

		/* Step 2: Enable both TXPLL_JA_HOLD and TXPLL_JA_PRESET_EN,
		 * along with retaining the fractional value,
		 */
		*(g_japll.mem_ptr0 + JAPLL_8_FRAC_PRESET) =
			(g_japll.fraction | JAPLL_8_HOLD_PRESET_ENABLE);
		pr_debug("[Write][JAPLL_8_FRAC_PRESET Reg] Hold and Preset Enabled: 0x%x",
				(g_japll.fraction | JAPLL_8_HOLD_PRESET_ENABLE));

		g_japll.is_frac_written = true;
	}

	return EXIT_SUCCESS;
}

int japll_main(double ppb)
{
	int status = 0;
	static int iterations;
	double n_freq;

	pr_debug("Input PPB = %f\n", ppb);

	if (iterations < INITIAL_PPB_SETTLE_COUNT) {
		/* Initial wait to reach some stability in PPB. The value of
		 * INITIAL_PPB_SETTLE_COUNT is chosen based on tuning.
		 */
		iterations++;

	} else if (iterations == INITIAL_PPB_SETTLE_COUNT) {
		/* Once PPB is stable, we take it as input for JAPLL fraction PRESET.
		 * The integer and frac parts were calculated once in the beginning.
		 * The integer part is written to JAPLL_9_INT_PRESET register
		 * while the fraction to the JAPLL_8_INT_PRESET register.
		 */
		if (g_japll.japll_wr_enable) {
			calculate_japll_presets(ppb);
			status = configure_japll();
			if (status)
				pr_err("configure_japll failed: %m\n");
		}

		iterations++;

	} else if (iterations > INITIAL_PPB_SETTLE_COUNT) {

		if (!(iterations % APPLY_PPB_STEP_SIZE)) {
			if (g_pi_conf.pi_enable)
				n_freq = pi_controller(ppb);
			else
				n_freq = ppb;

			if (g_japll.japll_wr_enable) {
				calculate_japll_presets(n_freq);
				status = configure_japll();
				if (status)
					pr_err("configure_japll failed: %m");
			}
		}

		if (++iterations == INT_MAX)
			iterations = INITIAL_PPB_SETTLE_COUNT + 1;
	}

	return status;
}

int memory_map(void)
{
	g_japll.uioFd_0 = open(UIO_0_DEVICE, O_RDWR);
	if (g_japll.uioFd_0 < 0) {
		pr_err("%s device open failed: %m", UIO_0_DEVICE);
		return errno;
	}

	g_japll.mem_ptr0 = mmap(NULL, MMAP_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED,
			g_japll.uioFd_0, 0);
	if (g_japll.mem_ptr0 == MAP_FAILED) {
		pr_err("mmap failed: %m");
		close(g_japll.uioFd_0);
		return errno;
	}

	return EXIT_SUCCESS;
}

void memory_unmap(void)
{
	if (g_japll.uioFd_0 >= 0)
		close(g_japll.uioFd_0);
	if (g_japll.mem_ptr0 != MAP_FAILED) {
		munmap((void *)g_japll.mem_ptr0, MMAP_SIZE);
		pr_info("memory unmapped successfully");
	}
}

void handle_sigint(int sig)
{
	memory_unmap();
	pr_info("******* exit with ctrl+C *******");
	exit(EXIT_SUCCESS);
}

int main(void)
{
	FILE *fp;
	char  p_buf[128] = {};
	char *freq_str = NULL;
	char *end_ptr = NULL;
	char *col_data = NULL;
	double freq = 0.0;
	int status = 0;
	char command[100] = {};

	signal(SIGINT, handle_sigint);

	if (!(g_pi_conf.is_read)) {
		status = get_pi_configuration();
		if (status) {
			pr_err("get_pi_configuration failed: %m");
			return errno;
		}
	}

	status = memory_map();
	if (status)
		g_japll.japll_wr_enable = 0;
	else {
		calculate_japll_initial_presets();
		/* NOTE : Depending on whether the Fabric design initializes JAPLL (INT and FRAC)
		 * Preset values, the respective flags i.e., g_japll.is_int_written and
		 * g_japll.is_frac_written can be set here, before calling configure_japll().
		 */
	}

	/* Launch ptp4l */
	snprintf(command, sizeof(command), "ptp4l -i %s -m -s -f %s",
			g_pi_conf.eth_interface, g_pi_conf.ptp4l_config);
	pr_info("Going to launch: %s", command);
	fp = popen(command, "r");
	if (fp == NULL) {
		pr_err("Failed to launch ptp4l\n");
		memory_unmap();
		exit(EXIT_FAILURE);
	}

	/* Read the output a line at a time - output it. */
	while (fgets(p_buf, sizeof(p_buf), fp) != NULL) {
		pr_info("p_buf = %s", p_buf);

		/*NOTE: If required, an additional pre-check may be added to discard
		 * false positives in the beginning itself, for e.g.,
		 * by searching 'master offset'.
		 */

		freq_str = strstr(p_buf, "freq");
		if (freq_str == NULL)
			continue;

		/* first token = freq : ignore */
		col_data = strtok(freq_str, " ");
		pr_debug("first token = %s", col_data);

		/* Now collect the freq value. */
		col_data = strtok(NULL, " ");
		pr_debug("second token = %s", col_data);

		errno = 0;
		freq = strtol(col_data, &end_ptr, 10);
		if (errno != 0) {
			pr_err("strtol error: %d", errno);
			memory_unmap();
			exit(EXIT_FAILURE);
		}

		/* This will discard the false positives i.e, lines
		 * which contain 'freq' string, but not the expected log.
		 */
		if (end_ptr == col_data) {
			pr_err("No digits were found!\n");
			continue;
		}

		pr_debug("freq : %lf", freq);

		japll_main(freq);
	}

	return EXIT_SUCCESS;
}
