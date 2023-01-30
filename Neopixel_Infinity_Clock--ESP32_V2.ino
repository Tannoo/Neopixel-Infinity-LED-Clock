/*
 *  ««« Neopixel Infinity Clock »»»
 *
 *  - Over the air update capability
 *  - Nation Time Protocol server polling to update the RTC
 *  - Real Time Clock for accurate time keeping 
 *  - Weather polling for current temperature to set the background color (need to supply API key)
 *  - Strip brightness dims over the evening hours and brightens over the morning hours
 *  - OTA updates stop the time operations and sets strip from green to red on progress
 *  - OTA default passcode is 0000
 *
 *  CPU: Adafruit Feather32 V2 (ESP32)
 */

// ESP Feather32 setting up the other core
TaskHandle_t timers;
const int heartbeatLED = 13;
bool heartbeat = LOW;

// >>>>>>> Time libraries <<<<<<<
// ******************************
#include <TimeLib.h>  // For Date/Time operations - Time library by Michael Margolis v1.6.1
bool stopTime = false; // OTA purpose
#define TIMEDELAY 889.0

// >>>>>>> WIFI libraries <<<<<<<
// ******************************
#include <WiFi.h>         // For WiFi library -- unknown source
#include <WiFiUdp.h>      // For UDP NTP
#include <WiFiManager.h>  // Help with configuration of a wifi connection
WiFiManager wm;

// >>>>>>> OTA Stuff <<<<<<<
// *************************
#include <ArduinoOTA.h>

// >>>>>>> RTC Stuff <<<<<<<
// *************************
#include <RTClib.h>  // Library for RTC stuff
RTC_DS3231 RTC;      // Setup an instance of DS1307 naming it RTC
float RTCtemp;

// >>>>>>> Weather libraries <<<<<<<
// *********************************
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>

// Weather timer
uint32_t previousWeatherMillis = 0;
uint32_t weatherUpdate = 240000;
double temperature_K;
// Time timer
uint32_t previousTimeMillis = 0;
uint32_t timeUpdate = 60000;
// NTP timer
uint32_t previousNTPMillis = 0;
uint32_t NTPUpdate = 600000;
// OTA_beat timer
uint32_t previousHeartbeatMillis = 0;
uint32_t heartUpdate = 1000;

// Temperature setup
double temperature;
uint8_t R_ed = 20;    // Background Red set by the weather
uint8_t G_reen = 20;  // Background Green set by the weather
uint8_t B_lue = 20;   // Background Blue set by the weather

// Register for a key @ https://openweathermap.org/api
String openWeatherMapApiKey = "*****************************";
String city = "********";

// "standard" = Kelvin, "imperial" = Fahrenheit, or "metric" = Celsius (will show up in serial data)
String temp_units = "imperial";

String jsonBuffer;

// >>>>>>> NEOPIXEL library and Stuff <<<<<<<
// ******************************************
#include <Adafruit_NeoPixel.h>  // Neopixel library by Adafruit

#define NEOPIN 15
#define NUMPIXELS 60
#define NEOPIXEL_TYPE NEO_GRBW

#define MORNING 5
#define EVENING 17
#define MIN_BRIGHTNESS 50
uint8_t brightness;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, NEOPIN, NEOPIXEL_TYPE + NEO_KHZ800);

byte neopix_gamma[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2,
  2, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5,
  5, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10,
  10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
  17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
  25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
  37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
  51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
  69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
  90, 92, 93, 95, 96, 98, 99, 101, 102, 104, 105, 107, 109, 110, 112, 114,
  115, 117, 119, 120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142,
  144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175,
  177, 180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213,
  215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252, 255
};

#define CHCOLOR1 neopix_gamma[j], 0, 0                                  // Red
#define CHCOLOR2 neopix_gamma[j], 0, 0, neopix_gamma[j]                 // Lt. Red
#define CHCOLOR3 neopix_gamma[j], neopix_gamma[j], 0                    // Yellow
#define CHCOLOR4 0, neopix_gamma[j], 0                                  // Green
#define CHCOLOR5 0, neopix_gamma[j], 0, neopix_gamma[j]                 // Lt. Green
#define CHCOLOR6 0, neopix_gamma[j], neopix_gamma[j]                    // Teal
#define CHCOLOR7 0, 0, neopix_gamma[j]                                  // Blue
#define CHCOLOR8 0, 0, neopix_gamma[j], neopix_gamma[j]                 // Lt. Blue
#define CHCOLOR9 neopix_gamma[j], 0, neopix_gamma[j]                    // Purple
#define CHCOLOR10 neopix_gamma[j], neopix_gamma[j], neopix_gamma[j]     // RGB White
#define CHCOLOR11 neopix_gamma[j], 0, neopix_gamma[j], neopix_gamma[j]  // Lt. Purple
#define CHCOLOR12 0, neopix_gamma[j], neopix_gamma[j], neopix_gamma[j]  // Lt. Teal

