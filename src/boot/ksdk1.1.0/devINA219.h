void initINA219(const uint8_t i2cAddress);
WarpStatus writeSensorRegisterINA219(uint8_t deviceRegister, uint16_t payload);
WarpStatus readSensorRegisterINA219(uint8_t deviceRegister, int numberOfBytes);
void printSensorDataINA219(bool hexModeFlag);
void printCurrentDataINA219(bool hexModeFlag);