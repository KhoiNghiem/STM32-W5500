# STM32 Ethernet network connection using module W5500.
## About
Application to control led lights, display temperature and humidity by DHT22, gas leak alarm by MQ2 through a web interface
STM32 Blue Pill connected to Wiznet W5500 to connect, send data, and control signals from the web down to devices and sensors.

## Components Required
* STM32F103C8T6
* Wiznet W5500
* DHT22
* MQ2
* LED

## Circuit Diagram and connections
| STM32  | Device  |
|:---------:|:---------:|
| PA4  | MOSI (W5500) | 
| PA5  | MISO (W5500) |
| PA6  | SCLK (W5500) |
| PA7  | SCS (W5500) |
| PA8  | SIREN |
| PA0  | OUT (MQ2) |
| PC13 | LED |
| PB9  | DATA (DHT22) |



## Working & Code Explanation
Use ST-Link V2 to upload the code to STM32
```
G   -- GND
CLK -- SWCLK
IO  -- SWDIO
V3  -- 3.3V
```
### Library
```
Ethernet.h
SPI.h
DHT.h
ioLibrary_Driver
```
### Configuration
#### MAC, IP, Mask, Gateway, and Remote Server IP
Edit in main.cpp
```
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};  // physical mac address
byte ip[] = {192, 168, 0, 200};                     // ip in LAN (that's what you need to use in your browser. ("192.168.1.178")
byte gateway[] = {192, 168, 0, 1};                  // internet access via router
byte subnet[] = {255, 255, 255, 0};                 // subnet mask
EthernetServer server(80);                          // server port
```

#### Compile
Using PlatformIO to **build**
```
platformio.exe run
```
To fix the web interface
```
main.html
```
#### Test
Connect Ethernet to the W5500 and type your IP address into the browser.
```
192.168.0.200
```
## Reference
* Official Library: https://github.com/Wiznet/ioLibrary_Driver
* Home page: https://www.wiznet.io/product-item/w5500/
* Official WIKI: http://ww7.wizwiki.net/wiki/doku.php/products:w5500:driver