/*
  h  = LED positional hour
  hr = RTC base hour
  m  = LED positional minute
  s  = LED positional second
  ms = LED positional millisecond
  Others are for LED fading operations
*/
uint8_t ms, s, m, mf, hf, h, hr;
uint8_t oldsec, old_mf, oldmin, old_hf, old_hr;
uint8_t m_backgnd_fade, h_backgnd_fade;
uint8_t oldm_backgnd_fade, oldh_backgnd_fade;
uint8_t old_brightness;

#define H_COLOR hf, h_backgnd_fade, h_backgnd_fade
#define OLD_H_COLOR old_hf, oldh_backgnd_fade, oldh_backgnd_fade
#define M_COLOR m_backgnd_fade, mf, m_backgnd_fade
#define OLD_M_COLOR oldm_backgnd_fade, old_mf, oldm_backgnd_fade
#define MH_COLOR hf, mf, h_backgnd_fade
#define OLD_MH_COLOR old_hf, old_mf, oldh_backgnd_fade
#define S_COLOR 0, 0, 255
#define SM_COLOR m_backgnd_fade, mf, 255
#define SH_COLOR hf, h_backgnd_fade, 255
#define OLD_SM_COLOR oldm_backgnd_fade, old_mf, 255
#define OLD_SH_COLOR old_hf, oldh_backgnd_fade, 255
#define MS_COLOR 255, 255, 255
#define BACKGND R_ed, G_reen, B_lue

// NTP Servers:
IPAddress timeServer;
static const char ntpServerName[] = "us.pool.ntp.org";
uint8_t timeZone = -7;
uint8_t NTPDayOffset = 10;
uint8_t NTPHourOffset = 16;
bool DST = false;
time_t prevDisplay = 0;
uint8_t localPort = 8888;
const int NTP_PACKET_SIZE = 48;
byte packetBuffer[NTP_PACKET_SIZE];

WiFiUDP Udp;
time_t getNtpTime();

// <<<<*** SETUP ***>>>>
void setup() {

  // Creating the task for the other core
  xTaskCreatePinnedToCore(
    timedtasks,  // Function to implement the task
    "timers",    // Name of the task
    10000,       // Stack size in words
    NULL,        // Task input parameter
    0,           // Priority of the task
    &timers,     // Task handle
    0);          // Core where the task should run

  // Generate random seed
  uint32_t seed = 0;
  for (uint8_t i = 10; i; i--) {
    seed = (seed << 5) + (analogRead(A2) * 3);
  }
  randomSeed(seed);  //set random seed

  Serial.begin(115200);
  Serial.println();
  Serial.println(F("Starting up..."));

  // heartbeat LED
  pinMode(heartbeatLED, OUTPUT);

  // Start the NEOPIXEL strip
  pinMode(NEOPIN, OUTPUT);
  strip.begin();
  strip.show();

  // >>>>>>> Start WiFi Manager <<<<<<<
  // **********************************
  bool res;
  Serial.print(F("Connecting to wifi via WifiManager... "));
  //wm.resetSettings(); // Wipe settings for experimentation
  wm.setConnectTimeout(60);
  // This is the SSID and PASSWORD to connect and configure the wifi to run with
  res = wm.autoConnect("INFINITY_CLOCK_AP", "password");
  if (!res) Serial.println(F("Failed to connect"));
  else Serial.println(F("Connected...yeah! :)"));

  // >>>>>>> Start OTA Stuff <<<<<<<
  // *******************************
  ArduinoOTA.setPassword("0000");  // OTA password

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else  // U_SPIFFS
      type = "filesystem";
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println(F("\nEnd"));
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    // While uploading, stop the time and change the LEDs from Green to Red
    stopTime = true;
    R_ed   = map(progress / (total / 100), 0, 100, 255, 0);
    G_reen = map(progress / (total / 100), 0, 100, 0, 255);
    B_lue  = 0;
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println(F("Auth Failed"));
    else if (error == OTA_BEGIN_ERROR) Serial.println(F("Begin Failed"));
    else if (error == OTA_CONNECT_ERROR) Serial.println(F("Connect Failed"));
    else if (error == OTA_RECEIVE_ERROR) Serial.println(F("Receive Failed"));
    else if (error == OTA_END_ERROR) Serial.println(F("End Failed"));
  });
  stopTime = false;
  ArduinoOTA.begin();
  Serial.println();
  Serial.println(F("OTA is ready"));
  Serial.print(F("IP address: "));
  Serial.println(WiFi.localIP());
  // END OTA STUFF

  // >>>>>>> Start NTP Stuff <<<<<<<
  // *******************************
  Serial.println(F("Starting UDP"));
  Udp.begin(localPort);
  Serial.print(F("Local port: "));
  Serial.println(localPort);
  Serial.println(F("Waiting for sync"));
  setSyncProvider(getNtpTime);
  setSyncInterval(5000);
  time_t prevDisplay = 0;

  // >>>>>>> Start RTC Stuff <<<<<<<
  // *******************************
  Wire.begin();  // Start the I2C
  RTC.begin();   // Init RTC

  // Time and date is expanded to date and time on your computer at compile time
  RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));
  Serial.print(F("Compile time and date set to: "));
  Serial.print(F(__DATE__));
  Serial.print(F(" "));
  Serial.println(F(__TIME__));

  // >>>>>>> Start Demo / Pixel test <<<<<<<
  // ***************************************
  Serial.print(F("Pixel Color Test... "));
  Serial.println(F("White Over Rainbow"));
  whiteOverRainbow(20, 25, 5);

  Serial.println();
  Serial.println(F("Neopixel Clock..."));
  delay(500);

  DateTime now = RTC.now();
  Serial.print(F("now.hour(): "));
  Serial.print(now.hour());
  if (now.hour() >= EVENING + 1 || now.hour() <= MORNING - 1) brightness = MIN_BRIGHTNESS;
  else brightness = 255;
  Serial.print(F(" - Setting the brightness to "));
  Serial.println(brightness);
  strip.setBrightness(brightness);

  displaySerialTime();

  fadeOnBackgnd(R_ed, G_reen, B_lue, 5);
  Serial.println(F("Running the Clock.. "));
}

