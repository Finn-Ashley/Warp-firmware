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


volatile uint8_t	inBuffer[1];
volatile uint8_t	payloadBytes[1];
int bin_width;

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
	warpPrint("Clearing screen...\n");
	writeCommand(kSSD1331CommandCLEAR);
	writeCommand(0x00);
	writeCommand(0x00);
	writeCommand(0x5F);
	writeCommand(0x3F);



	/*
	 *	Any post-initialization drawing commands go here.
	 */
	
	warpPrint("Drawing red background...\n");
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

	// draw_frequency_bar(0, 5, 60, 0xFF);

	/*

	double complex fft_output[NUMBER_OF_STORED_READINGS];
	double frequency_powers[NUMBER_OF_STORED_READINGS];

	chart_init();

    while(true){
		update_adc_data();

		warpPrint("Performing FFT...\n");
		fft(adc_readings, fft_output, NUMBER_OF_STORED_READINGS);
		warpPrint("FFT done:\n");

        for(int i = 0; i < NUMBER_OF_STORED_READINGS; i++){
			frequency_powers[i] = (creal(fft_output[i])*creal(fft_output[i]) + cimag(fft_output[i])*cimag(fft_output[i]));
			warpPrint("component %d: %d\n", i,(int)frequency_powers[i]);
            // warpPrint("%f + i%f\n", creal(fft_output[i]), cimag(fft_output[i]));
        }

		draw_frequency_chart(frequency_powers);

    }
	*/

	return 0;
}

void chart_init(void){
	bin_width = 0x5F / NUMBER_OF_STORED_READINGS;
	warpPrint("\nBin widths calculated.\n");
}

void draw_frequency_bar(int start, int end, int height, int colour){
	// set start and end row and column


	writeCommand(kSSD1331CommandDRAWRECT);
	warpPrint("Sending coords...");
	writeCommand(start); // start col
	writeCommand(0x00); // start row
	writeCommand(end); // end col
	writeCommand(height); // end row

	// set outline colour
	warpPrint("Sending outline command...");
	writeCommand(0x00);
	writeCommand(colour);
	writeCommand(0x00);

	// set fill colour
	writeCommand(0x00);
	writeCommand(colour);
	writeCommand(0x00);
}

void draw_frequency_chart(double *bar_heights){

	int normalised_height, start, end;
	
	int max = 0;
	for (int c = 0; c < NUMBER_OF_STORED_READINGS; c++){
        if (bar_heights[c] > max){
        	max = bar_heights[c];
		}
    }

	warpPrint("Max bar height found.\n");

	/*
	// clear screen before redrawing
	warpPrint("Clearing..\n");
	warpEnableSPIpins();
	writeCommand(kSSD1331CommandCLEAR);
	
	writeCommand(0x00);
	writeCommand(0x00);
	warpPrint("Start coords sent.\n");
	writeCommand(0x5F);
	writeCommand(0x3F);
	warpPrint("End coords sent.\n");
	warpPrint("Screen cleared.\n");
	*/

	


	warpPrint("Drawing chart:\n");
	for(int i = 0; i < NUMBER_OF_STORED_READINGS; i++){

		// height
		normalised_height = (bar_heights[i]/max)*(0x3F) + 1;
		warpPrint("\nBin %d: height = %d, ", i, normalised_height);

		start = i * bin_width;
		warpPrint("start = %d, ", start);

		end = start + bin_width;
		warpPrint("end = %d\n", end);

		warpPrint("Drawing bar...\n");
		draw_frequency_bar(start, end, 0x3A, 0xFF);
	}
 }