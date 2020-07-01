#include "conf.h"
#include "cmd.h"
#include <stddef.h>
#include <stdio.h>
#include "rcc.h"
#include "adc.h"
#include "regs.h"
#include "rcc-c.h"
#include "gpio-abs.h"
#include <string.h>

static cmd_handler_cb handlers[CMD_DEVICE_MAX];

extern unsigned sysclk_freq;
static unsigned get_adcclk_frequency()
{
	unsigned ahb_div = get_cfgr_hpre_divisor(rcc->cfgr.hpre);
	unsigned apb2_div = get_cfgr_ppre_divisor(rcc->cfgr.ppre2);
	unsigned adcclk_div = get_cfgr_adcpre_divisor(rcc->cfgr.adcpre);

	return sysclk_freq / (ahb_div * apb2_div * adcclk_div);
}

static unsigned get_clk_cycle_ns(unsigned clk_freq)
{
	unsigned ns = 1000000000;
	return (ns + clk_freq - 1) / clk_freq;
}

static unsigned get_corrected_mV(uint16_t d_refV, uint16_t d_measurement)
{
	unsigned vref_typical = 1200; // mV

	// Choose scaling factor such that
	//
	//	UINT16_MAX * c * vref_typical <= UINT32_MAX
	//
	// Although at most 12 bits coming from data register..
	//
	unsigned c = 50;

	unsigned scaledV = c * d_measurement * vref_typical / d_refV;
	unsigned mV = (scaledV + c/2) / c;
	return mV;
}

static int calculate_temperature_mC(int temp_mV)
{
	const int avg_slope = 4300; // 4300 µV/°C (spec: 4.3 mV/°C)
	const int V25 = 1430000; // 1.430.000 µV (spec: 1.43 V)

	int t_scaled = (100 * (V25 - temp_mV * 1000) + 50) / (avg_slope/1000) / 100;
	int mC = t_scaled + 25000;
	return mC;
}

static int adc_initialized = 0;

static void setup_adc_pins()
{
	struct gpio_conf lcfg = {
		.mode = GPIO_MODER_ANALOG,
		.otype = GPIO_OTYPER_PP,
		.ospeed = GPIO_OSPEEDR_LOW,
		.pupd = GPIO_PUPDR_NONE,
	};
	gpio_configure(PA4, &lcfg);
}

static int read_temperature()
{
	if (!adc_initialized) {
		setup_adc_pins();

		// Set clock divider
		rcc->cfgr.adcpre = RCC_ADCPRE_PCLK2_DIV_6;
		sleep_busy(10000); // not necessary to sleep here?

		rcc->apb2enr.adc1en = 1;

		// Initialize ADC
		adc1->cr2.tsvrefe = 1;
		adc1->cr2.adon = 1; // First time for initialize
		sleep_busy(10000); // Sleep a µs, Tstab

		// Calibrate
		adc1->cr2.cal = 1;
		while (adc1->cr2.cal);

		adc1->sr.eoc = 0; // Just to make sure
		adc_initialized = 1;
	}

	// Get the clock cycle
	unsigned adcclk_freq = get_adcclk_frequency();
	unsigned adcclk_cycle_ns = get_clk_cycle_ns(adcclk_freq);
	assert(adcclk_freq <= 14000000);

	adc1->sqr1.l = 0; // We'll read one at a time, 0 → 1 conv

	// Read temperature
	adc1->sqr3.sq1 = 16; // temperature channel
	// Max temperature readout time 17.1 µs
	adc1->smpr1.smp16 = get_adc_smp(17100, adcclk_cycle_ns);
	adc1->cr2.adon = 1; // Set again to read

	while (!adc1->sr.eoc);
	// Reading data register resets eoc
	uint16_t readout_temp = adc1->dr.data;

	// Read Vrefint
	adc1->sqr3.sq1 = 17;
	// Max readout time for Vref 17.1 µs
	adc1->smpr1.smp17 = get_adc_smp(17100, adcclk_cycle_ns);
	adc1->cr2.adon = 1;

	while (!adc1->sr.eoc);
	uint16_t readout_Vrefint = adc1->dr.data;

	// Calculate temperature
	unsigned temp_mV = get_corrected_mV(readout_Vrefint, readout_temp);
	unsigned temp_mC = calculate_temperature_mC(temp_mV);


	// Read moisture sensor
	adc1->sqr3.sq1 = 4; // PA4 channel
	// Assuming readout time 17.1 µs
	adc1->smpr2.smp4 = get_adc_smp(17100, adcclk_cycle_ns);
	adc1->cr2.adon = 1; // Set again to read

	while (!adc1->sr.eoc);
	// Reading data register resets eoc
	uint16_t readout_moisture = adc1->dr.data;
	unsigned moisture_mV = get_corrected_mV(readout_Vrefint, readout_moisture);

	// Print out results
	char test[300];
	snprintf(test, sizeof(test), "%i %u %u\r\n",
			readout_Vrefint, temp_mC, moisture_mV);
	cmd_reply(CMD_DEVICE_USB, test);
	return temp_mC;
}

enum active_read_speed {
	UPDATE_OFF,
	UPDATE_SLOW,
	UPDATE_FAST,
};
static enum active_read_speed  active_read = UPDATE_OFF;

static void measure_cmd_cb(enum cmd_device_enum d, struct cmd_line *n)
{
	int exit = 0;

	for (int i = 0; i < n->length; i++) {
		if (n->line[i] != 3) // ^C
			continue;
		exit=1;
		break;
	}

	if (!exit)
		return;

	cmd_reply(d, "interrupt received..\r\n");
	cmd_handler_set(d, handlers[d]);
	handlers[d] = NULL;
	active_read = UPDATE_OFF;
}

void end()
{
	assert(0);
}

void adc_update(int i)
{
	if (!active_read)
		return;

	if (active_read == UPDATE_FAST) {
		read_temperature();
	} else if (i % 1000 == 0) { // about every 1.6s
		read_temperature();
	}
}

void measure_cli_call(enum cmd_device_enum d, char *c, int l)
{
	if (l == 5 && !memcmp(c, " fast", 5))
		active_read = UPDATE_FAST;
	else
		active_read = UPDATE_SLOW;
	handlers[d] = cmd_handler_set(d, measure_cmd_cb);
	cmd_reply(CMD_DEVICE_USB, "\r\nVref mC moisture_mV\r\n");
	//readout_Vrefint, readout_temp, temp_mV, temp_mC, moisture_mV);
}