// <<<<*** LOOP ***>>>>
void loop() {
  ArduinoOTA.handle();

  if (stopTime == false) {
    timeOperations();
  }
  // Set all LEDs to the background color during OTA updates
  if (stopTime == true) {
    for (int k = 0; k <= NUMPIXELS; k ++) {
      strip.setPixelColor(k, strip.Color(BACKGND));
    }
    strip.show();  // Set the LEDs
  }
}

void timeOperations() {
  oldmin = m;
  oldsec = s;

  DateTime now = RTC.now();

  if (now.hour() >= 12) hr = now.hour() - 12;
  else hr = now.hour();

  h = map(m, 0, NUMPIXELS - 1, hr * (NUMPIXELS / 12), (hr * (NUMPIXELS / 12)) + ((NUMPIXELS / 12) - 1));
  m = now.minute();
  s = now.second();

  // Chime
  if (old_hr != hr && m == 0 && s == 0) {
    displaySerialTime();
    Serial.print(F(" | old_hr: "));
    Serial.println(old_hr);
    FadeOffBackgnd(R_ed, G_reen, B_lue, 2);
    delay(200);
    hourchime(3);
  }

  // ********************
  //      Time Stuff
  // ********************

  // Map and set the fading minute colors
  mf = map(s, 0, NUMPIXELS - 1, G_reen, 255);
  old_mf = map(s, 0, NUMPIXELS - 1, 255, G_reen);

  // Map and fade the minute backgroud intensities
  m_backgnd_fade = map(s, 0, NUMPIXELS - 1, G_reen, 0);
  oldm_backgnd_fade = map(s, 0, NUMPIXELS - 1, 0, G_reen);

  // Map and set the fading hour colors.
  hf = map(m, 0, NUMPIXELS - 1, G_reen, 255);
  old_hf = map(m, 0, NUMPIXELS - 1, 255, G_reen);

  // Map and fade the hour backgroud intensities
  h_backgnd_fade = map(m, 0, NUMPIXELS - 1, G_reen, 0);
  oldh_backgnd_fade = map(m, 0, NUMPIXELS - 1, 0, G_reen);

  // Let's place (set) the LEDs
  for (ms = 0; ms <= NUMPIXELS - 1; ms++) {
    // Set the millisecond color
    strip.setPixelColor(ms, strip.Color(MS_COLOR));
    // Set hour color at #1 position
    if (h + 1 >= NUMPIXELS) strip.setPixelColor(0, strip.Color(H_COLOR));
    // Set hour color
    else strip.setPixelColor(h + 1, strip.Color(H_COLOR));
    // Set the old hour color
    strip.setPixelColor(h, strip.Color(OLD_H_COLOR));
    // Set the minute color at #1 position
    if (m + 1 >= NUMPIXELS) strip.setPixelColor(0, strip.Color(M_COLOR));
    // Set the minute color
    else strip.setPixelColor(m + 1, strip.Color(M_COLOR));
    // Set the old minute color
    strip.setPixelColor(m, strip.Color(OLD_M_COLOR));
    if (s == m)
      // Set the combined second - minute color
      strip.setPixelColor(s, strip.Color(SM_COLOR));
    else if (s == m + 1)
      // Set the combined second - minute color
      strip.setPixelColor(s, strip.Color(OLD_SM_COLOR));
    else if (s == h)
      // Set the combined second - hour color
      strip.setPixelColor(s, strip.Color(SH_COLOR));
    else if (s == h + 1)
      // Set the combined second - hour color
      strip.setPixelColor(s, strip.Color(OLD_SH_COLOR));
    // Set the second color
    else strip.setPixelColor(s, strip.Color(S_COLOR));
    if (m == h)
      // Set the minute and hour combined color
      strip.setPixelColor(m, strip.Color(OLD_MH_COLOR));
    if (m == h + 1)
      // Set the minute and hour combined color
      strip.setPixelColor(m, strip.Color(MH_COLOR));
    if (m + 1 == h || m + 1 == h + 1)
      // Set the minute and hour combined color at #1 position
      if (m + 1 >= NUMPIXELS) strip.setPixelColor(0, strip.Color(MH_COLOR));
      // Set the minute and hour combined color
      else strip.setPixelColor(m + 1, strip.Color(MH_COLOR));

    // Set the LEDs
    strip.show();
    // Change the ms delay for secs to match the LEDs
    delay(TIMEDELAY / NUMPIXELS);

    // Setting the LED and background colors after a specific delay
    // Set the ms to the background color
    strip.setPixelColor(ms, strip.Color(BACKGND));
    // Set the old second to the background color
    strip.setPixelColor(oldsec, strip.Color(BACKGND));
    // Set the old hour color
    strip.setPixelColor(h, strip.Color(OLD_H_COLOR));
    // Set hour color at #1 position
    if (h + 1 >= NUMPIXELS) strip.setPixelColor(0, strip.Color(H_COLOR));
    // Set hour color
    else strip.setPixelColor(h + 1, strip.Color(H_COLOR));
    // Set the old minute color
    strip.setPixelColor(m, strip.Color(OLD_M_COLOR));
    // Set the minute color at #1 position
    if (m + 1 >= NUMPIXELS) strip.setPixelColor(0, strip.Color(M_COLOR));
    // Set the minute color
    else strip.setPixelColor(m + 1, strip.Color(M_COLOR));
    if (s == m)
      // Set the combined second - minute color
      strip.setPixelColor(s, strip.Color(OLD_SM_COLOR));
    else if (s == m + 1)
      // Set the combined second - minute color
      strip.setPixelColor(s, strip.Color(SM_COLOR));
    else if (s == h)
      // Set the combined second - hour color
      strip.setPixelColor(s, strip.Color(OLD_SH_COLOR));
    else if (s == h + 1)
      // Set the combined second - hour color
      strip.setPixelColor(s, strip.Color(SH_COLOR));
    // Set the second color
    else strip.setPixelColor(s, strip.Color(S_COLOR));
    if (m == h)
      // Set the minute and hour combined color
      strip.setPixelColor(m, strip.Color(OLD_MH_COLOR));
    if (m == h + 1)
      // Set the minute and hour combined color
      strip.setPixelColor(m, strip.Color(MH_COLOR));
    if (m + 1 == h || m + 1 == h + 1)
      // Set the minute and hour combined color at #1 position
      if (m + 1 >= NUMPIXELS) strip.setPixelColor(0, strip.Color(MH_COLOR));
      // Set the minute and hour combined color
      else strip.setPixelColor(m + 1, strip.Color(MH_COLOR));
  }
  strip.show();  // Set the LEDs
  set_Brightness();
}

