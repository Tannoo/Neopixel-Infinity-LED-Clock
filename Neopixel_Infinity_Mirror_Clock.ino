/*
 *  Neopixel - OTA - NTP - RTC - Infinity Clock
 */

// http://arduino.esp8266.com/stable/package_esp8266com_index.json

// >>>>>>> WIFI & UDP <<<<<<<
// **************************
#include <TimeLib.h>         // For Date/Time operations - Time library by Michael Margolis v1.6.1
#include <ESP8266WiFi.h>     // For WiFi ESP8266WiFi library -- unknown source
#include <WiFiUdp.h>         // For UDP NTP

unsigned long NTPreqnum = 0; // For NTP

char ssid[] = "********";
char pass[] = "********";

// >>>>>>> OTA STUFF <<<<<<<
// *************************
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>

// >>>>>>> RTC STUFF <<<<<<<
// *************************
#include <RTClib.h> // Library for RTC stuff

RTC_DS1307 RTC;     // Setup an instance of DS1307 naming it RTC

// >>>>>>> NEOPIXEL STUFF <<<<<<<
// ******************************
#include <Adafruit_NeoPixel.h> // Neopixel library by Adafruit

#define NEOPIN 15
#define NUMPIXELS 60
#define NEOPIXEL_TYPE NEO_GRBW

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, NEOPIN, NEOPIXEL_TYPE + NEO_KHZ800);

byte neopix_gamma[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };

#define CHCOLOR1  neopix_gamma[j],               0,               0                  // Red
#define CHCOLOR2  neopix_gamma[j],               0,               0, neopix_gamma[j] // Lt. Red
#define CHCOLOR3  neopix_gamma[j], neopix_gamma[j],               0                  // Yellow
#define CHCOLOR4                0, neopix_gamma[j],               0                  // Green
#define CHCOLOR5                0, neopix_gamma[j],               0, neopix_gamma[j] // Lt. Green
#define CHCOLOR6                0, neopix_gamma[j], neopix_gamma[j]                  // Teal
#define CHCOLOR7                0,               0, neopix_gamma[j]                  // Blue
#define CHCOLOR8                0,               0, neopix_gamma[j], neopix_gamma[j] // Lt. Blue
#define CHCOLOR9  neopix_gamma[j],               0, neopix_gamma[j]                  // Purple
#define CHCOLOR10 neopix_gamma[j], neopix_gamma[j], neopix_gamma[j]                  // RGB White
#define CHCOLOR11 neopix_gamma[j],               0, neopix_gamma[j], neopix_gamma[j] // Lt. Purple
#define CHCOLOR12               0, neopix_gamma[j], neopix_gamma[j], neopix_gamma[j] // Lt. Teal

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
uint8_t backgnd_white, m_backgnd_fade, h_backgnd_fade;
uint8_t oldm_backgnd_fade, oldh_backgnd_fade;

#define H_COLOR                    hf,    h_backgnd_fade,    h_backgnd_fade
#define OLD_H_COLOR            old_hf, oldh_backgnd_fade, oldh_backgnd_fade
#define M_COLOR        m_backgnd_fade,                mf,    m_backgnd_fade
#define OLD_M_COLOR oldm_backgnd_fade,            old_mf, oldm_backgnd_fade
#define MH_COLOR                   hf,                mf,                 0
#define S_COLOR                     0,                 0,               255
#define MS_COLOR        backgnd_white,               255,               255
#define BACKGND         backgnd_white,     backgnd_white,     backgnd_white
#define ALL_WHITE                 255,               255,               255

// NTP Servers:
IPAddress timeServer;
const char* ntpServerName = "pool.ntp.org";
int timeZone = -7;
bool DST;

WiFiUDP Udp;
uint8_t localPort = 8888;  // Local port to listen for UDP packets
unsigned long previousMillis = 0;
uint32_t timeUpdate = random(8000, 60000);

