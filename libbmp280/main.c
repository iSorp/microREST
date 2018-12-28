#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "bmp280.h"
#include "bmp280i2c.h"

/*
*  Main function for testing
*/
int
main (void)
{
    initI2C();
    initBmc280();
    while (1) {
		double temp = readTemperature();
        double pressure = readPressure();
		printf("Measurement: temperature %f, pressure %f\r\n", temp, pressure);
        sleep(1);
	}
    return 0;
}
