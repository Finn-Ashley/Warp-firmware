#include <stdint.h>
#include <math.h>
#include <complex.h>

/*
 *	config.h needs to come first
 */
#include "config.h"

#include "fsl_spi_master_driver.h"
#include "fsl_port_hal.h"

#include "SEGGER_RTT.h"
#include "gpio_pins.h"
#include "warp.h"
#include "devSSD1331.h"
#include "devADC.h"
#include "fft.h"

#define SPLASH_SCREEN 1


volatile uint8_t	inBuffer[1];
volatile uint8_t	payloadBytes[1];
float stats[6][2];
uint8_t nBins = 6;
uint8_t bin_width;

/*
 *	Override Warp firmware's use of these pins and define new aliases.
 */
enum
{
	kSSD1331PinMOSI		= GPIO_MAKE_PIN(HW_GPIOA, 8),
	kSSD1331PinSCK		= GPIO_MAKE_PIN(HW_GPIOA, 9),
	kSSD1331PinCSn		= GPIO_MAKE_PIN(HW_GPIOB, 11),
	kSSD1331PinDC		= GPIO_MAKE_PIN(HW_GPIOA, 12),
	kSSD1331PinRST		= GPIO_MAKE_PIN(HW_GPIOB, 0),
};

static int
writeCommand(uint8_t commandByte)
{
	spi_status_t status;

	/*
	 *	Drive /CS low.
	 *
	 *	Make sure there is a high-to-low transition by first driving high, delay, then drive low.
	 */
	GPIO_DRV_ClearPinOutput(kSSD1331PinCSn);

	/*
	 *	Drive DC low (command).
	 */
	GPIO_DRV_ClearPinOutput(kSSD1331PinDC);

	payloadBytes[0] = commandByte;
	status = SPI_DRV_MasterTransferBlocking(0	/* master instance */,
					NULL		/* spi_master_user_config_t */,
					(const uint8_t * restrict)&payloadBytes[0],
					(uint8_t * restrict)&inBuffer[0],
					1		/* transfer size */,
					1000		/* timeout in microseconds (unlike I2C which is ms) */);

	/*
	 *	Drive /CS high
	 */
	GPIO_DRV_SetPinOutput(kSSD1331PinCSn);

	return status;
}

int
devSSD1331init(void)
{
	/*
	 *	Override Warp firmware's use of these pins.
	 *
	 *	Re-configure SPI to be on PTA8 and PTA9 for MOSI and SCK respectively.
	 */
	PORT_HAL_SetMuxMode(PORTA_BASE, 8u, kPortMuxAlt3);
	PORT_HAL_SetMuxMode(PORTA_BASE, 9u, kPortMuxAlt3);

	warpEnableSPIpins();

	/*
	 *	Override Warp firmware's use of these pins.
	 *
	 *	Reconfigure to use as GPIO.
	 */
	PORT_HAL_SetMuxMode(PORTB_BASE, 11u, kPortMuxAsGpio);
	PORT_HAL_SetMuxMode(PORTA_BASE, 12u, kPortMuxAsGpio);
	PORT_HAL_SetMuxMode(PORTB_BASE, 0u, kPortMuxAsGpio);


	/*
	 *	RST high->low->high.
	 */
	GPIO_DRV_SetPinOutput(kSSD1331PinRST);
	OSA_TimeDelay(100);
	GPIO_DRV_ClearPinOutput(kSSD1331PinRST);
	OSA_TimeDelay(100);
	GPIO_DRV_SetPinOutput(kSSD1331PinRST);
	OSA_TimeDelay(100);

	/*
	 *	Initialization sequence, borrowed from https://github.com/adafruit/Adafruit-SSD1331-OLED-Driver-Library-for-Arduino
	 */
	writeCommand(kSSD1331CommandDISPLAYOFF);	// 0xAE
	writeCommand(kSSD1331CommandSETREMAP);		// 0xA0
	writeCommand(0x72);				// RGB Color
	writeCommand(kSSD1331CommandSTARTLINE);		// 0xA1
	writeCommand(0x0);
	writeCommand(kSSD1331CommandDISPLAYOFFSET);	// 0xA2
	writeCommand(0x0);
	writeCommand(kSSD1331CommandNORMALDISPLAY);	// 0xA4
	writeCommand(kSSD1331CommandSETMULTIPLEX);	// 0xA8
	writeCommand(0x3F);				// 0x3F 1/64 duty
	writeCommand(kSSD1331CommandSETMASTER);		// 0xAD
	writeCommand(0x8E);
	writeCommand(kSSD1331CommandPOWERMODE);		// 0xB0
	writeCommand(0x0B);
	writeCommand(kSSD1331CommandPRECHARGE);		// 0xB1
	writeCommand(0x31);
	writeCommand(kSSD1331CommandCLOCKDIV);		// 0xB3
	writeCommand(0xF0);				// 7:4 = Oscillator Frequency, 3:0 = CLK Div Ratio (A[3:0]+1 = 1..16)
	writeCommand(kSSD1331CommandPRECHARGEA);	// 0x8A
	writeCommand(0x64);
	writeCommand(kSSD1331CommandPRECHARGEB);	// 0x8B
	writeCommand(0x78);
	writeCommand(kSSD1331CommandPRECHARGEA);	// 0x8C
	writeCommand(0x64);
	writeCommand(kSSD1331CommandPRECHARGELEVEL);	// 0xBB
	writeCommand(0x3A);
	writeCommand(kSSD1331CommandVCOMH);		// 0xBE
	writeCommand(0x3E);
	writeCommand(kSSD1331CommandMASTERCURRENT);	// 0x87
	writeCommand(0x06);
	writeCommand(kSSD1331CommandCONTRASTA);		// 0x81
	writeCommand(0x91);
	writeCommand(kSSD1331CommandCONTRASTB);		// 0x82
	writeCommand(0x50);
	writeCommand(kSSD1331CommandCONTRASTC);		// 0x83
	writeCommand(0x7D);
	writeCommand(kSSD1331CommandDISPLAYON);		// Turn on oled panel

	/*
	 *	To use fill commands, you will have to issue a command to the display to enable them. See the manual.
	 */
	writeCommand(kSSD1331CommandFILL);
	writeCommand(0x01);

	/*
	 *	Clear Screen
	 */
	writeCommand(kSSD1331CommandCLEAR);
	writeCommand(0x00);
	writeCommand(0x00);
	writeCommand(0x5F);
	writeCommand(0x3F);


	/*
	 *	Any post-initialization drawing commands go here.
	 */
	bin_width = 0x5F / nBins;

	#if SPLASH_SCREEN
		// draw rectangle
		writeCommand(kSSD1331CommandDRAWRECT);

		// set start and end row and column
		writeCommand(0x00);
		writeCommand(0x00);
		writeCommand(0x5F); // col max
		writeCommand(0x3F); // row max

		// set outline colour
		writeCommand(0x00);
		writeCommand(0xFF);
		writeCommand(0x00);

		// set fill colour
		writeCommand(0xFF);
		writeCommand(0x00);
		writeCommand(0x00);
	#endif

	return 0;
}

