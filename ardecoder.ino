/* ardecoder: decoding up to 3 encoder with Arduino Uno or Nano.
 *
 * Copyright (C) 2020  Nicola Fontana <ntd@entidi.it>
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <stdint.h>
#include <stdlib.h>

/* Undefine if you do not want homing, e.g. encoder without Z channel */
#define HOME 1

/* Set to 1 to handle phase skips at cost of slowing down the reading */
#undef OVERFLOW


typedef struct {
    volatile int16_t raw;
    int16_t          dumped;
#if HOME
    bool             homed;
#endif
#if OVERFLOW
    volatile uint8_t skips;
    volatile int8_t  last;
#endif
} Encoder;

Encoder encoders[3] = { 0 };
long timeout = 0;
const int8_t lut[] = {
     0, +1, -1,  0,
    -1,  0,  0, +1,
    +1,  0,  0, -1,
     0, -1, +1,  0
};

#if OVERFLOW
const bool skp[] = {
     false,  false,  false,  true,
     false,  false,  true,   false,
     false,  true,   false,  false,
     true,   false,  false,  false
};
#endif


void
encoder_update(Encoder *encoder, uint8_t baba)
{
    int8_t delta;

#if OVERFLOW
    if (skp[baba]) {
        delta = encoder->last * 2;
        ++encoder->skips;
    } else {
        encoder->last = delta = lut[baba];
    }
#else
    delta = lut[baba];
#endif

    encoder->raw += delta;
}

void
encoder_reset(Encoder *encoder, uint8_t mask)
{
    if (mask > 0) {
#if HOME
        encoder->homed = true;
#endif
        encoder->raw   = 0;
    }
}

void
encoder_dump(const Encoder *encoder)
{
    Serial.print(encoder - encoders + 1);
    Serial.print(" ");
    Serial.print(encoder->raw);
#if HOME
    Serial.print(" ");
    Serial.print(encoder->homed ? "1" : "0");
#endif
#if OVERFLOW
    Serial.print(" ");
    Serial.print(encoder->skips);
#endif
    Serial.print("\r\n");
}

void
encoder_dump_if_changed(Encoder *encoder)
{
    if (encoder->dumped != encoder->raw) {
        encoder_dump(encoder);
        encoder->dumped = encoder->raw;
    }
}

/**
 * Update raw counters on AB channel changes.
 * PIND holds the states of digital inputs D0..D7.
 */
ISR(PCINT2_vect)
{
    uint8_t now = PIND;
    static uint8_t old = 0x00;

    encoder_update(encoders + 0,  (now & 0x0C) + ((old & 0x0C) >> 2));
    encoder_update(encoders + 1, ((now & 0x30) + ((old & 0x30) >> 2)) >> 2);
    encoder_update(encoders + 2, ((now & 0xC0) + ((old & 0xC0) >> 2)) >> 4);

    old = now;
}

#if HOME
/**
 * Reset counters on zero signal.
 * PINB holds the states of digital inputs D8..D13.
 */
ISR(PCINT0_vect)
{
    uint8_t bits = PINB;

    encoder_reset(encoders + 0, bits & 0x02);
    encoder_reset(encoders + 1, bits & 0x04);
    encoder_reset(encoders + 2, bits & 0x08);
}
#endif

bool
handle_request(const char *request)
{
    if (request[1] == '\0') {
        int n = request[0] - '0';
        if (n >= 1 && n <=3) {
            encoder_dump(encoders + n - 1);
            return true;
        }
    } else if (request[0] == 'S') {
        timeout = atoi(request + 1);
        return true;
    }
    return false;
}

void
setup()
{
    /* Enable interrupt on any change of D2..D7 */
    for (int pin = 2; pin <= 7; ++pin) {
        pinMode(pin, INPUT_PULLUP);
    }
    PCMSK2 = 0xFC;
    PCIFR |= bit(PCIF2);
    PCICR |= bit(PCIE2);

#if HOME
    /* Enable interrupt on any change of D9..D11 (home handling) */
    for (int pin = 9; pin <= 11; ++pin) {
        pinMode(pin, INPUT_PULLUP);
    }
    PCMSK0 = 0x0E;
    PCIFR |= bit(PCIF0);
    PCICR |= bit(PCIE0);
#endif

    /* Setup serial communication on USB */
    Serial.begin(115200);
    Serial.setTimeout(1000);
    Serial.print("#Started ardecoder\r\n");
}

void
loop()
{
    static char request[32] = { 0 };
    static char *ptr = request;
    char ch;
    if (Serial.readBytes(&ch, 1) == 0) {
        /* Timeout on reading: dump encoder statuses (if needed) */
        if (timeout > 0) {
            encoder_dump_if_changed(encoders + 0);
            encoder_dump_if_changed(encoders + 1);
            encoder_dump_if_changed(encoders + 2);
        }
    } else if (ch == '\n' || ch == '\r') {
        /* EOL encountered: execute the request */
        *ptr = '\0';
        if (ptr == request) {
            /* Ignore spurious EOLs */
        } else if (! handle_request(request)) {
            /* Failure: unrecognized request */
            Serial.print("?'");
            Serial.print(request);
            Serial.print("'\r\n");
        } else {
            /* Success: always adjust the timeout (could be changed) */
            Serial.setTimeout(timeout == 0 ? 1000 : timeout);
        }
        ptr = request;
    } else if (ptr - request < 32) {
        /* Append the new char to the request, if enough space left */
        *ptr = ch;
        ++ptr;
    }
}
