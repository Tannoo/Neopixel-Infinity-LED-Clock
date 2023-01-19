# Neopixel-Infinity-LED-Clock
An Infinity mirror clock I created using 60 neopixels in a strip.

This clock is using an Adafruit Feather 32 V2 and a DS3132 Feather wing.

<img align="top" width=200 src="20180128_Clock.jpg">


# Features:

    Very quiet operation.
    Chimes via pulsing the entire strip. No sound.
    DS3231 real time clock for accurate time-keeping ability.
    NTP sync'ed time for the RTC.
    Outside temperature via Weather update from openweathermap.org for the the background color.
    Dual-core usage to provide a more stable operation.


# Libraries needed:

// Neopixel support
Neopixel library by Adafruit

// Date and time operations
Time library by Michael Margolis v1.6.1

// Wifi Support
WiFi library
WiFiUdp
WiFiManager

// Weather
WiFiClient
HTTPClient
Arduino_JSON

// OTA abilities
ArduinoOTA

// RTC from Adafruit
RTClib
