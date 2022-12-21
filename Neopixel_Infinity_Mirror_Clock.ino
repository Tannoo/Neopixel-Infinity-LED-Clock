/*
 *  Neopixel - OTA - NTP - RTC - Infinity Clock
 */

// >>>>>>> WIFI & UDP <<<<<<<
// **************************
#include <TimeLib.h>         // For Date/Time operations
#include <ESP8266WiFi.h>     // For WiFi
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
#include <Wire.h>   // Library for I2C communication
#include <SPI.h>    // Not used here, but needed to prevent a RTClib compile error
#include <RTClib.h> // Library for RTC stuff

RTC_DS1307 RTC;     // Setup an instance of DS1307 naming it RTC

// >>>>>>> NEOPIXEL STUFF <<<<<<<
// ******************************
#include <Adafruit_NeoPixel.h>

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

#define CHCOLOR1  neopix_gamma[j], 0, 0                                 // Red
#define CHCOLOR2  neopix_gamma[j], 0, 0, neopix_gamma[j]                // Lt. Red
#define CHCOLOR3  neopix_gamma[j], neopix_gamma[j], 0                   // Yellow
#define CHCOLOR4  0, neopix_gamma[j], 0                                 // Green
#define CHCOLOR5  0, neopix_gamma[j], 0, neopix_gamma[j]                // Lt. Green
#define CHCOLOR6  0, neopix_gamma[j], neopix_gamma[j]                   // Teal
#define CHCOLOR7  0, 0, neopix_gamma[j]                                 // Blue
#define CHCOLOR8  0, 0, neopix_gamma[j], neopix_gamma[j]                // Lt. Blue
#define CHCOLOR9  neopix_gamma[j], 0, neopix_gamma[j]                   // Purple
#define CHCOLOR10  neopix_gamma[j], neopix_gamma[j], neopix_gamma[j]    // RGB White
#define CHCOLOR11  neopix_gamma[j], 0, neopix_gamma[j], neopix_gamma[j] // Lt. Purple
#define CHCOLOR12  0, neopix_gamma[j], neopix_gamma[j], neopix_gamma[j] // Lt. Teal

/*
  h  = Base hour
  sh = Sweeping hour
  hr = RTC hour
  m  = Base minute
  s  = Base second
  ms = Base millisecond
*/
uint8_t ms, s, m, mf, h, hf, sh, hr;
uint8_t oldsec, old_mf, oldmin, old_hf, old_hr, oldhr;
uint8_t backgnd_white, m_backgnd_fade, h_backgnd_fade;
uint8_t oldm_backgnd_fade, oldh_backgnd_fade;

#define H_COLOR                    hf,    h_backgnd_fade,    h_backgnd_fade
#define OLD_H_COLOR            old_hf, oldh_backgnd_fade, oldh_backgnd_fade
#define M_COLOR        m_backgnd_fade,                mf,    m_backgnd_fade
#define OLD_M_COLOR oldm_backgnd_fade,            old_mf, oldm_backgnd_fade
#define MH_COLOR                  255,               255,                 0
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
uint32_t timeUpdate = random(20000, 120000);

void setup(void) {
  // Generate random seed
  uint32_t seed = 0;
  for ( uint8_t i = 10 ; i ; i-- ) {
    seed = ( seed << 5 ) + ( analogRead( 15 ) * 3 );
  }
  randomSeed( seed );  //set random seed

  backgnd_white = random(0, 50); // Randomizing the background intensity to reduce monotony.

  Serial.begin(115200);
  Serial.println("Starting up...");

  // >>>>>>> BEGIN OTA STUFF <<<<<<<
  // *******************************
  Serial.print("Connecting to wifi... ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Try again later.");
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
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("OTA is ready.");

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  // END OTA STUFF

  // >>>>>>> BEGIN RTC STUFF <<<<<<<
  // *******************************
  Wire.begin(); // Start the I2C
  RTC.begin();  // Init RTC
  RTC.adjust(DateTime(__DATE__, __TIME__));  // Time and date is expanded to date and time on your computer at compiletime
  Serial.print("Time and date set");

  Serial.println("Starting UDP");
  Udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());

  Serial.print("Using NTP Server ");
  Serial.println(ntpServerName);

  //get a random server from the pool
  WiFi.hostByName(ntpServerName, timeServer);
  Serial.print("NTP Server IP ");
  Serial.println(timeServer);

  Serial.println("Waiting for sync");
  setSyncProvider(getNtpTime);  
  setSyncInterval(86400);

  ESP.wdtDisable();

  // >>>>>>> Start Demo / Pixel test <<<<<<<
  // ***************************************
  strip.begin();
  strip.setBrightness(50);
  Serial.print("Pixel Color Test... ");
  Serial.println("White Over Rainbow");
  whiteOverRainbow(20,25,5);
  // End Demo / Pixel test

  // Set the DST and the time
  setDST(true);

  Serial.println("Neopixel Clock...");

  Serial.println("Getting RTC time...");
  digitalClockDisplay();
  oldhr = sh; old_hr = hr; oldmin = m; oldsec = s;

  hourchime(5);
}

