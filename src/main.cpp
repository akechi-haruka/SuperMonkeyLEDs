#include <Arduino.h>
#include <FastLED.h>
#include <HardwareSerial.h>
#include "jvs.h"
#include "hresult.h"
#include "led.h"
#include "avr/wdt.h"

/// CONFIGURATION START

// The number of physical LEDs connected to the board
#define NUM_LEDS 66
// The offset from the beginning of how many LEDs should be skipped
#define LED_SHIFT 0
// The data pin for the LED strip
#define LED_PIN 7
// unused
#define LED_BRIGHTNESS 5
// The maximum current in mA that may be taken before brightness will be reduced. Should not be changed.
#define MAX_CURRENT 750
// The FastLED constant for the board used.
#define LED_BOARD WS2812B

// The output PIN for an extra output (ex. the camera LED in SAOAC:DE). This can only be used by mods that support it.
#define AUX1_OUTPUT_PIN 6
// Whether the extra output is another FastLED strip (1) or a single LED (0)
#define AUX1_IS_STRIP 1
// If the extra output is a strip, this is the FastLED constant for it.
#define AUX1_LED_BOARD WS2812B
// If the extra output is a strip, this is the amount of LEDs.
#define AUX1_NUM_LEDS 4
// If the extra output is a strip, this is the maximum current in mA that may be taken before brightness will be reduced. Should not be changed.
#define AUX1_MAX_CURRENT 40
/// CONFIGURATION END


struct CRGB leds[NUM_LEDS + LED_SHIFT];
struct CRGB leds_aux1[AUX1_NUM_LEDS];
#define LED_OFF 0xFF
#define LED_ON 0xFE

#define JVS_MAX_LEDS 66
#define DELAY 5

#define UNKNOWN_IS_OK 0

#define BOARD_NAME_LEN 8
#define CHIP_NUM_LEN 5

#define FADE_TIMER_DEFAULT 80

#define FADE_MODE_NONE 0
#define FADE_MODE_DECREASE 1
#define FADE_MODE_INCREASE 2

static uint16_t setting_timeout = 0;
static int32_t timeout_counter = 0;
static uint16_t setting_led_count = JVS_MAX_LEDS;
static bool setting_disable_resp = false;
static uint8_t fade_mode = FADE_MODE_NONE;
static int16_t fade_timer_max = FADE_TIMER_DEFAULT;
static int16_t fade_timer = FADE_TIMER_DEFAULT;
static int16_t fade_modifier = 1;
static int16_t fade_value = 255;

static uint8_t translation_table[NUM_LEDS];
static uint16_t setting_fw_sum = 0xFFFF;
static char board_name[BOARD_NAME_LEN];
static char chip_num[CHIP_NUM_LEN];
static uint16_t setting_fw_ver = 0xFF;
static uint8_t setting_channels[3];

void reset_monkey(){
    strncpy(chip_num, "6710 ", CHIP_NUM_LEN);
    strncpy(board_name, "MONKEY06", BOARD_NAME_LEN);
    setting_fw_sum = 0xFFFF;
    setting_fw_ver = 0xFF;

    for (uint32_t i = 0; i < sizeof(translation_table); i++){
        translation_table[i] = i;
    }
    setting_channels[0] = 1;
    setting_channels[1] = 0;
    setting_channels[2] = 2;
}

void led_get_board_info(jvs_req_any *req, jvs_resp_any *resp) {
    resp->len += 18;
    resp->report = 1;
    strcpy((char*)resp->payload, board_name);
    *(resp->payload + 8) = 0x0A;
    strcpy((char*)resp->payload + 9, chip_num);
    *(resp->payload + 14) = 0xFF;
    *(resp->payload + 15) = setting_fw_ver;
    *(resp->payload + 16) = 0;
    *(resp->payload + 17) = 204;
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
    memset(&leds, 0, sizeof(CRGB) * (NUM_LEDS + LED_SHIFT));
#if AUX1_IS_STRIP && AUX1_OUTPUT_PIN > 0
    memset(&leds_aux1, 0, sizeof(CRGB) * AUX1_NUM_LEDS);
#elif AUX1_OUTPUT_PIN > 0
    digitalWrite(AUX1_OUTPUT_PIN, LOW);
#endif
    FastLED.show();
}