void setup(void) {
  // Generate random seed
  uint32_t seed = 0;
  for ( uint8_t i = 10 ; i ; i-- ) {
    seed = ( seed << 5 ) + ( analogRead( 15 ) * 3 );
  }
  randomSeed( seed );  //set random seed

  backgnd_white = random(0, 50); // Randomizing the background intensity to reduce monotony.

  Serial.begin(115200);
  Serial.println(F("Starting up..."));

  // >>>>>>> BEGIN OTA STUFF <<<<<<<
  // *******************************
  Serial.print(F("Connecting to wifi... "));
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println(F("Connection Failed! Try again later."));
    delay(5000);
  }
  ArduinoOTA.setPassword("3825"); // OTA password

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println(F("Auth Failed"));
    else if (error == OTA_BEGIN_ERROR) Serial.println(F("Begin Failed"));
    else if (error == OTA_CONNECT_ERROR) Serial.println(F("Connect Failed"));
    else if (error == OTA_RECEIVE_ERROR) Serial.println(F("Receive Failed"));
    else if (error == OTA_END_ERROR) Serial.println(F("End Failed"));
  });
  ArduinoOTA.begin();
  Serial.println(F("OTA is ready."));

  Serial.print(F("IP address: "));
  Serial.println(WiFi.localIP());
  // END OTA STUFF

  // >>>>>>> BEGIN RTC STUFF <<<<<<<
  // *******************************
  Wire.begin(); // Start the I2C
  RTC.begin();  // Init RTC
  
  // Time and date is expanded to date and time on your computer at compile time
  RTC.adjust(DateTime(__DATE__, __TIME__));
  Serial.print(F("Time and date set"));

  Serial.println(F("Starting UDP"));
  Udp.begin(localPort);
  Serial.print(F("Local port: "));
  Serial.println(Udp.localPort());

  Serial.print(F("Using NTP Server "));
  Serial.println(ntpServerName);

  //get a random server from the pool
  WiFi.hostByName(ntpServerName, timeServer);
  Serial.print(F("NTP Server IP "));
  Serial.println(timeServer);

  Serial.println(F("Waiting for sync"));
  setSyncProvider(getNtpTime);  
  setSyncInterval(86400);

  ESP.wdtDisable();

  // >>>>>>> Start Demo / Pixel test <<<<<<<
  // ***************************************
  strip.begin();
  strip.setBrightness(50);
  Serial.print(F("Pixel Color Test... "));
  Serial.println(F("White Over Rainbow"));
  whiteOverRainbow(20,25,5);
  // End Demo / Pixel test

  // Set the DST and the time
  setDST(true);

  Serial.println(F("Neopixel Clock..."));

  Serial.println(F("Getting RTC time..."));
  digitalClockDisplay();
  old_hr = hr; oldmin = m; oldsec = s;

  delay(500);
  if (m == 0) hourchime(5);
  else {
    backgnd_white = random(0, 100);
    FadeonBackgnd(20);    
  }
}

