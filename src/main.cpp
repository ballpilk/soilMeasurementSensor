#include <ESP8266WiFi.h>      // Include the Wi-Fi library
#include <ESP8266WiFiMulti.h> // Include the Wi-Fi-Multi library
#include <ESP8266mDNS.h>      // Include the mDNS library
// BEGIN missing includes for autoconnect library
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <DNSServer.h>
#include <Ticker.h>
#include <ESP8266httpUpdate.h>
#include <SPI.h>
#include <SD.h>
// END  missing includes for autoconnect library

#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Arduino.h>
#include <EEPROM.h>
#include <HumidityMeasurements.hpp>
#include <WifiCredencials.hpp>
//ESP8266WiFiMulti wifiMulti; // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'
WiFiServer server(80);      // Set port to 80

const long utcOffsetInSeconds = 7200;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

String header; // This storees the HTTP request
MeasurementBuffer measBuff;

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);


  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
}
void setup()
{
  Serial.begin(9600); // Start the Serial communication to send messages to the computer
  delay(10);
  pinMode(D9, OUTPUT);
  Serial.println('\n');

  initWiFi();

  if (!MDNS.begin("miernikWilgotnosciGleby"))
  { // Start the mDNS responder
    Serial.println("Error setting up MDNS responder!");
  }
  Serial.println("mDNS responder started");
  server.begin();
  timeClient.begin();
  timeClient.update();
  Serial.println("Setup complete");
}



void printTime(){
    Serial.print(timeClient.getDay());
    Serial.print(", ");
    Serial.print(timeClient.getHours());
    Serial.print(":");
    Serial.print(timeClient.getMinutes());
    Serial.print(":");
    Serial.println(timeClient.getSeconds());
}

/*unsigned long int getOffset()
{
  unsigned long int currTime = timeClient.getHours()*3600 + timeClient.getMinutes()*60 + timeClient.getSeconds();
  return (86400 + currTime - (millis() / 1000)%86400 ) % 86400;

}
unsigned long int getCurrentTimeSec()
{
  return timeClient.getHours()*3600 + timeClient.getMinutes()*60 + timeClient.getSeconds();
}
*/

void measurements()
{
  static unsigned long int last_meas_time = 0;
  const unsigned long int meas_update_period = 1200000;//20min
  timeClient.update();
  if(millis() - last_meas_time > meas_update_period)
  {
    measBuff.addMeasurement(timeClient);
    last_meas_time = millis();
    printTime();
    Serial.println(analogRead(A0));
  }
}
void RSSIPrint()
{
  static unsigned long int rssiTime = 0;
  const unsigned long int rssiperiod = 2000;
  if (millis() - rssiTime > rssiperiod)
  {
      
      Serial.println(String(WiFi.RSSI()) + ", " + String(timeClient.getDay()) + " " + String(timeClient.getHours()) + ":" + String(timeClient.getMinutes()) + ":" + String(timeClient.getSeconds()) + " ");
      Serial.println(analogRead(A0));
      Serial.println(measBuff.getHumidity());
      rssiTime = millis();
  }  

}
String measurementsDataPage(MeasurementBuffer& buff)
{
  MeasOutput* out = buff.getMeasurements();
  String retval = "{\"Measurements\":[";
  for(int i=0; i < buff.len-1; ++i)
  {
    retval += "{\"Time\":\"" + out[i].time + "\", \"value\": " + String(out[i].meas, 4) + " }, \r\n";
  }
  retval += "{\"Time\":\"" + out[buff.len-1].time + "\", \"value\": " + String(out[buff.len-1].meas, 4) + " } \r\n";
  retval += "]}\r\n";
  return retval;
}
void webClientHandling()
{
  WiFiClient client = server.available(); // Listen for incoming clients
  if (client)
  {                          // If a new client connects,
    String currentLine = ""; // make a String to hold incoming data from the client
    while (client.connected())
    { // loop while the client's connected
      if (client.available())
      {                         // if there's bytes to read from the client,
        char c = client.read(); // read a byte, then
        Serial.write(c);        // print it out the serial monitor
        header += c;
        if (c == '\n')
        { // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0)
          {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:application/json");
            client.println("Connection: close");
            client.println();
            if (header.indexOf("GET /ID") >= 0)
            {
                client.println(String("SoilHumiditySensor tcp/json") + WiFi.localIP().toString() );
                break;
            }
            if (header.indexOf("GET /history") >= 0)
            {
                client.println(measurementsDataPage(measBuff));
                break;
            }
            if (header.indexOf("GET /current") >= 0)
            {
                client.print(String(daysOfTheWeek[timeClient.getDay()]) + " " + String(timeClient.getHours()) + ":" + String(timeClient.getMinutes()) + ":" + String(timeClient.getSeconds()) + " ");
                client.println(String(measBuff.getHumidity()));
                break;
            }
            
            else if (header.indexOf("GET /set/") >= 0)
            {//set new status
              
              client.println();
              client.println();
              client.println();
            break;
            }
            else{
              // Display the HTML web page
              client.println();
              client.println();
              client.println();
              break;
            }
          }
          else
          { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        }
        else if (c != '\r')
        {                   // if you got anything else but a carriage return character,
          currentLine += c; // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}

void loop()
{

  measurements();
  RSSIPrint();
  webClientHandling();
  if(millis()%2000 > 100){
      digitalWrite(D9, HIGH);
  }
  else{
      digitalWrite(D9, LOW);
  }

}
#ifndef ARDUINO
int main()
{
	while(true)
	{
		loop();
	}
}
#endif