void led_reset(jvs_req_any *req, jvs_resp_any *resp) {
    setting_timeout = 0;
    setting_disable_resp = false;
    setting_led_count = JVS_MAX_LEDS;
    fade_mode = FADE_MODE_NONE;
    fade_timer = FADE_TIMER_DEFAULT;
    fade_timer_max = FADE_TIMER_DEFAULT;
    fade_modifier = 1;
    fade_value = 255;

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

void led_set_direct(jvs_req_any *req) {
    for (uint32_t i = 0; i < sizeof(translation_table) && i < setting_led_count; i++){
        uint8_t translation = translation_table[i];
        if (translation == LED_OFF){
            leds[i + LED_SHIFT].setRGB(0, 0, 0);
        } else if (translation == LED_ON){
            leds[i + LED_SHIFT].setRGB(255, 255, 255);
        } else {
            uint8_t input_offset = translation * 3;
            if (input_offset < req->len - 3) {
                leds[i + LED_SHIFT].setRGB(req->payload[input_offset + setting_channels[0]], req->payload[input_offset + setting_channels[1]],
                               req->payload[input_offset + setting_channels[2]]);
            }
        }
    }

}

void led_set(jvs_req_any *req, jvs_resp_any *resp) {

    led_set_direct(req);

    fade_mode = FADE_MODE_NONE;
    fade_value = 255;

    FastLED.setBrightness(255);
    FastLED.show();
}

void led_set_fade(jvs_req_any *req, jvs_resp_any *resp) {
    led_set_direct(req);
    fade_mode = FADE_MODE_DECREASE;
    fade_value = 255;

    FastLED.setBrightness(255);
    FastLED.show();
}
void led_set_fade_pattern(jvs_req_any *req, jvs_resp_any *resp) {
    uint8_t depth = req->payload[0]; // 48
    uint8_t cycle = req->payload[1]; // 2

    fade_mode = FADE_MODE_DECREASE;
    fade_value = 255;

    // TODO: I truly have no idea what these variables even mean
    fade_timer = depth;
    fade_timer_max = depth;
    fade_modifier = cycle * 2;
}

void led_set_aux(jvs_req_any *req, jvs_resp_any *resp) {

#if AUX1_IS_STRIP && AUX1_OUTPUT_PIN > 0
    for (uint32_t i = 0; i < AUX1_NUM_LEDS; i++){
        uint8_t input_offset = i * 3;
        if (input_offset < req->len - 3) {
            leds_aux1[i].setRGB(req->payload[input_offset + setting_channels[0]], req->payload[input_offset + setting_channels[1]],
                           req->payload[input_offset + setting_channels[2]]);
        }
    }
    FastLED.show();
#elif AUX1_OUTPUT_PIN > 0
    digitalWrite(AUX1_OUTPUT_PIN, req->payload[0] != 0 ? HIGH : LOW);
#endif

}

void led_set_count(jvs_req_any *req, jvs_resp_any *resp) {

    setting_led_count = req->payload[0];

    resp->len += 1;
    *(resp->payload) = min(req->payload[0], NUM_LEDS);
}

void led_set_sum(jvs_req_any *req, jvs_resp_any *resp) {

    setting_fw_sum = (req->payload[1] << 8) | req->payload[0];

}

void led_set_chip(jvs_req_any *req, jvs_resp_any *resp) {

    memset(chip_num, 0, CHIP_NUM_LEN);
    memcpy(chip_num, req->payload, min(req->len - 2, CHIP_NUM_LEN));

}

void led_set_fw_ver(jvs_req_any *req, jvs_resp_any *resp) {

    setting_fw_ver = req->payload[0];

}

void led_set_board_name(jvs_req_any *req, jvs_resp_any *resp) {

    memset(board_name, ' ', BOARD_NAME_LEN);
    memcpy(board_name, req->payload, min(req->len - 2, BOARD_NAME_LEN));

}

void led_reset_monkey(jvs_req_any *req, jvs_resp_any *resp) {
    reset_monkey();
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

    CFastLED::addLeds<LED_BOARD, LED_PIN>(leds, NUM_LEDS + LED_SHIFT);
    if (AUX1_IS_STRIP && AUX1_OUTPUT_PIN > 0) {
        CFastLED::addLeds<AUX1_LED_BOARD, AUX1_OUTPUT_PIN>(leds_aux1, AUX1_NUM_LEDS);
    }
    //FastLED.setBrightness(LED_BRIGHTNESS);
    FastLED.setMaxPowerInVoltsAndMilliamps(5, MAX_CURRENT);
    if (AUX1_IS_STRIP && AUX1_OUTPUT_PIN > 0) {
        FastLED.setMaxPowerInVoltsAndMilliamps(5, MAX_CURRENT + AUX1_MAX_CURRENT);
    }

    leds[0] = CRGB::Red;
    FastLED.show();
    delay(100);

    for (uint32_t i = 0; i < sizeof(translation_table); i++){
        translation_table[i] = i;
    }

    reset_monkey();

    leds[0] = CRGB::Yellow;
    FastLED.show();
    delay(100);

    Serial.begin(115200);

    leds[0] = CRGB::Green;
    leds_aux1[0] = CRGB::Green;
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
        } else if (req.cmd == LED_CMD_SET_LED_FADE){
            led_set_fade(&req, &resp);
        } else if (req.cmd == LED_CMD_SET_LED_FADE_PATTERN){
            led_set_fade_pattern(&req, &resp);
        } else if (req.cmd == LED_CMD_SET_COUNT){
            led_set_count(&req, &resp);
        } else if (req.cmd == LED_CMD_MONKEY_RESET){
            led_reset_monkey(&req, &resp);
        } else if (req.cmd == LED_CMD_MONKEY_SET_SUM){
            led_set_sum(&req, &resp);
        } else if (req.cmd == LED_CMD_MONKEY_SET_CHIP){
            led_set_chip(&req, &resp);
        } else if (req.cmd == LED_CMD_MONKEY_SET_FW_VER){
            led_set_fw_ver(&req, &resp);
        } else if (req.cmd == LED_CMD_MONKEY_SET_BOARD_NAME){
            led_set_board_name(&req, &resp);
        } else if (req.cmd == LED_CMD_MONKEY_SET_TRANSLATION){
            led_set_translation(&req, &resp);
        } else if (req.cmd == LED_CMD_MONKEY_SET_CHANNELS){
            led_set_channels(&req, &resp);
        } else if (req.cmd == LED_CMD_MONKEY_SET_AUXILIARY_OUTPUT){
            led_set_aux(&req, &resp);
        } else {
#if UNKNOWN_IS_OK
#else
            jvs_write_failure(E_NOTIMPL, req.cmd, &req);
            return;
#endif
        }

        if (!setting_disable_resp || req.cmd == LED_CMD_DISABLE_RESPONSE) {
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

    if (fade_mode != 0) {
        if (fade_timer > 0) {
            fade_timer -= DELAY;
        }
        if (fade_timer <= 0) {
            if (fade_mode == FADE_MODE_DECREASE) {
                fade_value -= fade_modifier;
                if (fade_value <= 0) {
                    fade_value = 0;
                    fade_mode = FADE_MODE_INCREASE;
                }
            } else if (fade_mode == FADE_MODE_INCREASE) {
                fade_value += fade_modifier;
                if (fade_value >= 255) {
                    fade_value = 255;
                    fade_mode = FADE_MODE_DECREASE;
                }
            }

            FastLED.setBrightness(fade_value);
            FastLED.show();

            fade_timer = fade_timer_max;
        }
    }
    delay(DELAY);
}
