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

#define NUMBER_OF_STORED_READINGS 1024
volatile int32_t adc_readings[NUMBER_OF_STORED_READINGS];
// volatile int32_t* oldest_reading = &adc_readings[0];
int oldest_reading_index = 0;

const uint32_t instance = 0U;
const uint32_t chnGroup = 0U;
const uint8_t chn = 8U;



void ADCinit(void)
{

    #if FSL_FEATURE_ADC16_HAS_CALIBRATION
        adc16_calibration_param_t MyAdcCalibraitionParam;
    #endif // FSL_FEATURE_ADC16_HAS_CALIBRATION //

    adc16_user_config_t MyAdcUserConfig;
    adc16_chn_config_t MyChnConfig;
    uint16_t MyAdcValue;
    uint32_t i;

    #if FSL_FEATURE_ADC16_HAS_CALIBRATION
        // Auto calibraion. //
        ADC16_DRV_GetAutoCalibrationParam(instance, &MyAdcCalibraitionParam);
        ADC16_DRV_SetCalibrationParam(instance, &MyAdcCalibraitionParam);
    #endif // FSL_FEATURE_ADC16_HAS_CALIBRATION //

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

    for (i = 0U; i < 4U; i++)
    {
        // Wait for the most recent conversion.//
        ADC16_DRV_WaitConvDone(instance, chnGroup);

        // Fetch the conversion value and format it. //
        MyAdcValue = ADC16_DRV_GetConvValueRAW(instance, chnGroup);
        warpPrint("ADC16_DRV_GetConvValueRAW: 0x%X\t", MyAdcValue);
        warpPrint("ADC16_DRV_ConvRAWData: %ld\r\n",
        ADC16_DRV_ConvRAWData(MyAdcValue, false,
        kAdcResolutionBitOfSingleEndAs12) );
    }
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
        adc_readings[i] = read_from_adc();
    }
}

/*
 * Fetches new data point from ADC and overwrites the oldest prior point to store.
 */
void update_adc_data(void){
    // I want this to fetch the latest ADC value, convert it and then put it in 
    // into some array of fixed length, overwriting oldest data value if full

    int32_t new_adc_read = read_from_adc();
    // *oldest_reading = new_adc_read;
    adc_readings[oldest_reading_index] = new_adc_read;
    oldest_reading_index = (oldest_reading_index + 1)%NUMBER_OF_STORED_READINGS;
}
