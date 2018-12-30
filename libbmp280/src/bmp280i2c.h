#include <stdlib.h>

int
initI2C();

int
initBmc280();

int 
softReset();

int 
setPowerMode(int8_t mode);

int
setConfiguration(struct bmp280_config *conf);

int
getConfiguration(struct bmp280_config *conf);

double 
readTemperature();

double 
readPressure();