void hourchime(uint8_t wait) {
  old_hr = hr;
  Serial.print(F("Hour Chimes.. "));
  int chmhr;
  if (hr == 0) chmhr = 12;
  else chmhr = hr;
  for (int m = 1; m <= chmhr; m++) {
    if (m > 1) Serial.print(F(" - "));
    Serial.print(m);
    for (uint16_t i = 0; i <= NUMPIXELS - 1; i++) {
      if (m == 1) strip.setPixelColor(i, PSTR(strip.Color(255, 0, 0, 0)));       // Red
      if (m == 2) strip.setPixelColor(i, PSTR(strip.Color(255, 0, 0, 255)));     // LT Red
      if (m == 3) strip.setPixelColor(i, PSTR(strip.Color(255, 255, 0, 0)));     // Yellow
      if (m == 4) strip.setPixelColor(i, PSTR(strip.Color(0, 255, 0, 0)));       // Green
      if (m == 5) strip.setPixelColor(i, PSTR(strip.Color(0, 255, 0, 255)));     // LT Green
      if (m == 6) strip.setPixelColor(i, PSTR(strip.Color(0, 255, 255, 0)));     // Teal
      if (m == 7) strip.setPixelColor(i, PSTR(strip.Color(0, 0, 255, 0)));       // Blue
      if (m == 8) strip.setPixelColor(i, PSTR(strip.Color(0, 0, 255, 255)));     // LT Blue
      if (m == 9) strip.setPixelColor(i, PSTR(strip.Color(255, 0, 255, 0)));     // Purple
      if (m == 10) strip.setPixelColor(i, PSTR(strip.Color(MS_COLOR)));          // RGB White
      if (m == 11) strip.setPixelColor(i, PSTR(strip.Color(255, 0, 255, 255)));  // LT Purple
      if (m == 12) strip.setPixelColor(i, PSTR(strip.Color(0, 255, 255, 255)));  // LT Teal
    }
    strip.show();
    delay(wait);
    for (int j = 255; j >= 0; j--) {
      for (uint16_t i = 0; i <= NUMPIXELS - 1; i++) {
        if (m == 1) strip.setPixelColor(i, PSTR(strip.Color(CHCOLOR1)));
        if (m == 2) strip.setPixelColor(i, PSTR(strip.Color(CHCOLOR2)));
        if (m == 3) strip.setPixelColor(i, PSTR(strip.Color(CHCOLOR3)));
        if (m == 4) strip.setPixelColor(i, PSTR(strip.Color(CHCOLOR4)));
        if (m == 5) strip.setPixelColor(i, PSTR(strip.Color(CHCOLOR5)));
        if (m == 6) strip.setPixelColor(i, PSTR(strip.Color(CHCOLOR6)));
        if (m == 7) strip.setPixelColor(i, PSTR(strip.Color(CHCOLOR7)));
        if (m == 8) strip.setPixelColor(i, PSTR(strip.Color(CHCOLOR8)));
        if (m == 9) strip.setPixelColor(i, PSTR(strip.Color(CHCOLOR9)));
        if (m == 10) strip.setPixelColor(i, PSTR(strip.Color(CHCOLOR10)));
        if (m == 11) strip.setPixelColor(i, PSTR(strip.Color(CHCOLOR11)));
        if (m == 12) strip.setPixelColor(i, PSTR(strip.Color(CHCOLOR12)));
      }
      delay(wait);
      strip.show();
    }
  }
  Serial.println();
  Serial.println(F("End of chime"));
  Serial.println();
  fadeOnBackgnd(R_ed, G_reen, B_lue, 5);
}

