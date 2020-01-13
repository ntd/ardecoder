#include <stdint.h>

/* Set to 1 to have echo enabled on USB */
#undef  ECHO

/* Set to 1 to handle phase skips at cost of slowing down the reading */
#define OVERFLOW  1


typedef struct {
    volatile int16_t raw;
    bool             homed;
#if OVERFLOW
    volatile uint8_t skips;
    volatile int8_t  last;
#endif
} Encoder;

static Encoder encoders[4] = { 0 };


static void
encoder_update(Encoder *encoder, uint8_t baba)
{
    int8_t delta;
    static const int8_t lut[] = {
         0, +1, -1,  0,
        -1,  0,  0, +1,
        +1,  0,  0, -1,
         0, -1, +1,  0
    };

#if OVERFLOW
    static const bool skp[] = {
         false,  false,  false,  true,
         false,  false,  true,   false,
         false,  true,   false,  false,
         true,   false,  false,  false
    };
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

static void
encoder_reset(Encoder *encoder, uint8_t mask)
{
    if (mask > 0) {
        encoder->homed = true;
        encoder->raw   = 0;
    }
}

static void
encoder_dump_status(const Encoder *encoder)
{
    Serial.print(encoder - encoders);
    Serial.print(" ");
    Serial.print(encoder->raw);
    Serial.print(" ");
    Serial.print(encoder->homed ? "1" : "0");
#if OVERFLOW
    Serial.print(" ");
    Serial.print(encoder->skips);
#endif
    Serial.print("\n");
}


/**
 * Update raw counters on AB channel changes.
 * PIND holds the states of digital inputs D0..D7.
 */
ISR(PCINT2_vect)
{
    uint8_t now = PIND;
    static uint8_t old = 0x00;

    encoder_update(encoders + 0, ((now & 0x03) << 2) + (old & 0x03));
    encoder_update(encoders + 1,  (now & 0x0C) + ((old & 0x0C) >> 2));
    encoder_update(encoders + 2, ((now & 0x30) + ((old & 0x30) >> 2)) >> 2);
    encoder_update(encoders + 3, ((now & 0xC0) + ((old & 0xC0) >> 2)) >> 4);

    old = now;
}

/**
 * Reset counters on zero signal.
 * PINB holds the states of digital inputs D8..D13.
 */
ISR(PCINT0_vect)
{
    uint8_t bits = PINB;

    encoder_reset(encoders + 0, bits & 0x01);
    encoder_reset(encoders + 1, bits & 0x02);
    encoder_reset(encoders + 2, bits & 0x04);
    encoder_reset(encoders + 3, bits & 0x08);
}

static void
setup()
{
    /* Set D0..D11 in input mode */
    for (int pin = 0; pin <= 11; ++pin) {
        pinMode(pin, INPUT_PULLUP);
    }

    /* Enable interrupt on any change of D0..D7 */
    PCMSK2 = 0xFF;
    PCIFR |= bit(PCIF2);
    PCICR |= bit(PCIE2);

    /* Enable interrupt on any change of D8..D11 */
    PCMSK0 = 0x0F;
    PCIFR |= bit(PCIF0);
    PCICR |= bit(PCIE0);

    /* Setup serial communication on USB */
    Serial.begin(115200);
    Serial.println("!Started ardecoder");
}

static bool
handle_request(const char *request)
{
    if (request[1] == '\0') {
        int n = request[0] - '0';
        if (n >= 1 && n <=4) {
            encoder_dump_status(encoders + n - 1);
            return true;
        }
    }
    return false;
}

static void
loop()
{
    static char request[32] = { 0 };
    static char *ptr = request;
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
        *ptr = '\0';
#if ECHO
        /* Echo of the request */
        Serial.print("#'");
        Serial.print(request);
        Serial.print("'\n");
#endif
        if (! handle_request(request)) {
            /* Unrecognized request */
            Serial.print("?'");
            Serial.print(request);
            Serial.print("'\n");
        }
        ptr = request;
    } else if (c != -1 && ptr - request < 32) {
        *ptr = (char) c;
        ++ptr;
    }
}
