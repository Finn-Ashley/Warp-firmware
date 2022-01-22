#define NUMBER_OF_STORED_READINGS 16
#define NUMBER_OF_FREQS 8
#define IGNORED_FREQS 2
#define SHUNT IGNORED_FREQS / 2
#define SAMPLING_PERIOD 0.5

void ADCinit(void);
int32_t read_from_adc(void);
void ADC_burn_in(void);
void update_adc_data(void);

extern int adc_readings[NUMBER_OF_STORED_READINGS];