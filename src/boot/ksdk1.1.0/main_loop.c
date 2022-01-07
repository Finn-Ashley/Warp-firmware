#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
/*
 *	config.h needs to come first
 */
#include "config.h"
#include "string.h"
#include "fsl_port_hal.h"

#include "main_loop.h"

#include "SEGGER_RTT.h"
#include "gpio_pins.h"
#include "warp.h"
#include "devADC.h"
#include "fft4g.h"

// #include "board.h"
#include "fsl_os_abstraction.h"
#include "fsl_debug_console.h"
#include "fsl_adc16_driver.h"


#ifndef NMAX
#define NMAX 16
#define NMAXSQRT 4
#endif

// populate adc readings
// do fft
// draw screen based on that
void main_loop(void){

    double adc_fft_copy[NUMBER_OF_STORED_READINGS];
    int n, ip[NMAXSQRT + 2];
    double w[NMAX * 5 / 4];

    n = NUMBER_OF_STORED_READINGS;
    ip[0] = 0;

    ADCinit();
    while(true){
        update_adc_data();

        memcpy(adc_fft_copy, adc_readings, NUMBER_OF_STORED_READINGS);

        rdft(n, 1, adc_fft_copy, ip, w);

        for(int i; i < NUMBER_OF_STORED_READINGS; i++){
            warpPrint("%f\n", adc_fft_copy[i]);
        }

        for(int i; i < 10000; i++){
            ;
        }
        

    }

}