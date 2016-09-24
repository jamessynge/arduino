/*
 *  Simple HTTP get webclient test
 */
 
#include <ESP8266WiFi.h>
#include <Wire.h>
#include "Adafruit_HTU21DF.h"

Adafruit_HTU21DF htu = Adafruit_HTU21DF();
const char* ssid     = "stargate";
const char* password = "sarahanneyakersynge";
const char* host = "192.168.1.17";
 
void setup() {
  Serial.begin(9600);

  pinMode(0, INPUT);  // Red LED
  pinMode(2, INPUT);  // Blue LED
//  digitalWrite(0, LOW);
//  digitalWrite(2, LOW);

  // We start by connecting to a WiFi network
 
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
//    digitalWrite(0, LOW);

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Connect to HTU21D-F Temperature and Humidity Sensor.
//    digitalWrite(2, HIGH);
  Wire.begin(4,5);
  if (!htu.begin()) {
    Serial.println("Couldn't find Temperature and Humidity Sensor!");
    while (1);
  }
//    digitalWrite(2, LOW);
  Serial.println("Connected to Temperature and Humidity Sensor");
}

int value = 0;
 
void loop() {
  ++value;
//  digitalWrite(2, HIGH);
  float temperature = htu.readTemperature();
  float humidity = htu.readHumidity();
  Serial.print("Temp: "); Serial.print(temperature);
  Serial.print("\t\tHum: "); Serial.println(humidity);
//  digitalWrite(2, LOW);

  Serial.print("Connecting to ");
  Serial.println(host);
  
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 8080;

//  digitalWrite(0, HIGH);
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }
//  digitalWrite(0, LOW);

  // We now create a URI for the request
  Serial.println("Connected to server");

  // This will send the request to the server
  client.print("GET /.record-data?temperature=");
  client.print(temperature);
  client.print("&humidity=");
  client.print(humidity);
  client.print(" HTTP/1.1\r\n");
  client.print(String("Host: ") + host + "\r\nConnection: close\r\n\r\n");
  client.flush();
  delay(500);
  
  // Read all the lines of the reply from server and print them to Serial
  while(client.available()){
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }
  
  Serial.println();
  Serial.println("closing connection");

  delay(60000);
}

