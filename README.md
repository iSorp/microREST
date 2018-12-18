# Micro REST service

### Description
This service provides function to read sensor data(temperature and air pressure) and
write thresholds for alarms.
it is possible to read the current values. Alarms are messaged by longpolling.
The connections are encrypted by tls.

It also provides a little web page which shows the current values and supports to add the thresholds.

### Supported Emvoirments
The code was testet on a Rasperry PI 3 and i2c interface for the sensores

### Sensors 
- Humidity
- Air pressure

### Libraries
- lmicrohttpd
- lz