void set_Brightness() {
  DateTime now = RTC.now();
  // Brighten the strip morning
  if (now.hour() == MORNING && OM != now.minute()) {
    brightness = map(now.minute(), 0, 59, MIN_BRIGHTNESS, 127);
    Serial.print(F("Adjusting first morning brightness: "));
    Serial.println(brightness);
  }
  if (now.hour() == MORNING + 1 && OM != now.minute()) {
    brightness = map(now.minute(), 0, 59, 127, 255);
    Serial.print(F("Adjusting second morning brightness: "));
    Serial.println(brightness);
  }
  // Dim the strip in the evening
  if (now.hour() == EVENING && OM != now.minute()) {
    brightness = map(now.minute(), 0, 59, 255, 127);
    Serial.print(F("Adjusting first evening brightness: "));
    Serial.println(brightness);
  }
  if (now.hour() == EVENING + 1 && OM != now.minute() ) {
    brightness = map(now.minute(), 0, 59, 127, MIN_BRIGHTNESS);
    Serial.print(F("Adjusting second evening brightness: "));
    Serial.println(brightness);
  }
  OM = now.minute();
}

void digitalClockDisplay() {
  now();
  DateTime now = RTC.now();
  rtcTime();
  ntpTime();

  if (now.minute() != minute()) {
    Serial.print(F("RTC (m): "));
    Serial.print(now.minute());
    Serial.print(F(" vs NTP (m): "));
    Serial.println(minute());
    //Serial.println(F("Updating the RTC from NTP time has been disabled due to inconsistant time from NTP."));
    Serial.println(F("Updating the RTC from NTP time"));
    RTC.adjust(DateTime(year(), month(), day(), hour(), minute(), second()));
    Serial.print(F("Adjustment done »---> "));
    rtcTime();
  }

  if (old_brightness != brightness) {
    old_brightness = brightness;
    Serial.print("Setting the brightness to " + brightness);
    strip.setBrightness(brightness);
  }

  Serial.print(F("old_brightness: "));
  Serial.print(old_brightness);
  Serial.print(F(" vs brightness: "));
  Serial.println(brightness);

  m = now.minute();
  s = now.second();
  old_hr = hr;
  oldmin = m;
  oldsec = s;
}

void rtcTime() {
  DateTime now = RTC.now();
  RTCtemp = RTC.getTemperature();

  // Date and Time from RTC
  Serial.print(F("RTC Time: "));
  printDigits(now.month());
  Serial.print(F("/"));
  printDigits(now.day());
  Serial.print(F("/"));
  printDigits(now.year());
  Serial.print(F(" "));
  printDigits(now.hour());
  Serial.print(F(":"));
  printDigits(now.minute());
  Serial.print(F(":"));
  printDigits(now.second());
  Serial.print(F(" <<«»>> "));
  Serial.print(F("RTC Temp: "));
  Serial.print((RTCtemp * 9 / 5) + 32);
  Serial.print(F("°F"));
  Serial.println();
}

void ntpTime() {
  // NTP Time
  now();
  Serial.print(F("NTP Time: "));
  printDigits(month());
  Serial.print(F("/"));
  printDigits(day());
  Serial.print(F("/"));
  printDigits(year());
  Serial.print(F(" "));
  Serial.print(hour());
  Serial.print(F(":"));
  printDigits(minute());
  Serial.print(F(":"));
  printDigits(second());
  Serial.println();
}

void printDigits(int digits) {
  // utility for digital clock display: prints preceding colon and leading 0
  if (digits < 10)
    Serial.print(F("0"));
  Serial.print(digits);
}

uint32_t NTPOffset = (NTPDayOffset * 86400) + (NTPHourOffset * 3600);

