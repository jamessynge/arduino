/* Reports temperature, humidity and Vcc voltage to io.adafruit.com.
 * 
 * Sequence:
 * 1) Set SENSOR_POWER to OUTPUT HIGH.
 * 2) Connect to sensor.
 * 3) Start connecting to AP.
 * 4) Measure temperature, humidity and Vcc voltage.
 * 5) Set SENSOR_POWER to INPUT.
 * 6) Wait until connected to AP.
 * 7) Report measurements.
 * 8) Enter deepSleep.
 *
 * Change Log:
 * 2015-12-25a:       Starting from sketch_dec19a_gpio_enable_sensor, send
 *                    report to io.adafruit.com so that the laptop doesn't
 *                    need to be running.
 * 
 * 2015-12-19a:       Starting from sketch_dec17b_connect_and_measure_together.
 *                    First tried powering sensor via its 3V3 pin instead of VIN,
 *                    which skips its redundant regulator; appeared to work fine.
 *                    Then adding powering of the sensor via a raising an OUTPUT
 *                    pin HIGH, so it is hopefully unpowered most of the time.
 * 
 * 2015-12-17b:       Connect to sensor, start AP connect, measure, check connected.
 *                    (Doesn't work to start AP connect, then connect to sensor.)
 *                    Reduces total awake time to around 10 seconds, most of which
 *                    is the time to connect to the AP.
 * 2015-12-17a:       Put measurements before radio start.
 *                    Replace Serial.print*(...) with console.print*(INFO, ...)
 *                    so that I can control the amount of printing.
 * 2015-12-14:        Deep sleep; GPIO 16 to RST.
 * 2015-12-13 11:14:  Refactor to pull temperature & humidity measurement out, etc.
 * 
 * TODO Come up with voltage divider for measuring BAT voltage, hook up to
 * an available GPIO pin; set pin to OUTPUT LOW when measuring;
 * at other times set pin to INPUT, else OUTPUT HIGH.
 *
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
#define SENSOR_POWER 15

// TODO Store params, especially password, in EEPROM instead of in code.
const char* ssid     = "stargate";
const char* password = "sarahanneyakersynge";

const char* hostAddr = nullptr;
const char* hostName = "hygromote.appspot.com";
const int hostPort = 80;

// The clock used during deep sleep seems to run fast, and unfortunately not strictly linearly.
// At 5 minutes it is about 3% fast, and at 60 minutes around 5% fast.
const int report_interval_secs = (2 * 60) * 1.03;
// const int report_interval_secs = (60 * 60) * 1.05;

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

#define WIFI_DEBUG 0
#define DEBUG 1
#define INFO 2
#define RARE 3

class LevelPrinter {
 public:
  LevelPrinter() {
    setLevel(INFO);
  }

  void setLevel(byte level) {
    curr_level_ = level;
    Serial.setDebugOutput(level <= DEBUG);
  }

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

  void printDurationMs(byte level, unsigned long ms) {
    if (LevelIsOn(level)) {
      Serial.print(FormatDurationMs(ms));
    }
  }

  bool LevelIsOn(byte level) {
    return level >= curr_level_;
  }

 private:
  byte curr_level_;
};

LevelPrinter console;

// Use low power mode, which will reboot the ESP after the specified sleep period
// (in particular, will run this sketch from the beginning again).
void EnterDeepSleep() {
  unsigned long current_time = millis();
  console.print(DEBUG, "Current time: ");
  console.printDurationMs(DEBUG, current_time);
  console.print(DEBUG, "  (");
  console.print(DEBUG, current_time);
  console.println(DEBUG, ')');

  unsigned long wake_time = report_interval_secs * 1000;
  if (wake_time < current_time) {
    wake_time = current_time + 1;
  }
  unsigned long sleep_ms = wake_time - current_time;
  console.print(DEBUG, "Sleep time: ");
  console.printDurationMs(DEBUG, sleep_ms);
  console.print(DEBUG, "  (");
  console.print(DEBUG, sleep_ms);
  console.println(DEBUG, ')');

  unsigned long sleep_us = sleep_ms * 1000;
  console.print(INFO, "Requesting deep sleep mode for ");
  console.printDurationMs(INFO, sleep_ms);
  console.println(INFO);
  ESP.deepSleep(sleep_us, WAKE_RF_DISABLED);
  // Wait a little while because the ESP8266 takes a while to enter low power mode after that call.
  if (console.LevelIsOn(DEBUG)) {
    while (true) {
      delay(1);
      console.print(DEBUG, ".");
    }
  } else {
    console.println(INFO);
    delay(10000);
  }
}

void StartConnectingToWiFi() {
  if (console.LevelIsOn(WIFI_DEBUG)) {
    console.print(WIFI_DEBUG, "wifi_get_opmode() -> ");
    console.println(WIFI_DEBUG, wifi_get_opmode());
  }

  console.print(INFO, "Starting to connect to WiFi access point ");
  console.println(INFO, ssid);
  WiFi.begin(ssid, password);
}

void WaitUntilConnectedToWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    console.print(INFO, "Waiting until connected to WiFi");
    do {
      delay(500);
      console.print(INFO, ".");
    } while (WiFi.status() != WL_CONNECTED);
    console.println(INFO);
  }
  
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
  console.print(DEBUG, "Client created, client status: ");
  console.println(DEBUG, client.status());

  console.print(INFO, "Connecting to http://");
  console.print(INFO, hostAddr);
  console.print(INFO, ":");
  console.print(INFO, httpPort);
  console.println(INFO, url);
  unsigned long start_time = millis();
  if (!client.connect(hostAddr, httpPort)) {
    unsigned long duration = millis() - start_time;
    console.print(RARE, "TCP connect() failed after ");
    console.printDurationMs(RARE, duration);
    console.println(RARE);
    return;
  }

  console.print(DEBUG, "Connected, client status: ");
  console.println(DEBUG, client.status());

  // We now create a URI for the request
  console.println(INFO, "Connected to server, sending GET request");

  // This will send the request to the server
  client.print("GET ");
  client.print(url);
  client.print(" HTTP/1.1\r\n");
  client.print(String("Host: ") + hostName + "\r\nConnection: close\r\n\r\n");
  client.flush();

  console.print(DEBUG, "Sent request, client status: ");
  console.println(DEBUG, client.status());

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

  console.println(DEBUG);
  console.print(DEBUG, "Status: ");
  console.println(DEBUG, client.status());
  console.print(INFO, "Closing TCP connection...");
  client.stop();
  console.println(INFO, "  disconnected");
}

class Reporter {
 public:
  Reporter() {}

  bool Init() {
    console.println(DEBUG, "Initializing HTU21D communication");
    // Provide power via a GPIO pin. EXPERIMENT on 2015-12-19.
    pinMode(SENSOR_POWER, OUTPUT);
    digitalWrite(SENSOR_POWER, HIGH);
    delay(1);

    for (int loop = 0; loop < 20; ++loop) {
      Wire.begin(4,5);  // On Adafruit Feather HUZZAH ESP8266, pins 4 and 5 are SDA and SDC.
      if (htu_.begin()) break;
      console.println(RARE, "Couldn't find Temperature and Humidity Sensor!");
      delay(100);
    }
    if (!htu_.begin()) {
      return false;
    }
    console.println(INFO, "Connected to Temperature and Humidity Sensor");
    return true;
  }

  void Measure() {
    const int loops = 16;
    temperature_ = 0;
    humidity_ = 0;
    vcc_ = 0;
    for (int loop = 0; loop < loops; ++loop) {
      temperature_ += htu_.readTemperature();
      yield();
      humidity_ += htu_.readHumidity();
      yield();
      vcc_ += ESP.getVcc();
      yield();
    }
    digitalWrite(SENSOR_POWER, LOW);
    pinMode(SENSOR_POWER, INPUT);

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
    String url("/.record-data?sensor=rs1&temperature=");
    url += temperature_;
    url += "&humidity=";
    url += humidity_;
    url += "&compensated=";
    url += compensated_;
    url += "&vcc=";
    url += vcc_;
    GETtoSerial(hostAddr, hostName, hostPort, url);
  }

 private:
  float temperature_;  // Degrees C
  float humidity_;     // Percent Relative Humidity
  float compensated_;  // Percent Relative Humidity (temperature adjusted).
  float vcc_;
  Adafruit_HTU21DF htu_;
};

Reporter reporter;

void setup() {
  unsigned long start_time = millis();
  unsigned long end_time;
  Serial.begin(115200);
  pinMode(RED_LED, INPUT);
  pinMode(BLUE_LED, INPUT);

  console.println(INFO);
  console.print(DEBUG, "Start time: ");
  console.println(DEBUG, start_time);

  // Connect to HTU21D-F Temperature and Humidity Sensor.
  start_time = millis();
  reporter.Init();
  end_time = millis();
  console.print(DEBUG, "reporter.Init() took: ");
  console.println(DEBUG, end_time - start_time);

  // Connect to wi-fi base station.
  start_time = millis();
  StartConnectingToWiFi();
  end_time = millis();
  console.print(DEBUG, "StartConnectingToWiFi() took: ");
  console.println(DEBUG, end_time - start_time);

  // Measure.
  start_time = millis();
  reporter.Measure();
  end_time = millis();
  console.print(DEBUG, "reporter.Measure() took: ");
  console.println(DEBUG, end_time - start_time);
  reporter.PrintMeasurements();

  // Connect to wi-fi base station.
  start_time = millis();
  WaitUntilConnectedToWiFi();
  end_time = millis();
  console.print(DEBUG, "WaitUntilConnectedToWiFi() took: ");
  console.println(DEBUG, end_time - start_time);

  start_time = end_time;
  reporter.ReportMeasurements();
  end_time = millis();
  console.print(DEBUG, "reporter.ReportMeasurements() took: ");
  console.println(DEBUG, end_time - start_time);

  EnterDeepSleep();
}

void loop() {
  console.println(RARE, "SHOULD NOT REACH LOOP!");
}

