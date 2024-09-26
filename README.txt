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

Default is Pin 7 and a WS2812B. Technically you can use everything FastLED supports, see the first few lines in main.cpp:

/// CONFIGURATION START
#define NUM_LEDS 66
#define LED_PIN 7
#define LED_BRIGHTNESS 5
#define MAX_CURRENT 750
#define LED_BOARD WS2812B
/// CONFIGURATION END


Known LED mappings:

ApmV3:

LED BOTTOM: 3-12
LED TOP: 16-21
LED CENTER: 0-2,13-15

KCA:
Gear Pop LED 1-16: 0-15

Chrono Regalia Satellite:
top left 16-18
top right 19-21
bottom left 0-2
bottom right 13-15
bottom 3-12

Chrono Regalia Terminal:
ALL: 0

FGO:
CABINET LT 16-18
CABINET RT 19-21
CABINET LB 0-2
CABINET RB 13-15
CABINET CB 3-12
CABINET RING OUTER 22-43
CABINET RING INNER 44-64

SAOAC:DE (with SwordArtOffline):

Backboard: 0
Centerboard: 1
Frontboard: 2 (white only)
Side: 3
QR Code: 4 (white only)

Synchronica (if using https://github.com/akechi-haruka/SynchronicaLEDToSega):

LED1: 0-9
LED2: 10-19
LED3: 20-29
LED4: 30-39
LED5: 40-49
LED L: 50,51
LED R: 52,53
LED CENTER: 54-61

Examples (for an LED strip starting at the bottom right of a 32 inch monitor, wrapping around counter-clockwise):

KCA: sega835cmd led --set-monkey-channels Green,Red,Blue --set-monkey-translation 0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,6,6,6,6,7,7,7,7,8,8,8,8,9,9,9,9,10,10,10,10,11,11,11,11,12,12,12,12,13,13,13,13,14,14,14,14,15,15,15,15 0,0,0
ApmV3: sega835cmd led --set-monkey-channels Green,Red,Blue --set-monkey-translation 3,3,3,3,3,3,3,3,3,3,3,3,16,16,16,16,16,16,16,16,16,16,16,16,16,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 0,0,0
Chrono Regalia (Satellite): sega835cmd led --monkey-reset --set-monkey-board-name 15093-06 --set-monkey-chip-number 6710A --set-monkey-version 160 --set-monkey-checksum 43603 --set-monkey-translation 13,13,13,13,13,13,13,13,13,13,13,13,19,19,19,19,19,19,19,19,19,19,19,19,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3 255,255,255
Chrono Regalia (Terminal): sega835cmd led --monkey-reset --set-monkey-board-name 15093-06 --set-monkey-chip-number 6710A --set-monkey-version 160 --set-monkey-checksum 43603 --set-monkey-translation 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
FGO: sega835cmd led --set-monkey-translation 13,13,13,13,13,13,13,13,3,3,3,3,3,3,3,3,19,19,19,19,19,19,19,19,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64 0,0,0
SAOAC:DE: sega835cmd led --set-monkey-translation 3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 0,0,0
Synchronica: sega835cmd led --monkey-reset --set-monkey-translation 53,53,53,53,53,53,53,53,53,53,53,53,53,53,53,53,53,53,53,53,53,53,53,53,41,40,39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0 0,0,0
Kemono Friends 3: sega835cmd led --monkey-reset --set-monkey-checksum 43603 --set-monkey-version 160 --set-monkey-board-name 15093-04 --set-monkey-chip-number 6704 0,0,0