time_t getNtpTime() {
  IPAddress ntpServerIP;  // NTP server's ip address

  while (Udp.parsePacket() > 0)
    ;  // Discard any previously received packets
  Serial.println(F("Transmit NTP Request"));
  // Get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(F(": "));
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println(F("Receive NTP Response"));
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // Read packet into the buffer
      unsigned long secsSince1900;
      // Convert four bytes starting at location 40 to a long integer
      secsSince1900 = (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - NTPOffset - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println(F("No NTP Response :-("));
  Serial.println();
  return 0;  // Return 0 if unable to get the time
}

// Send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress& address) {
  // Set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;  // LI, Version, Mode
  packetBuffer[1] = 0;           // Stratum, or type of clock
  packetBuffer[2] = 6;           // Polling Interval
  packetBuffer[3] = 0xEC;        // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // All NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123);  //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

void fadeOnBackgnd(uint8_t red, uint8_t green, uint8_t blue, uint8_t wait) {
  Serial.print(F("Background Colors: "));
  Serial.print(R_ed);
  Serial.print(F(" | "));
  Serial.print(G_reen);
  Serial.print(F(" | "));
  Serial.println(B_lue);
  Serial.println(F("Fading on the background..."));

  for (uint8_t b = 0; b < 255; b++) {
    for (uint8_t l = 0; l < NUMPIXELS; l++) {
      strip.setPixelColor(l, red * b / 60, green * b / 60, blue * b / 60);
    }
    strip.show();
    delay(wait);
    if (red * b / 75 >= red || green * b / 75 >= green || blue * b / 75 >= blue)
      break;
  }
}

void FadeOffBackgnd(uint8_t red, uint8_t green, uint8_t blue, uint8_t wait) {
  Serial.println();
  Serial.println(F("Fading off the background..."));

  for (uint8_t b = 255; b > 0; b--) {
    for (uint8_t i = 0; i < NUMPIXELS; i++) {
      strip.setPixelColor(i, red * b / 75, green * b / 75, blue * b / 75);
    }
    strip.show();
    delay(wait);
  }
}

void fadeColor(uint8_t start_red, uint8_t start_green, uint8_t start_blue, uint8_t end_red, uint8_t end_green, uint8_t end_blue, uint8_t wait) {
  Serial.println();
  Serial.println(F("Fading color of the background..."));

  uint8_t b = (start_red + start_green + start_blue) / 3;
  uint8_t c = (end_red + end_green + end_blue) / 3;

  if (b < c) {
    for (uint8_t b = b; b < c; b++) {
      if (h > 0 && m > 0 && s > 0) {  // If all are above 0, then start at zero
        if (s < h && s < m) {
          for (uint8_t l = 0; l < s; l++) {  // 0 --> s
            strip.setPixelColor(l, end_red * b / 60, end_green * b / 60, end_blue * b / 60);
          }
        }
        if (m < h && m < s) {
          for (uint8_t l = 0; l < m; l++) {  // 0 --> m
            strip.setPixelColor(l, end_red * b / 60, end_green * b / 60, end_blue * b / 60);
          }
        }
        if (h < m && h < s) {
          for (uint8_t l = 0; l < h; l++) {  // 0 --> h
            strip.setPixelColor(l, end_red * b / 60, end_green * b / 60, end_blue * b / 60);
          }
        }
      }
      if (s < m && s < h) {
        if (m < h) {
          for (uint8_t l = s + 1; l < m; l++) {  // (s + 1) --> m
            strip.setPixelColor(l, end_red * b / 60, end_green * b / 60, end_blue * b / 60);
          }
        }
        if (h < m) {
          for (uint8_t l = s + 1; l < h; l++) {  // (s + 1) --> h
            strip.setPixelColor(l, end_red * b / 60, end_green * b / 60, end_blue * b / 60);
          }
        }
      }
      if (m < h) {
        for (uint8_t l = m + 2; l < h; l++) {  // (m + 2) --> h
          strip.setPixelColor(l, end_red * b / 60, end_green * b / 60, end_blue * b / 60);
        }
        for (uint8_t l = h + 2; l < NUMPIXELS; l++) {  // (h + 2) --> NUMPIXELS
          strip.setPixelColor(l, end_red * b / 60, end_green * b / 60, end_blue * b / 60);
        }
      }
      if (h < m) {
        for (uint8_t l = h + 2; l < m; l++) {  // (h + 2) --> m
          strip.setPixelColor(l, end_red * b / 60, end_green * b / 60, end_blue * b / 60);
        }
        for (uint8_t l = m + 2; l < NUMPIXELS; l++) {  // (m + 2) --> NUMPIXELS
          strip.setPixelColor(l, end_red * b / 60, end_green * b / 60, end_blue * b / 60);
        }
      } else {
        for (uint8_t l = s + 1; l < NUMPIXELS; l++) {  // (s + 1) --> NUMPIXELS
          strip.setPixelColor(l, end_red * b / 60, end_green * b / 60, end_blue * b / 60);
        }
      }
      // Let's put all this into affect so we can see the effect
      strip.show();
      delay(wait);
      if (end_red * b / 75 >= end_red || end_green * b / 75 >= end_green || end_blue * b / 75 >= end_blue)
        break;
    }
  } else {
    for (uint8_t b = b; b > c; b--) {
      if (h > 0 && m > 0 && s > 0) {  // If all are above 0, then start at zero
        if (s < h && s < m) {
          for (uint8_t l = 0; l < s; l++) {  // 0 --> s
            strip.setPixelColor(l, end_red * b / 60, end_green * b / 60, end_blue * b / 60);
          }
        }
        if (m < h && m < s) {
          for (uint8_t l = 0; l < m; l++) {  // 0 --> m
            strip.setPixelColor(l, end_red * b / 60, end_green * b / 60, end_blue * b / 60);
          }
        }
        if (h < m && h < s) {
          for (uint8_t l = 0; l < h; l++) {  // 0 --> h
            strip.setPixelColor(l, end_red * b / 60, end_green * b / 60, end_blue * b / 60);
          }
        }
      }
      if (s < m && s < h) {
        if (m < h) {
          for (uint8_t l = s + 1; l < m; l++) {  // (s + 1) --> m
            strip.setPixelColor(l, end_red * b / 60, end_green * b / 60, end_blue * b / 60);
          }
        }
        if (h < m) {
          for (uint8_t l = s + 1; l < h; l++) {  // (s + 1) --> h
            strip.setPixelColor(l, end_red * b / 60, end_green * b / 60, end_blue * b / 60);
          }
        }
      }
      if (m < h) {
        for (uint8_t l = m + 2; l < h; l++) {  // (m + 2) --> h
          strip.setPixelColor(l, end_red * b / 60, end_green * b / 60, end_blue * b / 60);
        }
        for (uint8_t l = h + 2; l < NUMPIXELS; l++) {  // (h + 2) --> NUMPIXELS
          strip.setPixelColor(l, end_red * b / 60, end_green * b / 60, end_blue * b / 60);
        }
      }
      if (h < m) {
        for (uint8_t l = h + 2; l < m; l++) {  // (h + 2) --> m
          strip.setPixelColor(l, end_red * b / 60, end_green * b / 60, end_blue * b / 60);
        }
        for (uint8_t l = m + 2; l < NUMPIXELS; l++) {  // (m + 2) --> NUMPIXELS
          strip.setPixelColor(l, end_red * b / 60, end_green * b / 60, end_blue * b / 60);
        }
      } else {
        for (uint8_t l = s + 1; l < NUMPIXELS; l++) {  // (s + 1) --> NUMPIXELS
          strip.setPixelColor(l, end_red * b / 60, end_green * b / 60, end_blue * b / 60);
        }
      }
      // Let's put all this into affect so we can see the effect
      strip.show();
      delay(wait);
      if (end_red * b / 75 >= end_red || end_green * b / 75 >= end_green || end_blue * b / 75 >= end_blue)
        break;
    }
  }
}

void whiteOverRainbow(uint8_t wait, uint8_t whiteSpeed, uint8_t whiteLength) {
  if (whiteLength >= NUMPIXELS) whiteLength = NUMPIXELS - 1;
  uint8_t head = whiteLength - 1;
  uint8_t tail = 0;
  uint8_t loops = 3;
  uint8_t loopNum = 0;
  static unsigned long previousWhiteMillis = 0;

  while (true) {
    for (uint8_t j = 0; j < 256; j++) {
      for (uint16_t i = 0; i < NUMPIXELS; i++) {
        if ((i >= tail && i <= head) || (tail > head && i >= tail) || (tail > head && i <= head))
          strip.setPixelColor(i, strip.Color(MS_COLOR));
        else strip.setPixelColor(i, Wheel(((i * 256 / NUMPIXELS) + j) & 255));
      }
      if (millis() - previousWhiteMillis > whiteSpeed) {
        head++;
        tail++;
        if (head == NUMPIXELS) loopNum++;
        previousWhiteMillis = millis();
      }
      if (loopNum == loops) return;
      head %= NUMPIXELS;
      tail %= NUMPIXELS;
      strip.show();
      delay(wait);
    }
  }
}

// Input a value 0 to 255 to get a color value
// The colours are a transition r - g - b - back to r
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3, 0);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3, 0);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0, 0);
}

