# Micro REST service

### Description
The service provides functions to read sensor data and write configurations.

### Supported Emvoirments
The code was testet on a Raspberry PI 3, i2c interface, bmp280 sensor board

### Supported Authentication
- Basic
- Digest
- Bearer

### Sensors 
- Humidity
- Air pressure

### Libraries
- libmicrohttpd
- libpthread
- libm

### Routes

| Route                            | Method     | Description                                                                          |
|----------------------------------|------------|--------------------------------------------------------------------------------------|
| /                                | GET        | (root) the client can verify if the server responds to requests                      |
| /user/auth                       | GET        | User basic authentication, returns a token                                           |
| /board/{id}/sensor/data          | GET        | Returns all sensors data as a json string                                            |
| /board/{id}/sensor/{sensor}/data | GET        | Returns the data of a specified sensor as a json string                              |
| /board/{id}/config               | GET PUT    | Sets or gets the configuration of a specified board. The config is transferred by HTTP body |
| /board/{id}/action               | PUT        | Fires an action to a specified board. The action is transferred by url query  |
| /board/{id}/mode                 | GET PUT    | Sets or gets the mode of a specified board. The mode is transferred by url query    |

### DATA


#### Return data

```json 
{
    "status": success/invalid,
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

### Apache Benchmark
```
ab -t5 -c25 http://localhost:8888/

Server Software:        
Server Hostname:        localhost
Server Port:            8888

Document Path:          /
Document Length:        182 bytes

Concurrency Level:      25
Time taken for tests:   5.000 seconds
Complete requests:      21689
Failed requests:        0
Total transferred:      6311499 bytes
HTML transferred:       3947398 bytes
Requests per second:    4337.74 [#/sec] (mean)
Time per request:       5.763 [ms] (mean)
Time per request:       0.231 [ms] (mean, across all concurrent requests)
Transfer rate:          1232.70 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.1      0       2
Processing:     1    6   0.4      6       7
Waiting:        1    6   0.4      6       7
Total:          3    6   0.3      6       7

Percentage of the requests served within a certain time (ms)
  50%      6
  66%      6
  75%      6
  80%      6
  90%      6
  95%      6
  98%      6
  99%      7
 100%      7 (longest request)
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

