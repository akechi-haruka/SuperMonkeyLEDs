SuperMonkeyLEDs
2024 Haruka
Licensed under the GPLv3

SEGA 15093-06 board emulator for Arduino to use WS2812B LEDs.

NOTE!
Arduino boards do not enable DTR/RTS by default, so the game will fail to connect.
Put this into your game launch bat file to fix this:

mode COMxx DTR=on RTS=on

where xx is your Arduino COM port