uint8_t red(uint32_t c) {
  return (c >> 16);
}
uint8_t green(uint32_t c) {
  return (c >> 8);
}
uint8_t blue(uint32_t c) {
  return (c);
}

void weather() {
  uint8_t OR = R_ed, OG = G_reen, OB = B_lue;
  displaySerialTime();
  String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "&appid=" + openWeatherMapApiKey;
  Serial.println();
  Serial.println(serverPath);
  Serial.println();
  jsonBuffer = httpGETRequest(serverPath.c_str());
  JSONVar myObject = JSON.parse(jsonBuffer);
  double temperature_K = (myObject["main"]["temp"]);
  Serial.println(F(" °K"));

  if (JSON.typeof(myObject) == "undefined") {
    Serial.println(F("Parsing input failed!"));
    return;
  }
  Serial.println();
  Serial.println(F("****************"));
  Serial.print(F("JSON object = "));
  Serial.println(myObject);
  Serial.println(F("****************"));
  Serial.println(F("extracted from JSON object:"));
  Serial.print(F("Temperature: "));
  Serial.print(myObject["main"]["temp"]);
  Serial.println(F(" °K"));

  // Scale the temperature to the red and blue colors
  Serial.print(F("Temperature: "));
  Serial.print(temperature);

  if (temp_units == "imperial") {
    temperature = (temperature_K - 273.15) * 9 / 5 + 32;  // F = (K - 273.15) * 9/5 + 32;
    R_ed = constrain(map(temperature, 0, 100, 0, 70), 0, 255);
    B_lue = constrain(map(temperature, 0, 100, 70, 0), 0, 255);
    Serial.print(F(" °F"));
  } else if (temp_units == "metric") {
    temperature = temperature_K - 273.15;  // C = K - 273
    R_ed = constrain(map(temperature, -17, 38, 0, 70), 0, 255);
    B_lue = constrain(map(temperature, -17, 38, 70, 0), 0, 255);
    Serial.print(F(" °C"));
  } else {
    temperature = temperature_K;  // K = K
    R_ed = constrain(map(temperature, 255.372, 310.928, 0, 70), 0, 255);
    B_lue = constrain(map(temperature, 255.372, 310.928, 70, 0), 0, 255);
    Serial.println(F(" °K"));
  }

  // Let's lighten up things if there is a good temp
  if (temperature_K >= 200)
    G_reen = 20;
  else {
    R_ed = 10;
    G_reen = 10;
    B_lue = 10;
  }

  Serial.print(F("old_Background value -  "));
  Serial.print(F("Red: "));
  Serial.print(OR);
  Serial.print(F(" | "));
  Serial.print(F("Green: "));
  Serial.print(OG);
  Serial.print(F(" | "));
  Serial.print(F("Blue: "));
  Serial.println(OB);

  Serial.print(F("Background value -  "));
  Serial.print(F("Red: "));
  Serial.print(R_ed);
  Serial.print(F(" | "));
  Serial.print(F("Green: "));
  Serial.print(G_reen);
  Serial.print(F(" | "));
  Serial.print(F("Blue: "));
  Serial.println(B_lue);

  if ((OR + OG + OB) != (R_ed + G_reen + B_lue))
    fadeColor(OR, OG, OB, R_ed, G_reen, B_lue, 10);
}

