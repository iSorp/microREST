# Micro REST service

### Description
The service provides function to read sensor data and write configurations.

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

#### Configuration

```json 
{
    "os_temp"   = 01, // Over-sampling
    "os_pres"   = 01, // Over-sampling
    "odr"       = 05, // ODR options
    "filter"    = 01, // COEFF_2
    "altitude"  = 500 // altitude above sea level
}
```

#### Mode
```
mode=0x00 // Sleep
mode=0x01 // Forced
mode=0x03 // Normal
```

#### Action
```
action=reset
```

## Siege test
```
siege -H 'Content-Type: application/json'  'http://localhost:8888/board/bmp280/config PATCH {"param1":"green","param2":"brs"}' 
```

```
Lifting the server siege...
Transactions:		       17340 hits
Availability:		      100.00 %
Elapsed time:		       52.02 secs
Data transferred:	        1.06 MB
Response time:		        0.07 secs
Transaction rate:	      333.33 trans/sec
Throughput:		        0.02 MB/sec
Concurrency:		       24.62
Successful transactions:       17385
Failed transactions:	           0
Longest transaction:	        2.46
Shortest transaction:	        0.01
```