void loop() {
  ArduinoOTA.handle();
  oldmin = m; oldsec = s;

  DateTime now = RTC.now();
  if (now.hour() >= 12)  
    h = map(m, 0, NUMPIXELS - 1, (now.hour() - 12) * (NUMPIXELS / 12), ((now.hour() - 12) * (NUMPIXELS / 12)) + 4);
  else h = map(m, 0, NUMPIXELS - 1, now.hour() * (NUMPIXELS / 12), (now.hour() * (NUMPIXELS / 12)) + 4);

  m = now.minute();
  s = now.second(); 

  // Chime
  if (old_hr != h && m == 0) {
    ESP.wdtFeed(); // Keep the watchdogs happy
    FadeoffBackgnd(5);
    delay(200);
    setDST(true);
    hourchime(3);
    backgnd_white = random(0, 127);
    old_hr = h;
  }

  // digital clock display of the time
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= timeUpdate) {
    previousMillis = currentMillis;
    timeUpdate = 60000;
    digitalClockDisplay();
  }

  // **************
  //      TIME
  // **************

  // Tried a few times to fade the second's LED and it just makes the
  // second's LED black. No fading to be seen.

  // Map and set the fading minute colors.
  mf = map(s, 0, NUMPIXELS - 1, backgnd_white, 255);
  old_mf = map(s, 0, NUMPIXELS - 1, 255, backgnd_white);

  // Map and fade the minute backgroud intensities
  m_backgnd_fade = map(s, 0, NUMPIXELS - 1, backgnd_white, 0);
  oldm_backgnd_fade = map(s, 0, NUMPIXELS - 1, 0, backgnd_white);

  // Map and set the fading hour colors.
  hf = map(m, 0, NUMPIXELS - 1, backgnd_white, 255);
  old_hf = map(m, 0, NUMPIXELS - 1, 255, backgnd_white);

  // Map and fade the hour backgroud intensities
  h_backgnd_fade = map(m, 0, NUMPIXELS - 1, backgnd_white, 0);
  oldh_backgnd_fade = map(m, 0, NUMPIXELS - 1, 0, backgnd_white);

  for (ms = 0; ms <= NUMPIXELS - 1; ms ++) {
    strip.setPixelColor(ms, strip.Color(MS_COLOR));                       // Set the millisecond color
    strip.setPixelColor(h + 1, strip.Color(H_COLOR));                     // Set hour color
    strip.setPixelColor(h, strip.Color(OLD_H_COLOR));                     // Set the old hour color
    if (m + 1 >= NUMPIXELS) strip.setPixelColor(1, strip.Color(M_COLOR)); // Set the minute color at #1 position
    else strip.setPixelColor(m + 1, strip.Color(M_COLOR));                // Set the minute color
    strip.setPixelColor(m, strip.Color(OLD_M_COLOR));                     // Set the old minute color
    strip.setPixelColor(s, strip.Color(S_COLOR));                         // Set the second color
    if (m == h || m == h + 1 || m + 1 == h || m + 1 == h + 1)
      strip.setPixelColor(m, strip.Color(MH_COLOR));                      // Set the minute and hour combined color

    strip.show();           // Set the LEDs
    ESP.wdtFeed();          // Keep the watchdogs happy
    delay(719 / NUMPIXELS); // Change the ms delay for secs to match the LEDs.

    // Setting the LED and background colors after a specific delay
    strip.setPixelColor(ms, strip.Color(BACKGND));                        // Set the ms to the background color
    strip.setPixelColor(h, strip.Color(OLD_H_COLOR));                     // Set the old hour color
    strip.setPixelColor(h + 1, strip.Color(H_COLOR));                     // Set the hour color
    strip.setPixelColor(m, strip.Color(OLD_M_COLOR));                     // Set the old minute color
    if (m + 1 >= NUMPIXELS) strip.setPixelColor(1, strip.Color(M_COLOR)); // Set the minute color at #1 position
    else strip.setPixelColor(m + 1, strip.Color(M_COLOR));                // Set the minute color
    strip.setPixelColor(s, strip.Color(S_COLOR));                         // Set the second color
    strip.setPixelColor(oldsec, strip.Color(BACKGND));                    // Set the old second to the background color
    if (m == h || m == h + 1 || m + 1 == h || m + 1 == h + 1)
      strip.setPixelColor(m, strip.Color(MH_COLOR));                      // Set the minute and hour combined color

    strip.show();  // Set the LEDs
    ESP.wdtFeed(); // Keep the watchdogs happy
  }
}