// *************************
//  http GET request module
// *************************
String httpGETRequest(const char* serverName) {

  WiFiClient client;
  HTTPClient http;

  http.begin(client, serverName);  // Your IP address with path or Domain name with URL path

  int httpResponseCode = http.GET();  // Send HTTP POST request
  String payload = "{}";
  if (httpResponseCode > 0) {
    Serial.print(F("HTTP Response code: "));
    Serial.println(httpResponseCode);
    payload = http.getString();
  } else {
    Serial.print(F("Error code: "));
    Serial.println(httpResponseCode);
    temperature = 10;  // Set the K temp back to default if no connection was made
  }
  http.end();  // Free resources
  return payload;
}

void displaySerialTime() {
  Serial.println();
  Serial.println(F("          ******************** "));
  Serial.print(F("          »» Time: "));
  if (hr < 10) Serial.print(F("0"));
  Serial.print(hr);
  printDigits(m);
  printDigits(s);
  Serial.println(F(" «« "));
  Serial.println(F("          ******************** "));
}

void timedtasks(void* parameter) {
  for (;;) {
    heartBeat();
    clockInterval();
    weatherInterval();
    ntpInterval();
  }
}

// heartbeat of the system
void heartBeat() {
  if ((millis() - previousHeartbeatMillis) >= heartUpdate) {
    previousHeartbeatMillis = millis();
    if (heartbeat == LOW)
      heartbeat = HIGH;
    else
      heartbeat = LOW;
    digitalWrite(heartbeatLED, heartbeat);
  }
}

// Clock time interval
void clockInterval() {
  if ((millis() - previousTimeMillis) >= timeUpdate) {
    Serial.println();
    Serial.println(F("Update the Clock (RTC and NTP).."));
    digitalClockDisplay();

    previousTimeMillis = millis();
  }
}

// Weather time interval
void weatherInterval() {
  if ((millis() - previousWeatherMillis) > weatherUpdate) {
    Serial.println(F("Get the weather.."));
    Serial.println();
    weather();
    previousWeatherMillis = millis();
  }
}

// NTP time interval
void ntpInterval() {
  if ((millis() - previousNTPMillis) >= NTPUpdate) {
    if (timeStatus() != timeNotSet) {
      if (now() != prevDisplay) {  // Update only if time has changed
        prevDisplay = now();
        Serial.println();
        Serial.println(F("Get NTP time.."));
        ntpTime();
      }
    }
    previousNTPMillis = millis();
  }
}
