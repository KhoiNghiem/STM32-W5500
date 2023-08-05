#include <Arduino.h>

// put function declarations here:#include <SPI.h>
#include <SPI.h>
#include <Ethernet.h>
#include "DHT.h"
#include "EEPROM.h"

#define DHTPIN PB9
#define DHTTYPE DHT22
#define BUZZER PA8
#define GAS PA0

double temp = 25.5;
double humi = 50;
int status = 0;
int warning = 0;
int count = 0;

;

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};  // physical mac address
byte ip[] = {192, 168, 0, 200};                     // ip in lan (that's what you need to use in your browser. ("192.168.1.178")
byte gateway[] = {192, 168, 0, 1};                  // internet access via router
byte subnet[] = {255, 255, 255, 0};                 // subnet mask
EthernetServer server(80);                          // server port

uint16_t DataWrite = 0;
uint16_t AddressWrite = 0x10;

String readString;
DHT dht(DHTPIN, DHTTYPE);
uint32_t lastTime = 0;
uint32_t lastTime2 = 0;
bool statusBuzzer = HIGH;
bool statusLed = LOW;
void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  // start the Ethernet connection and the server:
  // Ethernet.init(PA4);
  pinMode(PC13, OUTPUT);
  pinMode(GAS, INPUT);
  pinMode(BUZZER, OUTPUT);
  Ethernet.begin(mac, ip, gateway, subnet);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
  digitalWrite(BUZZER, statusBuzzer);
  dht.begin();
  statusLed = EEPROM.read(255);
  digitalWrite(PC13, statusLed);
}

void loop()
{
  // Create a client connection
  status = digitalRead(GAS);
  if (status == 0)
  {
    warning = 1;
    count = 0;
    // lastTime2 = millis();
    // digitalWrite(BUZZER,LOW);
  }

  if (warning == 1 && millis() - lastTime2 > 500)
  {
    lastTime2 = millis();
    statusBuzzer = !statusBuzzer;
    digitalWrite(BUZZER, statusBuzzer);
    count++;
    if (count > 10)
    {
      digitalWrite(BUZZER, HIGH);
      count = 0;
      warning = 0;
    }
  }

  if (millis() - lastTime > 2000)
  {
    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    humi = dht.readHumidity();
    // Read temperature as Celsius (the default)
     temp = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    dht.readTemperature(true);
    lastTime = millis();
  }
  EthernetClient client = server.available();
  if (client)
  {
    while (client.connected())
    {
      if (client.available())
      {
        char c = client.read();

        // read char by char HTTP request
        if (readString.length() < 100)
        {
          // store characters to string
          readString += c;
          // Serial.print(c);
        }

        // if HTTP request has ended
        if (c == '\n')
        {
          Serial.println(readString); // print to serial monitor for debuging

          client.println("HTTP/1.1 200 OK"); // send new page
          client.println("Content-Type: text/html");
          client.println();

          client.println("<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"><title>Smart Device Control</title><style>body{font-family:Arial,sans-serif;text-align:center;background-color:#f0e8dd}h1{color:#333;margin-top:40px}p{margin:10px 0;color:#757373}button{margin-top:20px;padding:10px 20px;font-size:16px;background-color:#ed9c51;color:#f6d3d3;border:none;border-radius:4px;cursor:pointer}button:hover{background-color:#79cc86}img{margin-top:20px;max-width:200px}.flex{display:flex;justify-content:space-between;align-items:center}.font-24{font-size:24px}.center{display:flex;flex-direction:column;width:50%;margin-left:auto;margin-right:auto}img.Logo{width:50%!important;height:auto;max-width:unset!important}.border{border:3px solid #000;width:50%;margin-left:auto;margin-right:auto;margin-top:24px;padding:16px;box-sizing:border-box}</style></head><body><div><img class=\"Logo\" src=\"https://seee.hust.edu.vn/theme-viendien-theme/images/VienDienTV.png\" alt=\"Logo\"></div><div class=\"border\"><h1>SMART DEVICE CONTROL</h1><div class=\"center\"><p class=\"flex\"><span class=\"font-24\">Trạng thái:</span><span><strong>OK</strong><strong style=\"color:green\">");

          if (warning == 0)
          {
            client.println("✅");
          }
          else if (warning == 1)
          {
            client.println("❌");
          }
          client.println("</strong></span></p><p class=\"flex\"><span class=\"font-24\">Nhiệt độ:</span><strong>");

          client.println(temp);
          client.println("°C</strong></p><p class=\"flex\"><span class=\"font-24\">Độ ẩm:</span><strong>");

          client.println(humi);
          client.println("</strong></p></div><div><button><a href=\"/?led=1\"><strong>Bật LED</a></button><button><a href=\"/?led=0\"><strong>Tắt LED</a></button></div><img src=\"https://cdn-icons-png.flaticon.com/512/954/954861.png\" alt=\"Avatar\"></div></body></html>");
          delay(1);
          // stopping client
          client.stop();
          // controls the Arduino if you press the buttons

          if (readString.indexOf("?led=1") > 0)
          {
            statusLed = LOW;
            EEPROM.write(255, statusLed);
            digitalWrite(PC13, statusLed);
          }
          if (readString.indexOf("?led=0") > 0)
          {
            statusLed = HIGH;
            EEPROM.write(255, statusLed);
            digitalWrite(PC13, statusLed);
          }
          readString = "";
        }
      }
    }
  }
}
