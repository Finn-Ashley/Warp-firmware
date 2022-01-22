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

//#include "/home/students/fwa20/Warp-firmware/tools/sdk/ksdk1.1.0/platform/CMSIS/Include/device/MKL03Z4/MKL03Z4_features.h"

const uint32_t instance = 0U;
const uint32_t chnGroup = 0U;
const uint8_t chn = 8U;

int adc_readings[NUMBER_OF_STORED_READINGS];

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


    ADC_burn_in();
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
 * Prepopulate the ADC data array with a full set of readings.
 * Separates out fetching new data after this point which will require
 * looping round and overwriting old data.
 */

void ADC_burn_in(void){

    for(int i = 0; i < NUMBER_OF_STORED_READINGS; i++){
        // wait for and fetch conversion
        adc_readings[i] = (int)read_from_adc();

        // OSA_TimeDelay(SAMPLING_PERIOD);
    }
}

/*
 * Fetches new data point from ADC and overwrites the oldest prior point to store.
 */
void update_adc_data(void){
    // I want this to fetch the latest ADC value, convert it and then put it in 
    // into some array of fixed length, overwriting oldest data value if full

    // shift data in array left by one to free up most recent datapoint
    for(int i = 0; i < NUMBER_OF_STORED_READINGS - 2; i++){
        adc_readings[i] = adc_readings[i+1];
    }

    // add in new datapoint
    adc_readings[NUMBER_OF_STORED_READINGS - 1] = (int)read_from_adc();
}