/* Reports temperature, humidity and Vcc voltage to a web server.
 * 
 * Sequence:
 * 1) Measure temperature, humidity and Vcc voltage.
 * 2) Turn on WiFi radio and connect to AP.
 * 3) Report measurements.
 * 4) Enter deepSleep, requesting WiFi radio to be left off when restarting.
 * 
 * STATUS: WAKE_RF_DISABLED is apparently broken in the current SDK I'm using,
 * 1.6.5-947-g39819f0. This means that there isn't a 'big' benefit to measuring
 * before trying to connect.
 * 
 * Change Log:
 * 2015-12-17:        Put measurements before radio start.
 *                    Replace Serial.print*(...) with console.print*(INFO, ...)
 *                    so that I can control the amount of printing.
 * 2015-12-14:        Deep sleep; GPIO 16 to RST.
 * 2015-12-13 11:14:  Refactor to pull temperature & humidity measurement out, etc.
 * 
 * TODO Come up with voltage divider for measuring BAT voltage, hook up to
 * an available GPIO pin; set pin to OUTPUT LOW when measuring;
 * at other times set pin to INPUT, else OUTPUT HIGH.
 * 
 * TODO Determine whether the HTU21D can be powered by a GPIO OUTPUT pin set to HIGH.
 * If so, this would enable turning off the chip when in deep sleep.
 * 
 * TODO Comment out most Serial.print* code to see if that affects the
 * rate of power consumption noticeably.
 * 
 * TODO Maybe make measurements while connecting to WiFi, rather than before
 * or after, to make use of the time it takes to connect but during those fractions
 * of a millisecond when nothing is happening. delay(0) / yield() allow the WiFi
 * code to run.
 */

#include <ESP8266WiFi.h>
#include <Wire.h>
#include "Adafruit_HTU21DF.h"

extern "C" {
#include "user_interface.h"
}

// Enable ESP.getVcc();
// TODO Determine how to move this declaration into runtime code so that
// the decision of measuring internal Vcc or an external input can be made
// at runtime.
ADC_MODE(ADC_VCC);

#define RED_LED 0   // Red LED on Adafruit HUZZAH Feather; no pull-up resistor, LOW lights up the LED.
#define BLUE_LED 2  // Blue LED on Adafruit HUZZAH Feather; has a pull-up resistor, LOW lights up the LED.

// TODO Store params, especially password, in EEPROM instead of in code.
const char* ssid     = "stargate";
const char* password = "sarahanneyakersynge";
const char* hostAddr = "192.168.1.17";
const char* hostName = "jsdabbler-hrd.appspot.com";

const int report_interval_secs = 20; // 5 * 60;

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

#define DEBUG 0
#define INFO 1
#define RARE 2

class LevelPrinter {
 public:
  LevelPrinter() : curr_level_(INFO) {}

  template <typename T>
  void print(byte level, T t) {
    if (level >= curr_level_) {
      Serial.print(t);
    }
  }

  template <typename T>
  void println(byte level, T t) {
    if (level >= curr_level_) {
      Serial.println(t);
    }
  }

  void println(byte level) {
    if (level >= curr_level_) {
      Serial.println();
    }
  }

 private:
  byte curr_level_;
};

LevelPrinter console;

// Use low power mode, which will reboot the ESP after the specified sleep period
// (in particular, will run this sketch from the beginning again).
void EnterDeepSleep() {
  unsigned long current_time = millis();
  console.print(INFO, "Current time: ");
  console.print(INFO, FormatDurationMs(current_time));
  console.print(INFO, "  (");
  console.print(INFO, current_time);
  console.println(INFO, ')');

  unsigned long wake_time = report_interval_secs * 1000;
  if (wake_time < current_time) {
    wake_time = current_time + 1;
  }
  unsigned long sleep_ms = wake_time - current_time;
  console.print(INFO, "Sleep time: ");
  console.print(INFO, FormatDurationMs(sleep_ms));
  console.print(INFO, "  (");
  console.print(INFO, sleep_ms);
  console.println(INFO, ')');

  // Various attempts to get red led to stop glowing dimly during deep sleep,
  // which is wasting power. It doesn't happen all the time, but often.
  pinMode(RED_LED, OUTPUT);
  digitalWrite(RED_LED, LOW);
  delay(10);
  digitalWrite(RED_LED, HIGH);
  delay(10);

  unsigned long sleep_us = sleep_ms * 1000;
  console.println(INFO, "Requesting deep sleep mode");
  ESP.deepSleep(sleep_us, WAKE_RF_DISABLED);
  // Wait a little while because the ESP8266 takes a while to enter low power mode after that call.
  while (true) {
    delay(1);
    console.print(INFO, ".");
  }
}

