SuperMonkeyLEDs
2024 Haruka
Licensed under the GPLv3

SEGA 15093-06 board emulator for Arduino to use WS2812B LEDs.

NOTE!
Arduino boards do not enable DTR/RTS by default, so the game will fail to connect.
Put this into your game launch bat file to fix this:

mode COMxx DTR=on RTS=on

where xx is your Arduino COM port


Default is Pin 7 and a WS2812B. Technically you can use everything FastLED supports, see the first few lines in main.cpp:

/// CONFIGURATION START
#define NUM_LEDS 66
#define LED_PIN 7
#define LED_BRIGHTNESS 32
#define LED_BOARD WS2812B
/// CONFIGURATION END
