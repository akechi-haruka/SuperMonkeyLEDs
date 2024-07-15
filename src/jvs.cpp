#pragma clang diagnostic push
#pragma ide diagnostic ignored "EmptyDeclOrStmt"
#pragma ide diagnostic ignored "modernize-use-nullptr"
#include <Arduino.h>
#include <HardwareSerial.h>
#include "hresult.h"
#include "jvs.h"

#define dprintf(x, ...)

#define COMM_BUF_SIZE 255
#define TRY_SALVAGE_COMM_SYNC 1
#define CHECKSUM_IS_ERROR 0

HRESULT jvs_encode(const uint8_t *in, uint32_t inlen, uint8_t *out, uint32_t *outlen){
    if (in == NULL || out == NULL || outlen == NULL){
        return E_HANDLE;
    }
    if (inlen < 2){
        return E_INVALIDARG;
    }
    if (*outlen < inlen + 2){
        return E_NOT_SUFFICIENT_BUFFER;
    }

    uint8_t checksum = 0;
    uint32_t offset = 0;

    out[offset++] = 0xE0;
    for (uint32_t i = 0; i < inlen; i++){

        uint8_t byte = in[i];

        if (byte == 0xE0 || byte == 0xD0) {
            CHECK_OFFSET_BOUNDARY(offset+2, *outlen)
            out[offset++] = 0xD0;
            out[offset++] = byte - 1;
        } else {
            CHECK_OFFSET_BOUNDARY(offset+1, *outlen)
            out[offset++] = byte;
        }

        checksum += byte;
    }
    CHECK_OFFSET_BOUNDARY(offset+1, *outlen)
    out[offset++] = checksum;
    *outlen = offset;

    return S_OK;
}

HRESULT serial_read_single_byte(uint8_t* ptr){
    int r = Serial.read();
    if (r == -1){
        dprintf(NAME ": Stream was empty\n");
        return HRESULT_FROM_WIN32(E_FAIL);
    }
    *ptr = (uint8_t)r;
    return S_OK;
}

HRESULT jvs_decoding_read(uint8_t *out, uint32_t *outlen){
    if (out == NULL || outlen == NULL){
        return E_HANDLE;
    }

    const uint32_t len_byte_offset = 1;
    uint8_t checksum = 0;
    uint32_t offset = 0;
    int bytes_left = COMM_BUF_SIZE;
    HRESULT hr;
    bool escape_flag = false;

    do {
        hr = serial_read_single_byte(out + offset);
        if (FAILED(hr)){
            return hr;
        }

        uint8_t byte = *(out + offset);

        if (offset == len_byte_offset){
            bytes_left = byte - 1;
        }

        if (offset == 0){
            if (byte != 0xE0){
#if TRY_SALVAGE_COMM_SYNC
                dprintf(NAME ": WARNING! Garbage on line: %x\n", byte);
                continue;
#else
                dprintf(NAME ": Failed to read from serial port: aime decode failed: Sync failure: %x\n", byte);
                return HRESULT_FROM_WIN32(ERROR_DATA_CHECKSUM_ERROR);
#endif
            }
            offset++;
        } else if (byte == 0xD0){
            escape_flag = true;
        } else {
            if (escape_flag) {
                byte += 1;
                escape_flag = false;
                bytes_left++;
            } else if (byte == 0xE0){
                dprintf(NAME ": Failed to read from serial port: aime decode failed: Found unexpected sync byte in stream at pos %d\n", offset);
                return HRESULT_FROM_WIN32(ERROR_DATA_CHECKSUM_ERROR);
            }
            checksum += byte;
            offset++;
        }
    } while (bytes_left-- > 0);

    hr = serial_read_single_byte(out + offset);
    if (FAILED(hr)){
        return hr;
    }

    uint8_t schecksum = *(out + offset);

    if (checksum != schecksum){
#if CHECKSUM_IS_ERROR
        dprintf(NAME ": Failed to read from serial port: aime decode failed: Checksum failed: expected %d, got %d\n", checksum, schecksum);
        return HRESULT_FROM_WIN32(ERROR_DATA_CHECKSUM_ERROR);
#else
        dprintf(NAME ": Decode: WARNING! Checksum mismatch: expected %d, got %d\n", checksum, schecksum);
#endif
    }

#if SUPER_VERBOSE
    dprintf(NAME": Data received from serial (%d):\n", offset);
    dump(out, offset);
#endif

    // strip sync byte from response
    *outlen = offset - 1;
    memcpy(out, out + 1, *outlen);

    return S_OK;
}

HRESULT jvs_read_packet(struct jvs_req_any* req){

    if (req == NULL){
        return E_HANDLE;
    }

    uint8_t out[255];
    uint32_t outlen = 255;
    HRESULT hr = jvs_decoding_read(out, &outlen);
    if (FAILED(hr)){
        return hr;
    }

    memcpy(req, out, outlen);

    return S_OK;
}

HRESULT jvs_write_packet(struct jvs_resp_any* resp) {

    if (resp == NULL){
        return E_HANDLE;
    }

    HRESULT hr;

    uint8_t out[255];
    uint32_t outlen = 255;

    hr = jvs_encode((uint8_t*)resp, resp->len + 5, out, &outlen);
    if (FAILED(hr)){
        return hr;
    }

    Serial.write(out, outlen);

    return S_OK;
}

HRESULT jvs_write_failure(HRESULT hr, struct jvs_req_any* req){

    struct jvs_resp_any resp = {0};
    resp.dest = req->src;
    resp.src = req->dest;
    resp.cmd = req->cmd;
    resp.len = 3;
    resp.status = 2;
    resp.report = 1;
    if (hr == E_NOT_SUFFICIENT_BUFFER){
        resp.status = 6;
    } else if (hr == HRESULT_FROM_WIN32(ERROR_DATA_CHECKSUM_ERROR)){
        resp.status = 3;
    } else if (hr == E_FAIL){
        resp.status = 4;
    } else if (hr == E_NOTIMPL){
        resp.status = 1;
        resp.report = 3;
    }

    return jvs_write_packet(&resp);
}
#pragma clang diagnostic pop