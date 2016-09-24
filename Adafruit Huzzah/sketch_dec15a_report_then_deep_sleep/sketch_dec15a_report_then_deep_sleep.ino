/* Reports temperature, humidity and Vcc voltage to a web server.
 * 
 * Change Log:
 * 2015-12-14:        Deep sleep; GPIO 16 to RST.
 * 2015-12-13 11:14:  Refactor to pull temperature & humidity measurement out, etc.
 * 
 * Status: Deep sleeping for 5 minutes at a time, it was able to make 237 reports
 * over 18 hours and 43 minutes.
 *
 * TODO Come up with voltage divider for measuring BAT voltage, hook up to
 * an available GPIO pin; set pin to OUTPUT LOW when measuring;
 * at other times set pin to INPUT, else OUTPUT HIGH.
 * 
 * TODO Determine whether the HTU21D can be powered by a GPIO OUTPUT pin set to HIGH.
 * If so, this would enable turning off the chip when in deep sleep.
 */

#include <ESP8266WiFi.h>
#include <Wire.h>
#include "Adafruit_HTU21DF.h"

// Enable ESP.getVcc();
// TODO Determine how to move this declaration into runtime code so that
// the decision of measuring internal Vcc or an external input can be made
// at runtime.
ADC_MODE(ADC_VCC);

#define RED_LED 0   // Red LED on Adafruit HUZZAH Feather; no pull-up resistor, LOW lights up the LED.
#define BLUE_LED 2  // Blue LED on Adafruit HUZZAH Feather; has a pull-up resistor, LOW lights up the LED.

const char* ssid     = "stargate";
const char* password = "sarahanneyakersynge";
const char* hostAddr = "192.168.1.17";
const char* hostName = "jsdabbler-hrd.appspot.com";

const int report_interval_secs = 5 * 60;

String FormatDurationMs(unsigned long ms) {
  int seconds = ms / 1000;
  ms -= seconds * 1000;
  int minutes = seconds / 60;
  seconds -= minutes * 60;
  int hours = minutes / 60;
  minutes -= hours * 60;
  char buf[64];
  int n = snprintf(buf, 64, "%d:%02d:%02d.%03d",
      hours, minutes, seconds, ms);
  buf[n] = 0;
  return String(buf);
}

// Use low power mode, which will reboot the ESP after the specified sleep period
// (in particular, will run this sketch from the beginning again).
void EnterDeepSleep() {
  unsigned long current_time = millis();
  Serial.print("Current time: ");
  Serial.print(FormatDurationMs(current_time));
  Serial.print("  (");
  Serial.print(current_time);
  Serial.println(')');

  unsigned long wake_time = report_interval_secs * 1000;
  if (wake_time < current_time) {
    wake_time = current_time + 1;
  }
  unsigned long sleep_ms = wake_time - current_time;
  Serial.print("Sleep time: ");
  Serial.print(FormatDurationMs(sleep_ms));
  Serial.print("  (");
  Serial.print(sleep_ms);
  Serial.println(')');

  // Various attempts to get red led to stop glowing dimly during deep sleep,
  // which is wasting power. It doesn't happen all the time, but often.
  pinMode(RED_LED, OUTPUT);
  digitalWrite(RED_LED, LOW);
  delay(10);
  digitalWrite(RED_LED, HIGH);
  delay(10);

  unsigned long sleep_us = sleep_ms * 1000;
  Serial.println("Requesting deep sleep mode...");
  ESP.deepSleep(sleep_us, WAKE_RFCAL);
  // Wait a little while because the ESP8266 takes a while to enter low power mode after that call.
  delay(100);  // Probably won't reach next line.
  Serial.print("Requested deep sleep mode");
  while (true) {
    delay(10);
    Serial.print(".");
  }
}

void ConnectToWiFi() {
  Serial.print("Connecting to WiFi access point ");
  Serial.println(ssid);

  // TODO Store params, especially password, in EEPROM instead of in code.
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("WiFi connected, local IP address: ");  
  Serial.println(WiFi.localIP());
}

