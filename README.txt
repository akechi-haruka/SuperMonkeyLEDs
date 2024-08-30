SuperMonkeyLEDs
2024 Haruka
Licensed under the GPLv3

SEGA 15093-06 board emulator for Arduino to use WS2812B LEDs.

NOTE!
Arduino boards do not enable DTR/RTS by default, so the game will fail to connect.
Put this into your game launch bat file to fix this:

mode COMxx DTR=on RTS=on

where xx is your Arduino COM port

you can also instead use Sega835Cmd (https://github.com/akechi-haruka/SEGA835Lib)
to fix the DTR/RTS and also preconfigure your board

Examples:

KCA: sega835cmd.exe led --set-monkey-channels Green,Red,Blue --set-monkey-translation 0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,6,6,6,6,7,7,7,7,8,8,8,8,9,9,9,9,10,10,10,10,11,11,11,11,12,12,12,12,13,13,13,13,14,14,14,14,15,15,15,15 0,0,0
ApmV3: D:\segacmd\sega835cmd led --set-monkey-channels Green,Red,Blue --set-monkey-translation 3,3,3,3,3,3,3,3,3,3,3,3,16,16,16,16,16,16,16,16,16,16,16,16,16,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 0,0,0

Default is Pin 7 and a WS2812B. Technically you can use everything FastLED supports, see the first few lines in main.cpp:

/// CONFIGURATION START
#define NUM_LEDS 66
#define LED_PIN 7
#define LED_BRIGHTNESS 5
#define MAX_CURRENT 750
#define LED_BOARD WS2812B
/// CONFIGURATION END