void hourchime(uint8_t wait) {
  Serial.print(F("Hour Chimes.. "));
  int chmhr;
  if (hr == 0) chmhr = 12;
    else chmhr = hr;
  for (int m = 1; m <= chmhr; m ++) {
    if (m > 1) Serial.print(F(" - "));
    Serial.print(m);
    for(uint16_t i = 0; i <= NUMPIXELS - 1; i ++) {
      if (m == 1)  strip.setPixelColor(i, strip.Color(255,   0,   0,   0)); // Red
      if (m == 2)  strip.setPixelColor(i, strip.Color(255,   0,   0, 255)); // LT Red
      if (m == 3)  strip.setPixelColor(i, strip.Color(255, 255,   0,   0)); // Yellow
      if (m == 4)  strip.setPixelColor(i, strip.Color(  0, 255,   0,   0)); // Green
      if (m == 5)  strip.setPixelColor(i, strip.Color(  0, 255,   0, 255)); // LT Green
      if (m == 6)  strip.setPixelColor(i, strip.Color(  0, 255, 255,   0)); // Teal
      if (m == 7)  strip.setPixelColor(i, strip.Color(  0,   0, 255,   0)); // Blue
      if (m == 8)  strip.setPixelColor(i, strip.Color(  0,   0, 255, 255)); // LT Blue
      if (m == 9)  strip.setPixelColor(i, strip.Color(255,   0, 255,   0)); // Purple
      if (m == 10) strip.setPixelColor(i, strip.Color(ALL_WHITE));          // RGB White
      if (m == 11) strip.setPixelColor(i, strip.Color(255,   0, 255, 255)); // LT Purple
      if (m == 12) strip.setPixelColor(i, strip.Color(  0, 255, 255, 255)); // LT Teal
    }
    strip.show();
    delay(wait);
    ESP.wdtFeed(); // Keep the watchdogs happy
    for(int j = 255; j >= 0; j--) {
      for(uint16_t i = 0; i <= NUMPIXELS - 1; i ++) {
        if (m == 1)  strip.setPixelColor(i, strip.Color(CHCOLOR1));
        if (m == 2)  strip.setPixelColor(i, strip.Color(CHCOLOR2));
        if (m == 3)  strip.setPixelColor(i, strip.Color(CHCOLOR3));
        if (m == 4)  strip.setPixelColor(i, strip.Color(CHCOLOR4));
        if (m == 5)  strip.setPixelColor(i, strip.Color(CHCOLOR5));
        if (m == 6)  strip.setPixelColor(i, strip.Color(CHCOLOR6));
        if (m == 7)  strip.setPixelColor(i, strip.Color(CHCOLOR7));
        if (m == 8)  strip.setPixelColor(i, strip.Color(CHCOLOR8));
        if (m == 9)  strip.setPixelColor(i, strip.Color(CHCOLOR9));
        if (m == 10) strip.setPixelColor(i, strip.Color(CHCOLOR10));
        if (m == 11) strip.setPixelColor(i, strip.Color(CHCOLOR11));
        if (m == 12) strip.setPixelColor(i, strip.Color(CHCOLOR12));
      }
      delay(wait);
      strip.show();
      ESP.wdtFeed(); // Keep the watchdogs happy
    }
  }
  Serial.println();
  Serial.println(F("Fading on the background..."));
  FadeonBackgnd(20);
  Serial.println();
  Serial.println(F("Running the clock..."));
  Serial.println();
}

void digitalClockDisplay() {
  DateTime now = RTC.now();

  // Date and Time from RTC  
  Serial.print(F("Date and Time from RTC: "));
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print('/');
  Serial.print(now.year(), DEC);
  Serial.print(' ');
  if (now.hour() > 12) {
    if ((now.hour() - 12) < 10) Serial.print(F("0"));
    Serial.print((now.hour() - 12), DEC);
  }
    else {
      if (now.hour() < 10) Serial.print(F("0"));
      Serial.print(now.hour(), DEC);
    }
  Serial.print(':');
  if (now.minute() < 10) Serial.print(F("0"));
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  if (now.second() < 10) Serial.print(F("0"));
  Serial.print(now.second(), DEC);
  Serial.println();

  // Clock LED Time  
  Serial.print(F("Clock LED Time: "));
  Serial.print(h);
  Serial.print(':');
  if (m < 10) Serial.print(F("0"));
  Serial.print(m);
  Serial.print(':');
  if (s < 10) Serial.print(F("0"));
  Serial.print(s);
  Serial.println();

  // Date and Time from NTP
  if (now.dayOfTheWeek() == 0) {
    //setSyncProvider(getNtpTime);

    Serial.print(F("Daylight Savings? "));
    if (DST == true) Serial.println(F("Yes"));
      else Serial.println(F("No"));

    Serial.print(F("Date and Time from NTP: "));
    if (hr < 10) Serial.print(F("0"));
    Serial.print(hr);
    Serial.print(F(":"));
    if (minute() < 10) Serial.print(F("0"));
    Serial.print(minute());
    if (second() < 10) Serial.print(F("0"));
    Serial.print(second());
    Serial.print(F(" "));
    Serial.print(month());
    Serial.print(F("/"));
    Serial.print(day());
    Serial.print(F("/"));
    Serial.print(year());
    Serial.println();
    Serial.println();
  }

  if ((now.hour() + now.minute()) != (hr + minute())) {
    RTC.adjust(DateTime(year(), month(), day(), hr, minute(), second()));

    RTC.now();

    // Show RTC time after adjustment from NTP
    Serial.print(F("NTP adjusted RTC time: "));
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print('/');
    Serial.print(now.year(), DEC);
    Serial.print(' ');
    if (now.hour() > 12) {
      if ((now.hour() - 12) < 10) Serial.print(F("0"));
      Serial.print((now.hour() - 12), DEC);
    }
      else {
        if (now.hour() < 10) Serial.print(F("0"));
        Serial.print(now.hour(), DEC);
      }
    Serial.print(':');
    if (now.minute() < 10) Serial.print(F("0"));
      Serial.print(now.minute(), DEC);
    Serial.print(':');
    if (now.second() < 10) Serial.print(F("0"));
      Serial.print(now.second(), DEC);
    Serial.println();
  }
  m = now.minute();
  s = now.second();
}