void loop() {
  ArduinoOTA.handle();
  oldhr = sh; oldmin = m; oldsec = s;

  DateTime now = RTC.now();
  if (now.hour() >= 12) h = (now.hour() - 12) * (NUMPIXELS / 12);
    else h = now.hour() * (NUMPIXELS / 12);
  m = now.minute();
  s = now.second(); 

  // Chime
  if (old_hr != sh && m == 0) {
    ESP.wdtFeed(); // Keep the watchdogs happy
    FadeoffBackgnd(5);
    delay(200);
    setDST(true);
    hourchime(3);
    backgnd_white = random(0, 127);
    old_hr = sh;
  }

  // digital clock display of the time
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= timeUpdate) {
    previousMillis = currentMillis;
    timeUpdate = random(20000, 120000);
    //setSyncProvider(getNtpTime);
    digitalClockDisplay();
  }

  // **** Time ****

  // Map and set the fading minute color.
  mf = map(s, 0, NUMPIXELS - 1, backgnd_white, 255);
  old_mf = map(s, 0, NUMPIXELS - 1, 255, backgnd_white);

  m_backgnd_fade = map(s, 0, NUMPIXELS - 1, backgnd_white, 0);
  oldm_backgnd_fade = map(s, 0, NUMPIXELS - 1, 0, backgnd_white);

  // Map and set the fading hour color.
  hf = map(m, 0, NUMPIXELS - 1, backgnd_white, 255);
  old_hf = map(m, 0, NUMPIXELS - 1, 255, backgnd_white);

  h_backgnd_fade = map(m, 0, NUMPIXELS - 1, backgnd_white, 0);
  oldh_backgnd_fade = map(m, 0, NUMPIXELS - 1, 0, backgnd_white);

  // Map and set the hour LED position.
  sh = map(m, 0, NUMPIXELS - 1, h, h + 4);

  for (ms = 0; ms <= NUMPIXELS - 1; ms ++) {
    if (ms == sh && oldhr != sh) strip.setPixelColor(ms, strip.Color(H_COLOR));
    else if (ms == sh) strip.setPixelColor(ms, strip.Color(H_COLOR));
    else if (ms == sh + 1) strip.setPixelColor(ms, strip.Color(H_COLOR));

    else if (ms == m && m != oldmin) strip.setPixelColor(ms, strip.Color(OLD_M_COLOR));
    else if (ms == m) strip.setPixelColor(ms, strip.Color(M_COLOR));
    else if (ms == m + 1) strip.setPixelColor(ms, strip.Color(M_COLOR));

    else if (ms == s && s != oldsec) strip.setPixelColor(ms, strip.Color(S_COLOR));
    else if (ms == s) strip.setPixelColor(ms, strip.Color(S_COLOR));

    else strip.setPixelColor(ms, strip.Color(MS_COLOR));
    strip.show();
    ESP.wdtFeed(); // Keep the watchdogs happy

    delay(718 / NUMPIXELS); // Change the ms delay for secs to match the LEDs.
    strip.setPixelColor(ms, strip.Color(BACKGND));
    strip.setPixelColor(sh, strip.Color(OLD_H_COLOR));
    strip.setPixelColor(sh + 1, strip.Color(H_COLOR));
    if (oldhr != sh)
      strip.setPixelColor(oldhr, strip.Color(BACKGND));
    if (m == sh)
      strip.setPixelColor(m, strip.Color(MH_COLOR));
    else
      strip.setPixelColor(m, strip.Color(OLD_M_COLOR));
      strip.setPixelColor(m + 1, strip.Color(M_COLOR));
    if (oldmin != m && oldmin != sh)
      strip.setPixelColor(oldmin, strip.Color(BACKGND));
    if (s == 0 && oldsec != sh && oldsec != m && !ms != sh && ms != m && sh != NUMPIXELS - 1 && m != NUMPIXELS - 1)
      strip.setPixelColor(NUMPIXELS - 1, strip.Color(BACKGND));
    strip.setPixelColor(s, strip.Color(S_COLOR));
    if (oldsec != s && oldsec != m && oldsec != m + 1 && oldsec != sh && oldsec != sh + 1)
      strip.setPixelColor(oldsec, strip.Color(BACKGND));
    strip.show();
    ESP.wdtFeed(); // Keep the watchdogs happy
  }
}