void chart_calibration(int *adc_readings, float complex *fft_output, float *frequency_powers){
	// temporary array to store the two reads for each freq. bin
	float tmp[2] = {0,0};

	// take two readings for the each place so that we can run continously after this
	// gives values to comapre against in the draw_frequency_chart function
	for(uint8_t i = 0; i < nBins; i++){
		ADC_read_set(0);
		fft(adc_readings, fft_output, NUMBER_OF_STORED_READINGS);
		process_powers(fft_output, frequency_powers);
		tmp[0] = frequency_powers[i];

		ADC_read_set(0);
		fft(adc_readings, fft_output, NUMBER_OF_STORED_READINGS);
		process_powers(fft_output, frequency_powers);
		tmp[1] = frequency_powers[i];

		stats[i][0] = min(tmp[0], tmp[1]); // min value for ith bin
		stats[i][1] = max(tmp[0], tmp[1]); // max value for ith bin
	}
}

void draw_frequency_bar(uint8_t start, uint8_t end, uint8_t height, uint8_t colour){

	writeCommand(kSSD1331CommandDRAWRECT);
	writeCommand(start); // start col
	writeCommand(0x00); // start row
	writeCommand(end); // end col
	writeCommand(height); // end row

	// set outline colour
	writeCommand(0xFF);
	writeCommand(0xFF);
	writeCommand(0xFF);

	// set fill colour - could pass in a struct containing all values, but
	// would be memory intensive for all bars and we are close to stack limit.
	// Sacrifice MCU cycles instead.
	switch(colour){

		// red
		case RED:
			writeCommand(0xFF);
			writeCommand(0x00);
			writeCommand(0x00);
			break;

		// green
		case GREEN:
			writeCommand(0x00);
			writeCommand(0xFF);
			writeCommand(0x00);
			break;

		// blue
		case BLUE:
			writeCommand(0x00);
			writeCommand(0x00);
			writeCommand(0xFF);
			break;

		// purple
		case PURPLE:
			writeCommand(0xA0);
			writeCommand(0x20);
			writeCommand(0xF0);
			break;

		// yellow
		case YELLOW:
			writeCommand(0xFF);
			writeCommand(0xFF);
			writeCommand(0x00);
			break;

		// orange
		case ORANGE:
			writeCommand(0xFF);
			writeCommand(0xA5);
			writeCommand(0x00);
			break;
	}
	
}

void draw_frequency_chart(float *bar_heights){

	// innit vars
	uint8_t normalised_height, start, end;
	
	// clear screen before redrawing
	writeCommand(kSSD1331CommandCLEAR);
	writeCommand(0x00);
	writeCommand(0x00);
	writeCommand(0x5F);
	writeCommand(0x3F);

	// draw chart - loop through each bar that needs to be created
	for(uint8_t i = 0; i < nBins; i++){

		/*  check if new min / max found with decays - these stop
			one erroneous large value from permenatly affecting the
			scaling for a chart so that it looks small, with it
			decaying to lower bounds if no other large values. */

        if (bar_heights[i] > stats[i][1]){

			// new max found - replace and upscale the minimum
        	stats[i][1] = bar_heights[i];
			stats[i][0] *= 1.1;
		}
		else if (bar_heights[i] < stats[i][0]){
			// new min found - replace and downscale the max
			stats[i][0] = bar_heights[i];
			stats[i][1] *= 0.9;
		}
		else{
			// value sits in middle - scales current bounds inwards
			stats[i][0] *= 1.1;
			stats[i][1] *= 0.8;
		}

		// scale bars to fit on display and have interesting dynamics
		normalised_height = (bar_heights[i] - stats[i][0])/(stats[i][1]-stats[i][0])*(0x3F);

		// calculate bar 'coordinates'
		start = i * bin_width;
		end = start + bin_width;

		// call drawing routine
		draw_frequency_bar(start, end, normalised_height, i);
	}
 }