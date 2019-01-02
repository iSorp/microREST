# Micro REST service

### Description
The service provides functions to read sensor data and write configurations.

### Supported Emvoirments
The code was testet on a Rasperry PI 3, i2c interface, bmp280 sensor board

### Sensors 
- Humidity
- Air pressure

### Libraries
- lmicrohttpd
- lz
- lm
- lbmp280



### Routes

| Route                            | Method | Description                                                                          |
|----------------------------------|--------|--------------------------------------------------------------------------------------|
| /                                | GET    | (root) the client can verify if the server responds to requests                      |
| /user/auth                       | GET    | User basic authentication, returns a token                                           |
| /board/{id}/sensor/data          | GET    | Returns all sensors data as a json string                                            |
| /board/{id}/sensor/{sensor}/data | GET    | Returns the data of a specified sensor as a json string                              |
| /board/{id}/config               | PATCH  | Sets the configuration of a specified board. The mode ist transfered by an post body |
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
?mode=0x00 // Sleep
?mode=0x01 // Forced
?mode=0x03 // Normal
```

#### Action
```
?action=reset
```

#### Timespan
Some routes support long polling get requests (only for HTTP 1.1)
```
?timespan=integer value
```

## Siege test
```
siege -H 'Content-Type: application/json'  'http://localhost:8888/board/bmp280/config PATCH {"os_temp":1,"os_pres":1,"odr":5,"filter":1,"altitude":500}' 
```

```
Lifting the server siege...
Transactions:		       67706 hits
Availability:		      100.00 %
Elapsed time:		      282.10 secs
Data transferred:	        4.33 MB
Response time:		        0.10 secs
Transaction rate:	      240.01 trans/sec
Throughput:		        0.02 MB/sec
Concurrency:		       24.83
Successful transactions:       67706
Failed transactions:	           0
Longest transaction:	        0.64
Shortest transaction:	        0.01
```