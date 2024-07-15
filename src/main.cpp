#include <Arduino.h>
#include <FastLED.h>
#include <HardwareSerial.h>
#include "jvs.h"
#include "hresult.h"
#include "led.h"

#define NUM_LEDS 255
struct CRGB leds[NUM_LEDS];

#define UNKNOWN_IS_OK 1

#define LED_PIN 6
static const char* BOARD_NAME = "MONKEY06";
static const uint8_t FIRMWARE_VERSION = 0xFF;

static uint8_t translation_table[255];


void led_get_board_info(jvs_req_any *req, jvs_resp_any *resp) {

}

void setup() {

    for (uint32_t i = 0; i < sizeof(translation_table); i++){
        translation_table[i] = i;
    }

    Serial.begin(115200);

    CFastLED::addLeds<WS2812B, LED_PIN>(leds, NUM_LEDS);

}

void loop() {

    if (Serial.available() > 0){

        jvs_req_any req = {0};
        jvs_resp_any resp = {0};
        HRESULT hr = jvs_read_packet(&req);
        if (FAILED(hr)){
            jvs_write_failure(hr, &req);
        }

        resp.src = req.dest;
        resp.dest = req.src;
        resp.len = 3;
        resp.status = 1;
        resp.report = 1;

        if (req.cmd == LED_CMD_GET_BOARD_INFO){
            led_get_board_info(&req, &resp);
        } else if (req.cmd == LED_CMD_GET_FIRM_SUM){
            led_get_firm_sum(&req, &resp);
        } else if (req.cmd == LED_CMD_GET_PROTOCOL_VER){
            led_get_protocol_ver(&req, &resp);
        } else if (req.cmd == LED_CMD_TIMEOUT){
            led_timeout(&req, &resp);
        } else if (req.cmd == LED_CMD_RESET){
            led_reset(&req, &resp);
        } else if (req.cmd == LED_CMD_GET_BOARD_STATUS){
            led_get_board_status(&req, &resp);
        } else if (req.cmd == LED_CMD_DISABLE_RESPONSE){
            led_disable_response(&req, &resp);
        } else if (req.cmd == LED_CMD_SET_LED){
            led_set(&req, &resp);
        } else if (req.cmd == LED_CMD_SET_COUNT){
            led_set_count(&req, &resp);
        } else if (req.cmd == LED_CMD_MONKEY_SET_SUM){
            led_set_sum(&req, &resp);
        } else if (req.cmd == LED_CMD_MONKEY_SET_TRANSLATION){
            led_set_translation(&req, &resp);
        } else {
#if UNKNOWN_IS_OK
#else
            jvs_write_failure(E_NOTIMPL, &req);
            return;
#endif
        }

    }

    delay(1);

    // FastLED.show();

}