void ConnectToWiFi() {
  console.print(INFO, "wifi_get_opmode() -> ");
  console.println(INFO, wifi_get_opmode());
  WiFi.mode(WIFI_STA);

  console.print(INFO, "Connecting to WiFi access point ");
  console.println(INFO, ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    console.print(INFO, ".");
  }

  console.println(INFO, "");
  console.print(INFO, "WiFi connected, local IP address: ");  
  console.println(INFO, WiFi.localIP());
}

void GETtoSerial(const char* hostAddr, const char* hostName, int httpPort, const String& url) {
  if (hostAddr == nullptr) {
    hostAddr = hostName;
  } else if (hostName == nullptr) {
    hostName = hostAddr;
  }

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  console.print(INFO, "Client created, client status: ");
  console.println(INFO, client.status());

  console.print(INFO, "Connecting to http://");
  console.print(INFO, hostAddr);
  console.print(INFO, ":");
  console.print(INFO, httpPort);
  console.println(INFO, url);
  if (!client.connect(hostAddr, httpPort)) {
    console.println(INFO, "connection failed");
    return;
  }

  console.print(INFO, "Connected, client status: ");
  console.println(INFO, client.status());

  // We now create a URI for the request
  console.println(INFO, "Connected to server, sending GET request");

  // This will send the request to the server
  client.print("GET ");
  client.print(url);
  client.print(" HTTP/1.1\r\n");
  client.print(String("Host: ") + hostName + "\r\nConnection: close\r\n\r\n");
  client.flush();

  console.print(INFO, "Sent request, client status: ");
  console.println(INFO, client.status());

  // TODO Determine if this delay is necessary, or if I can just loop checking for status is still ESTABLISHED, and waiting until
  // available is true.
  delay(500);

  // Read all the lines of the reply from server and print them to Serial.
  // TODO Parse the server timestamp, and make it available for computing when we
  // should next report (e.g. if prefer to report on the hour, then try to move to reporting such that the offset is tiny).
  while(client.available()){
    String line = client.readStringUntil('\r');
    console.print(INFO, line);
  }

  console.println(INFO);
  console.print(INFO, "Status: ");
  console.println(INFO, client.status());
  console.print(INFO, "Closing connection...");
  client.stop();
  console.println(INFO, "  disconnected");
}

class Reporter {
 public:
  Reporter() {}

  void Init() {
    console.println(INFO, "Initializing HTU21D communication");
    
    Wire.begin(4,5);  // On Adafruit Feather HUZZAH ESP8266, pins 4 and 5 are SDA and SDC.
    if (!htu_.begin()) {
      console.println(INFO, "Couldn't find Temperature and Humidity Sensor!");
      while (1);  // TODO Come up with a better death signal/response
                  // (e.g. light up Red LED as SOS, and blue with an error code).
    }
    console.println(INFO, "Connected to Temperature and Humidity Sensor");
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
    console.print(INFO, "Temp: "); console.print(INFO, temperature_);
    console.print(INFO, "\t\tRH(raw): "); console.print(INFO, humidity_);
    console.print(INFO, "\t\tRH(comp): "); console.print(INFO, compensated_);
    console.print(INFO, "\t\tVcc: "); console.print(INFO, vcc_);
    console.println(INFO);
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
  unsigned long end_time;
  Serial.begin(115200);
  pinMode(RED_LED, INPUT);
  pinMode(BLUE_LED, INPUT);

  // Give serial a bit of time (not sure if needed).
//  delay(100);
  console.println(INFO);
  console.print(INFO, "Start time: ");
  console.println(INFO, start_time);

  // Connect to HTU21D-F Temperature and Humidity Sensor.
  start_time = millis();
  reporter.Init();
  end_time = millis();
  console.print(INFO, "reporter.Init() took: ");
  console.println(INFO, end_time - start_time);

  // Measure.
  start_time = millis();
  reporter.Measure();
  end_time = millis();
  console.print(INFO, "reporter.Measure() took: ");
  console.println(INFO, end_time - start_time);
  reporter.PrintMeasurements();

  // Connect to wi-fi base station.
  start_time = millis();
  ConnectToWiFi();
  end_time = millis();
  console.print(INFO, "ConnectToWiFi() took: ");
  console.println(INFO, end_time - start_time);

  start_time = end_time;
  reporter.ReportMeasurements();
  end_time = millis();
  console.print(INFO, "ReportMeasurements() took: ");
  console.println(INFO, end_time - start_time);

  EnterDeepSleep();
}

void loop() {
  console.println(RARE, "SHOULD NOT REACH LOOP!");
}

