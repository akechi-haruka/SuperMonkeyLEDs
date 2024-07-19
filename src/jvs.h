#pragma once
#include <Arduino.h>
#include "hresult.h"

struct __attribute__((__packed__)) jvs_req_any {
    uint8_t dest;
    uint8_t src;
    uint8_t len;
    uint8_t cmd;
    uint8_t payload[249];
};

struct __attribute__((__packed__)) jvs_resp_any {
    uint8_t dest;
    uint8_t src;
    uint8_t len;
    uint8_t status;
    uint8_t cmd;
    uint8_t report;
    uint8_t payload[247];
};

HRESULT jvs_read_packet(struct jvs_req_any* req);
HRESULT jvs_write_packet(struct jvs_resp_any* resp);
HRESULT jvs_write_failure(HRESULT hr, int report, struct jvs_req_any* req);