void hourchime(uint8_t wait) {
  Serial.print("Hour Chimes.. ");
  int chmhr;
  if (hr == 0) chmhr = 12;
    else chmhr = hr;
  for (int m = 1; m <= chmhr; m ++) {
    if (m > 1) Serial.print(" - ");
    Serial.print(m);
    for(uint16_t i = 0; i <= NUMPIXELS - 1; i ++) {
      if (m == 1)  strip.setPixelColor(i, strip.Color(255, 0, 0));
      if (m == 2)  strip.setPixelColor(i, strip.Color(255, 0, 0, 255));
      if (m == 3)  strip.setPixelColor(i, strip.Color(255, 255, 0));
      if (m == 4)  strip.setPixelColor(i, strip.Color(0, 255, 0));
      if (m == 5)  strip.setPixelColor(i, strip.Color(0, 255, 0, 255));
      if (m == 6)  strip.setPixelColor(i, strip.Color(0, 255, 255));
      if (m == 7)  strip.setPixelColor(i, strip.Color(0, 0, 255));
      if (m == 8)  strip.setPixelColor(i, strip.Color(0, 0, 255, 255));
      if (m == 9)  strip.setPixelColor(i, strip.Color(255, 0, 255));
      if (m == 10) strip.setPixelColor(i, strip.Color(ALL_WHITE));
      if (m == 11) strip.setPixelColor(i, strip.Color(255, 0, 255, 255));
      if (m == 12) strip.setPixelColor(i, strip.Color(0, 255, 255, 255));
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
  Serial.println("Fading on the background...");
  FadeonBackgnd(5);
  Serial.println();
  Serial.println("Running the clock...");
  Serial.println();
}

void digitalClockDisplay() {
  DateTime now = RTC.now();

  // Date and Time from RTC  
  Serial.print("Date and Time from RTC: ");
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print('/');
  Serial.print(now.year(), DEC);
  Serial.print(' ');
  if (now.hour() > 12) {
    if ((now.hour() - 12) < 10) Serial.print("0");
    Serial.print((now.hour() - 12), DEC);
  }
    else {
      if (now.hour() < 10) Serial.print("0");
      Serial.print(now.hour(), DEC);
    }
  Serial.print(':');
  if (now.minute() < 10) Serial.print("0");
    Serial.print(now.minute(), DEC);
  Serial.print(':');
  if (now.second() < 10) Serial.print("0");
    Serial.print(now.second(), DEC);
  Serial.println();

  // Date and Time from NTP
  if (now.dayOfTheWeek() == 0) {
    setSyncProvider(getNtpTime);

    Serial.print("Daylight Savings? ");
    if (DST == true) Serial.println("Yes");
      else Serial.println("No");

    Serial.print("Date and Time from NTP: ");
    if (hr < 10) Serial.print("0");
    Serial.print(hr);
    Serial.print(":");
    if (minute() < 10) Serial.print("0");
    Serial.print(minute());
    if (second() < 10) Serial.print("0");
    Serial.print(second());
    Serial.print(" ");
    Serial.print(month());
    Serial.print("/");
    Serial.print(day());
    Serial.print("/");
    Serial.print(year());
    Serial.println();
    Serial.println();
  }

  if ((now.hour() + now.minute()) != (hr + minute()))
    RTC.adjust(DateTime(year(), month(), day(), hr, minute(), second()));

  RTC.now();

  // Show RTC time after adjustment from NTP
  Serial.print("NTP adjusted RTC time: ");
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print('/');
  Serial.print(now.year(), DEC);
  Serial.print(' ');
  if (now.hour() > 12) {
    if ((now.hour() - 12) < 10) Serial.print("0");
    Serial.print((now.hour() - 12), DEC);
  }
    else {
      if (now.hour() < 10) Serial.print("0");
      Serial.print(now.hour(), DEC);
    }
  Serial.print(':');
  if (now.minute() < 10) Serial.print("0");
    Serial.print(now.minute(), DEC);
  Serial.print(':');
  if (now.second() < 10) Serial.print("0");
    Serial.print(now.second(), DEC);
  Serial.println();

  // Set the time data for the clock to use
  if (now.hour() >= 12) h = (now.hour() - 12) * (NUMPIXELS / 12);
    else h = now.hour() * (NUMPIXELS / 12);
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
  Serial.print("Transmit NTP Request #");
  Serial.println(NTPreqnum);
  ESP.wdtFeed(); // Keep the watchdogs happy
  sendNTPpacket(timeServer);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    ESP.wdtFeed(); // Keep the watchdogs happy
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
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
  Serial.println("No NTP Response :-(");
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
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
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
  for(int j = 0; j < backgnd_white; j ++) {
    for(uint16_t i = 0; i < strip.numPixels(); i ++)
        strip.setPixelColor(i, strip.Color(neopix_gamma[j], neopix_gamma[j], neopix_gamma[j]));
    delay(wait);
    strip.show();
    ESP.wdtFeed(); // Keep the watchdogs happy
  }
}

void FadeoffBackgnd(uint8_t wait) {
  for(int j = backgnd_white; j >= 0; j --) {
    for(uint16_t i = 0; i < strip.numPixels(); i ++)
        strip.setPixelColor(i, strip.Color(neopix_gamma[j], neopix_gamma[j], neopix_gamma[j]));
    delay(wait);
    strip.show();
    ESP.wdtFeed(); // Keep the watchdogs happy
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
        if(head == strip.numPixels()){
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
