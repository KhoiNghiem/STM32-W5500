/****************************************************************************************************************************
  WebClient_ESP_SPI2.ino

  Ethernet_Generic is a library for the W5x00 Ethernet shields trying to merge the good features of
  previous Ethernet libraries

  Built by Khoi Hoang https://github.com/khoih-prog/Ethernet_Generic
 *****************************************************************************************************************************/

// Important: For ESP32, check https://github.com/khoih-prog/EthernetWebServer#7-for-fixing-esp32-compile-error
// and replace cores/esp32/Server.h to avoid compile error

#include "defines.h"

char server[] = "arduino.tips";

// Initialize the Ethernet client object
EthernetClient client;

void setup()
{
  Serial.begin(115200);

  while (!Serial && millis() < 5000);

  delay(200);

  Serial.print("\nStarting WebClient_ESP_SPI2 on ");
  Serial.print(ARDUINO_BOARD);
  Serial.print(" with ");
  Serial.println(SHIELD_TYPE);
  Serial.println(ETHERNET_GENERIC_VERSION);

  // You can use Ethernet.init(pin) to configure the CS pin
  //Ethernet.init(10);  // Most Arduino shields
  //Ethernet.init(5);   // MKR ETH shield
  //Ethernet.init(0);   // Teensy 2.0
  //Ethernet.init(20);  // Teensy++ 2.0
  //Ethernet.init(15);  // ESP8266 with Adafruit Featherwing Ethernet
  //Ethernet.init(33);  // ESP32 with Adafruit Featherwing Ethernet

#ifndef USE_THIS_SS_PIN
#define USE_THIS_SS_PIN   5   //22    // For ESP32
#endif

  ETG_LOGWARN1(F("ESP32 setCsPin:"), USE_THIS_SS_PIN);

  // Must use library patch for Ethernet, EthernetLarge libraries
  // ESP32 => GPIO2,4,5,13,15,21,22 OK with Ethernet, Ethernet2, EthernetLarge
  // ESP32 => GPIO2,4,5,15,21,22 OK with Ethernet3

  //Ethernet.setCsPin (USE_THIS_SS_PIN);
  Ethernet.init (USE_THIS_SS_PIN);

  // start the ethernet connection and the server:
  // Use DHCP dynamic IP and random mac
  uint16_t index = millis() % NUMBER_OF_MAC;
  // Use Static IP
  //Ethernet.begin(mac[index], ip);
  Ethernet.begin(mac[index]);

#if defined( ESP32 )
  // Just info to know how to connect correctly
  // To change for other SPI
  Serial.println("=========================");
  Serial.println("Currently Used SPI pinout:");
  Serial.print("MOSI:");
  Serial.println(PIN_MOSI);
  Serial.print("MISO:");
  Serial.println(PIN_MISO);
  Serial.print("SCK:");
  Serial.println(PIN_SCK);
  Serial.print("SS:");
  Serial.println(USE_THIS_SS_PIN);
  Serial.println(F("========================="));
#endif

  Serial.print(F("Using mac index = "));
  Serial.println(index);

  Serial.print(F("Connected! IP address: "));
  Serial.println(Ethernet.localIP());

  if ( (Ethernet.getChip() == w5500) || (Ethernet.getChip() == w6100) || (Ethernet.getAltChip() == w5100s) )
  {
    if (Ethernet.getChip() == w6100)
      Serial.print(F("W6100 => "));
    else if (Ethernet.getChip() == w5500)
      Serial.print(F("W6100 => "));
    else
      Serial.print(F("W5100S => "));
    
    Serial.print(F("Speed: "));
    Serial.print(Ethernet.speedReport());
    Serial.print(F(", Duplex: "));
    Serial.print(Ethernet.duplexReport());
    Serial.print(F(", Link status: "));
    Serial.println(Ethernet.linkReport());
  }

  Serial.println();
  Serial.println(F("Starting connection to server..."));

  // if you get a connection, report back via serial
  if (client.connect(server, 80))
  {
    Serial.println(F("Connected to server"));
    // Make a HTTP request
    client.println(F("GET /asciilogo.txt HTTP/1.1"));
    client.println(F("Host: arduino.tips"));
    client.println(F("Connection: close"));
    client.println();
  }
}

void printoutData()
{
  // if there are incoming bytes available
  // from the server, read them and print them
  while (client.available())
  {
    char c = client.read();
    Serial.write(c);
    Serial.flush();
  }
}

void loop()
{
  printoutData();

  // if the server's disconnected, stop the client
  if (!client.connected())
  {
    Serial.println();
    Serial.println(F("Disconnecting from server..."));
    client.stop();

    // do nothing forevermore
    while (true)
      yield();
  }
}
