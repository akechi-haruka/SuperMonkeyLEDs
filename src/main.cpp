#include <Arduino.h>
#include <FastLED.h>
#include <HardwareSerial.h>
#include "jvs.h"
#include "hresult.h"
#include "led.h"

#define NUM_LEDS 66
struct CRGB leds[NUM_LEDS];

#define UNKNOWN_IS_OK 1

#define LED_PIN 6
static const char* BOARD_NAME = "MONKEY06";
static const char* CHIP_NUM = "6710 ";
static const uint8_t FIRMWARE_VERSION = 0xFF;

static uint16_t setting_timeout = 0;
static uint16_t setting_led_count = 66;
static bool setting_disable_resp = false;

static uint8_t translation_table[66];
static uint16_t setting_fw_sum = 0xFFFF;

void led_get_board_info(jvs_req_any *req, jvs_resp_any *resp) {
    resp->len += 16;
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

void led_reset(jvs_req_any *req, jvs_resp_any *resp) {
    setting_timeout = 0;
    setting_disable_resp = false;
    setting_led_count = 66;

    memcpy(&leds, 0, sizeof(CRGB) * NUM_LEDS);

    FastLED.show();
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

    for (uint32_t i = 3; i < req->len && i - 3 < setting_led_count; i+=3){
        uint8_t j = translation_table[i - 3];
        leds[j].setRGB(req->payload[i], req->payload[i + 1], req->payload[i + 2]);
    }

    FastLED.show();

}

void led_set_count(jvs_req_any *req, jvs_resp_any *resp) {

    setting_led_count = req->payload[0];

    resp->len += 1;
    *(resp->payload) = req->payload[0];
}

void led_set_sum(jvs_req_any *req, jvs_resp_any *resp) {

    setting_led_count = req->payload[0];

    resp->len += 1;
    *(resp->payload) = req->payload[0];
}

void led_reset_monkey(jvs_req_any *req, jvs_resp_any *resp) {
    setting_fw_sum = 0xFFFF;

    for (uint32_t i = 0; i < sizeof(translation_table); i++){
        translation_table[i] = i;
    }
}

void led_set_translation(jvs_req_any *req, jvs_resp_any *resp) {
    for (uint32_t i = 0; i < sizeof(translation_table) && i < req->len - 3; i++){
        translation_table[i] = req->payload[i];
    }
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
            if (setting_disable_resp) {
                return;
            }
        } else if (req.cmd == LED_CMD_SET_COUNT){
            led_set_count(&req, &resp);
        } else if (req.cmd == LED_CMD_MONKEY_RESET){
            led_reset_monkey(&req, &resp);
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

        jvs_write_packet(&resp);
    }

    delay(1);
}
