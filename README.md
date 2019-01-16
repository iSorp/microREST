# Micro REST service

### Description
The service provides functions to read sensor data and write configurations.

### Supported Emvoirments
The code was testet on a Rasperry PI 3, i2c interface, bmp280 sensor board

### Supported Authentication
- Basic
- Digest
- Bearer

### Sensors 
- Humidity
- Air pressure

### Libraries
- lmicrohttpd
- lpthread
- lm

### Routes

| Route                            | Method | Description                                                                          |
|----------------------------------|--------|--------------------------------------------------------------------------------------|
| /                                | GET    | (root) the client can verify if the server responds to requests                      |
| /user/auth                       | GET    | User basic authentication, returns a token                                           |
| /board/{id}/sensor/data          | GET    | Returns all sensors data as a json string                                            |
| /board/{id}/sensor/{sensor}/data | GET    | Returns the data of a specified sensor as a json string                              |
| /board/{id}/config               | PUT    | Sets the configuration of a specified board. The mode ist transfered by an post body |
| /board/{id}/action               | PUT    | Fires an action to a specified board. The action ist transfered by an url parameter  |
| /board/{id}/mode                 | PUT    | Fires an mode to a specified board. The action ist transfered by an url parameter    |

### DATA


#### Return data

```json 
{
    "status": success/invalid,
    "message": string,
    "board": string,
    "data": {
        "temperature": double,
        "pressure": double
    }
}
```

#### Configuration

```json 
{
    "os_temp"   : 01, // Over-sampling
    "os_pres"   : 01, // Over-sampling
    "odr"       : 05, // ODR options
    "filter"    : 01, // COEFF_2
    "altitude"  : 500 // altitude above sea level
}
```

#### Mode
```
?mode=0 // Sleep
?mode=1 // Forced
?mode=3 // Normal
```

#### Action
```
?action=reset
```

#### Timespan
Some routes support long polling get requests
```
?timespan=integer_seconds
```
Example: 
curl http://localhost:8888/board/bmp280/sensor/temperature/data?timespan=10

## Examples
```
curl -X PUT -u admin:1234 --header "Content-Type: application/json"  --data '{"os_temp":1,"os_pres":1,"odr":5,"filter":1,"altitude":500}' http://localhost:8888/board/bmp280/config
```


## Tests
### Siege

siege -b -t5S http://localhost:8888/

```
Transactions:		        3800 hits
Availability:		      100.00 %
Elapsed time:		        4.99 secs
Data transferred:	        0.66 MB
Response time:		        0.03 secs
Transaction rate:	      761.52 trans/sec
Throughput:		        0.13 MB/sec
Concurrency:		       24.26
Successful transactions:        3800
Failed transactions:	           0
Longest transaction:	        0.07
Shortest transaction:	        0.01
```

With reading sensor data

siege -b -t5s  http://localhost:8888/board/bmp280/sensor/temperature/data

```
Transactions:		        1120 hits
Availability:		      100.00 %
Elapsed time:		        4.20 secs
Data transferred:	        0.12 MB
Response time:		        0.09 secs
Transaction rate:	      266.67 trans/sec
Throughput:		        0.03 MB/sec
Concurrency:		       24.53
Successful transactions:        1120
Failed transactions:	           0
Longest transaction:	        0.13
Shortest transaction:	        0.01
```


### Valgrind
```
valgrind  --leak-check=full --show-leak-kinds=all ./httpserver.out
valgrind --tool=massif  ./httpserver.out
```

Memory consumption GET http://localhost:8888/
![GitHub Logo](/doc/get.png)

Memory consumption PUT http://localhost:8888/board/bmp280/config
![GitHub Logo](/doc/put.png)

Memory consumption GET 25 clients (long polling) http://localhost:8888/board/bmp280/sensor/temperature/data?timespan=1
![GitHub Logo](/doc/stream.png)

