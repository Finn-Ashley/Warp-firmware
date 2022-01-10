#define NUMBER_OF_STORED_READINGS 16
#define FSL_FEATURE_ADC16_HAS_CALIBRATION 1

void ADCinit(void);
int32_t read_from_adc(void);
void ADC_burn_in(void);
void update_adc_data(void);
void fetch_adc_to_heap(void);
void populate_adc_heap(void);

extern int adc_readings[NUMBER_OF_STORED_READINGS];