#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
/*
 *	config.h needs to come first
 */
#include "config.h"
#include "fsl_port_hal.h"

#include "SEGGER_RTT.h"
#include "gpio_pins.h"
#include "warp.h"
#include "devADC.h"

// #include "board.h"
#include "fsl_os_abstraction.h"
#include "fsl_debug_console.h"
#include "fsl_adc16_driver.h"
#include "fsl_adc16_hal.h"

const uint32_t instance = 0U;
const uint32_t chnGroup = 0U;
const uint8_t chn = 8U;

uint32_t adc_readings[NUMBER_OF_STORED_READINGS];

/*
 * Intialise the ADC as defined in SDK, and
 * set running in continous conversion mode
 * so can access new reading when desired.
 */
void ADCinit(void)
{

    #if FSL_FEATURE_ADC16_HAS_CALIBRATION
        adc16_calibration_param_t MyAdcCalibraitionParam;
    #endif

    adc16_user_config_t MyAdcUserConfig;
    adc16_chn_config_t MyChnConfig;

    #if FSL_FEATURE_ADC16_HAS_CALIBRATION
        // Auto calibraion. //
        ADC16_DRV_GetAutoCalibrationParam(instance, &MyAdcCalibraitionParam);
        ADC16_DRV_SetCalibrationParam(instance, &MyAdcCalibraitionParam);
    #endif

    // Initialize the ADC converter. //
    ADC16_DRV_StructInitUserConfigDefault(&MyAdcUserConfig);
    MyAdcUserConfig.continuousConvEnable = true; // Enable continuous conversion. //
    ADC16_DRV_Init(instance, &MyAdcUserConfig);

    // Configure the ADC channel and take an initial trigger. //
    MyChnConfig.chnNum = chn;
    MyChnConfig.diffEnable= false;
    MyChnConfig.intEnable = false;
    MyChnConfig.chnMux = kAdcChnMuxOfA;
    ADC16_DRV_ConfigConvChn(instance, chnGroup, &MyChnConfig);


    ADC_read_set(0);
}

/*
 * Wrapper function to allow reading from ADC using only
 * one line downstream.
 */
int32_t read_from_adc(void){
    ADC16_DRV_WaitConvDone(instance, chnGroup);
    uint16_t MyAdcValue = ADC16_DRV_GetConvValueRAW(instance, chnGroup);
    int32_t converted_adc_read = ADC16_DRV_ConvRAWData(MyAdcValue, false, kAdcResolutionBitOfSingleEndAs12);
    return converted_adc_read;
}

/*
 * Populate the ADC data array with a full set of readings.
 * Option to add in a delay between readings if desired so
 * sampling frequency isn't based on ADC speed.
 */

void ADC_read_set(bool delay){

    for(int i = 0; i < NUMBER_OF_STORED_READINGS; i++){
        // wait for and fetch conversion
        adc_readings[i] = read_from_adc();

        // if want more control over sampling period, necessary
        // for explicit frequency bin calculations
        if (delay){
            OSA_TimeDelay(SAMPLING_PERIOD);
        }
    }
}