void GETtoSerial(const char* hostAddr, const char* hostName, int httpPort, const String& url) {
  if (hostAddr == nullptr) {
    hostAddr = hostName;
  } else if (hostName == nullptr) {
    hostName = hostAddr;
  }

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  Serial.print("Client created, client status: ");
  Serial.println(client.status());

  Serial.print("Connecting to http://");
  Serial.print(hostAddr);
  Serial.print(":");
  Serial.print(httpPort);
  Serial.println(url);
  if (!client.connect(hostAddr, httpPort)) {
    Serial.println("connection failed");
    return;
  }

  Serial.print("Connected, client status: ");
  Serial.println(client.status());

  // We now create a URI for the request
  Serial.println("Connected to server, sending GET request");

  // This will send the request to the server
  client.print("GET ");
  client.print(url);
  client.print(" HTTP/1.1\r\n");
  client.print(String("Host: ") + hostName + "\r\nConnection: close\r\n\r\n");
  client.flush();

  Serial.print("Sent request, client status: ");
  Serial.println(client.status());

  // TODO Determine if this delay is necessary, or if I can just loop checking for status is still ESTABLISHED, and waiting until
  // available is true.
  delay(500);

  // Read all the lines of the reply from server and print them to Serial.
  // TODO Parse the server timestamp, and make it available for computing when we
  // should next report (e.g. if prefer to report on the hour, then try to move to reporting such that the offset is tiny).
  while(client.available()){
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }

  Serial.println();
  Serial.print("Status: ");
  Serial.println(client.status());
  Serial.print("Closing connection...");
  client.stop();
  Serial.println("  disconnected");
}

class Reporter {
 public:
  Reporter() {}

  void Init() {
    Serial.println("Initializing HTU21D communication");
    Wire.begin(4,5);  // On Adafruit Feather HUZZAH ESP8266, pins 4 and 5 are SDA and SDC.
    if (!htu_.begin()) {
      Serial.println("Couldn't find Temperature and Humidity Sensor!");
      while (1);  // TODO Come up with a better death signal/response
                  // (e.g. light up Red LED as SOS, and blue with an error code).
    }
    Serial.println("Connected to Temperature and Humidity Sensor");
  }
 
  void Measure() {
    const int loops = 16;
    temperature_ = 0;
    humidity_ = 0;
    vcc_ = 0;
    for (int loop = 0; loop < loops; ++loop) {
      temperature_ += htu_.readTemperature();
      humidity_ += htu_.readHumidity();
      vcc_ += ESP.getVcc();
    }
    temperature_ /= loops;
    humidity_ /= loops;
    vcc_ /= loops;
    vcc_ /= 1024;  // ADC reads in units of 1/1024 volts.

    // See https://www.adafruit.com/datasheets/1899_HTU21D.pdf for
    // the source of this formula.
    compensated_ = humidity_ + (25.0 - temperature_) * 0.15;
  }

  void PrintMeasurements() {
    Serial.print("Temp: "); Serial.print(temperature_);
    Serial.print("\t\tRH(raw): "); Serial.print(humidity_);
    Serial.print("\t\tRH(comp): "); Serial.print(compensated_);
    Serial.print("\t\tVcc: "); Serial.print(vcc_);
    Serial.println();
  }

  void ReportMeasurements() {
    String url("/.record-data?temperature=");
    url += temperature_;
    url += "&humidity=";
    url += humidity_;
    url += "&compensated=";
    url += compensated_;
    url += "&vcc=";
    url += vcc_;
    GETtoSerial(hostAddr, hostName, 8080, url);
  }

 private:
  float temperature_;  // Degrees C
  float humidity_;     // Percent Relative Humidity
  float compensated_;  // Percent Relative Humidity (temperature adjusted).
  float vcc_;
  Adafruit_HTU21DF htu_;
};

Reporter reporter;

void measureAndReport() {
  reporter.Measure();
  reporter.PrintMeasurements();
  reporter.ReportMeasurements();
}

void setup() {
  unsigned long start_time = millis();
  Serial.begin(115200);
  pinMode(RED_LED, INPUT);
  pinMode(BLUE_LED, INPUT);

  // Give serial a bit of time (not sure if needed).
//  delay(100);
  Serial.println();
  Serial.print("Start time: ");
  Serial.println(start_time);

  // Connect to wi-fi base station.
  ConnectToWiFi();

  // Connect to HTU21D-F Temperature and Humidity Sensor.
  reporter.Init();

  Serial.print("setup() took: ");
  unsigned long end_time = millis();
  Serial.println(end_time - start_time);

  start_time = end_time;
  reporter.Measure();
  end_time = millis();
  Serial.print("Measure() took: ");
  Serial.println(end_time - start_time);
  reporter.PrintMeasurements();
  start_time = end_time;
  reporter.ReportMeasurements();
  end_time = millis();
  Serial.print("ReportMeasurements() took: ");
  Serial.println(end_time - start_time);

  EnterDeepSleep();
}

void loop() {
  Serial.println("SHOULD NOT REACH LOOP!");
}

