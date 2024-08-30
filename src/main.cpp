#include <Arduino.h>
#include <FastLED.h>
#include <HardwareSerial.h>
#include "jvs.h"
#include "hresult.h"
#include "led.h"
#include "avr/wdt.h"

/// CONFIGURATION START
#define NUM_LEDS 66
#define LED_PIN 7
#define LED_BRIGHTNESS 5
#define MAX_CURRENT 750
#define LED_BOARD WS2812B
/// CONFIGURATION END


struct CRGB leds[NUM_LEDS];
#define LED_OFF 0xFF
#define LED_ON 0xFE

#define DELAY 5

#define UNKNOWN_IS_OK 0
static const char* BOARD_NAME = "MONKEY06";
static const char* CHIP_NUM = "6710 ";
static const uint8_t FIRMWARE_VERSION = 0xFF;

static uint16_t setting_timeout = 0;
static int32_t timeout_counter = 0;
static uint16_t setting_led_count = 66;
static bool setting_disable_resp = false;

static uint8_t translation_table[NUM_LEDS];
static uint16_t setting_fw_sum = 0xFFFF;
static uint8_t setting_channels[3];

void led_get_board_info(jvs_req_any *req, jvs_resp_any *resp) {
    resp->len += 16;
    resp->report = 1;
    strcpy((char*)resp->payload, BOARD_NAME);
    *(resp->payload + 8) = 0x0A;
    strcpy((char*)resp->payload + 9, CHIP_NUM);
    *(resp->payload + 14) = 0xFF;
    *(resp->payload + 15) = FIRMWARE_VERSION;
}

void led_get_firm_sum(jvs_req_any *req, jvs_resp_any *resp) {
    resp->len += 2;
    *(resp->payload) = setting_fw_sum >> 8;
    *(resp->payload + 1) = (uint8_t)setting_fw_sum;
}

void led_get_protocol_ver(jvs_req_any *req, jvs_resp_any *resp) {
    resp->len += 3;
    *(resp->payload) = 0x01;
    *(resp->payload + 1) = 0x01;
    *(resp->payload + 2) = 0x04;
}

void led_timeout(jvs_req_any *req, jvs_resp_any *resp) {
    resp->len += 2;

    setting_timeout = req->payload[0] << 8 | req->payload[1];

    *(resp->payload) = req->payload[0];
    *(resp->payload + 1) = req->payload[1];
}

void wipe_leds(){
    memset(&leds, 0, sizeof(CRGB) * NUM_LEDS);
    FastLED.show();
}

void led_reset(jvs_req_any *req, jvs_resp_any *resp) {
    setting_timeout = 0;
    setting_disable_resp = false;
    setting_led_count = 66;

    wipe_leds();
}

void led_get_board_status(jvs_req_any *req, jvs_resp_any *resp) {
    resp->len += 4;
    // is this needed??
    *(resp->payload) = 0;
    *(resp->payload + 1) = 0;
    *(resp->payload + 2) = 0;
    *(resp->payload + 3) = 0;
}

void led_disable_response(jvs_req_any *req, jvs_resp_any *resp) {

    setting_disable_resp = req->payload[0];

    resp->len += 1;
    *(resp->payload) = req->payload[0];
}

void led_set(jvs_req_any *req, jvs_resp_any *resp) {

    for (uint32_t i = 0; i < sizeof(translation_table) && i < setting_led_count; i++){
        uint8_t translation = translation_table[i];
        if (translation == LED_OFF){
            leds[i].setRGB(0, 0, 0);
        } else if (translation == LED_ON){
            leds[i].setRGB(255, 255, 255);
        } else {
            uint8_t input_offset = translation * 3;
            if (input_offset < req->len - 3) {
                leds[i].setRGB(req->payload[input_offset + setting_channels[0]], req->payload[input_offset + setting_channels[1]],
                               req->payload[input_offset + setting_channels[2]]);
            }
        }
    }

    FastLED.show();

}

void led_set_count(jvs_req_any *req, jvs_resp_any *resp) {

    setting_led_count = req->payload[0];

    resp->len += 1;
    *(resp->payload) = min(req->payload[0], NUM_LEDS);
}

void led_set_sum(jvs_req_any *req, jvs_resp_any *resp) {

    setting_fw_sum = req->payload[0] << 8 | req->payload[1];

}

void led_reset_monkey(jvs_req_any *req, jvs_resp_any *resp) {
    setting_fw_sum = 0xFFFF;

    for (uint32_t i = 0; i < sizeof(translation_table); i++){
        translation_table[i] = i;
    }
    setting_channels[0] = 1;
    setting_channels[1] = 0;
    setting_channels[2] = 2;
}

void led_set_translation(jvs_req_any *req, jvs_resp_any *resp) {

    memset(translation_table, LED_OFF, sizeof(translation_table));

    for (uint32_t i = 0; i < sizeof(translation_table) && i < (uint32_t)(req->len - 1); i++){
        uint8_t val = req->payload[i];
        if (val < NUM_LEDS || val == LED_ON) {
            translation_table[i] = val;
        } else {
            translation_table[i] = LED_OFF;
        }
    }
}

void led_set_channels(jvs_req_any *req, jvs_resp_any *resp) {
    for (int i = 0; i < 3; i++){
        setting_channels[i] = req->payload[i];
    }
}

void setup() {

    CFastLED::addLeds<LED_BOARD, LED_PIN>(leds, NUM_LEDS);
    //FastLED.setBrightness(LED_BRIGHTNESS);
    FastLED.setMaxPowerInVoltsAndMilliamps(5, MAX_CURRENT);

    leds[0] = CRGB::Red;
    FastLED.show();
    delay(100);

    for (uint32_t i = 0; i < sizeof(translation_table); i++){
        translation_table[i] = i;
    }

    setting_channels[0] = 1;
    setting_channels[1] = 0;
    setting_channels[2] = 2;

    leds[0] = CRGB::Yellow;
    FastLED.show();
    delay(100);

    Serial.begin(115200);

    leds[0] = CRGB::Green;
    FastLED.show();
    delay(100);

    wdt_enable(WDTO_4S);
}

void loop() {

    wdt_reset();

    if (Serial.available() > 0){

        jvs_req_any req = {0};
        jvs_resp_any resp = {0};
        HRESULT hr = jvs_read_packet(&req);
        if (FAILED(hr)){
            jvs_write_failure(hr, 25, &req);
            return;
        }

        resp.src = req.dest;
        resp.dest = req.src;
        resp.cmd = req.cmd;
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
        } else if (req.cmd == LED_CMD_MONKEY_RESET){
            led_reset_monkey(&req, &resp);
        } else if (req.cmd == LED_CMD_MONKEY_SET_SUM){
            led_set_sum(&req, &resp);
        } else if (req.cmd == LED_CMD_MONKEY_SET_TRANSLATION){
            led_set_translation(&req, &resp);
        } else if (req.cmd == LED_CMD_MONKEY_SET_CHANNELS){
            led_set_channels(&req, &resp);
        } else {
#if UNKNOWN_IS_OK
#else
            jvs_write_failure(E_NOTIMPL, req.cmd, &req);
            return;
#endif
        }

        if (!setting_disable_resp) {
            jvs_write_packet(&resp);
        }

        timeout_counter = setting_timeout;
    }

    if (setting_timeout > 0 && timeout_counter > 0) {
        timeout_counter -= DELAY;
        if (timeout_counter < 0) {
            wipe_leds();
        }
    }
    delay(DELAY);
}