void setDST(bool check) {
  // Summer Time
  if (month() == 11 && weekday() == 1 && day() >= 1 && day() <= 7 && hour() == 2 && minute() == 0 && second() == 0 && DST == false)
    DST = 1;

  // Winter Time
  if (month() == 3 && weekday() == 1 && day() >= 8 && day() <= 14 && hour() == 2 && minute() == 0 && second() == 0 && DST == true)
    DST = 0;

  // Check now
  if (check == true) {
    if (month() > 11 && month() < 3) DST = false;
    if (month() < 11 && month() > 3) DST = true;
  }
  if (DST == true && hr != hourFormat12() + 1)  hr = hourFormat12() + 1;
  if (DST == false && hr != hourFormat12()) hr = hourFormat12();
}

/*-------- NTP code ----------*/
const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime() {
  while (Udp.parsePacket() > 0); // discard any previously received packets
  NTPreqnum ++;
  Serial.print(F("Transmit NTP Request #"));
  Serial.println(NTPreqnum);
  ESP.wdtFeed(); // Keep the watchdogs happy
  sendNTPpacket(timeServer);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    ESP.wdtFeed(); // Keep the watchdogs happy
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println(F("Receive NTP Response"));
      Serial.println();
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900  = (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println(F("No NTP Response :-("));
  ESP.wdtFeed(); // Keep the watchdogs happy
  Serial.println();
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011; // LI, Version, Mode
  packetBuffer[1] = 0;          // Stratum, or type of clock
  packetBuffer[2] = 6;          // Polling Interval
  packetBuffer[3] = 0xEC;       // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
  ESP.wdtFeed(); // Keep the watchdogs happy
}

void FadeonBackgnd(uint8_t wait) {
  Serial.println(F("Fading on the background..."));
  for(int j = 0; j < backgnd_white; j ++) {
    for(uint16_t i = 0; i < strip.numPixels(); i ++) {
      strip.setPixelColor(i, strip.Color(j, j, j));
    }
    strip.show();
    ESP.wdtFeed(); // Keep the watchdogs happy
    delay(wait);
  }
}

void FadeoffBackgnd(uint8_t wait) {
  Serial.println(F("Fading off the background..."));
  for(int j = backgnd_white; j >= 0; j --) {
    for(uint16_t i = 0; i < strip.numPixels(); i ++) {
      strip.setPixelColor(i, strip.Color(j, j, j));
    }
    strip.show();
    ESP.wdtFeed(); // Keep the watchdogs happy
    delay(wait);
  }
}

void whiteOverRainbow(uint8_t wait, uint8_t whiteSpeed, uint8_t whiteLength) {
  if(whiteLength >= strip.numPixels()) whiteLength = strip.numPixels() - 1;
  uint8_t head = whiteLength - 1;
  uint8_t tail = 0;
  uint8_t loops = 3;
  uint8_t loopNum = 0;
  static unsigned long lastTime = 0;

  while(true) {
    for(uint8_t j = 0; j < 256; j ++) {
      for(uint16_t i = 0; i < strip.numPixels(); i ++) {
        if((i >= tail && i <= head) || (tail > head && i >= tail) || (tail > head && i <= head)) {
          strip.setPixelColor(i, strip.Color(ALL_WHITE));
        }
        else {
          strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
        }
      }
      if(millis() - lastTime > whiteSpeed) {
        head ++;
        tail ++;
        if(head == strip.numPixels()) {
          loopNum ++;
        }
        lastTime = millis();
      }
      if(loopNum == loops) return;
      head %= strip.numPixels();
      tail %= strip.numPixels();
      strip.show();
      ESP.wdtFeed(); // Keep the watchdogs happy
      delay(wait);
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3,0);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3,0);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0,0);
}

uint8_t red(uint32_t c) { return (c >> 16); }
uint8_t green(uint32_t c) { return (c >> 8); }
uint8_t blue(uint32_t c) { return (c); }
