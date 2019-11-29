/* Ardecoder: decoding up to 4 encoder with Arduino Uno or Nano.
 *
 * Expected connections:
 *
 * D0   Encoder 1 (A)
 * D1   Encoder 1 (B)
 * D2   Encoder 2 (A)
 * D3   Encoder 2 (B)
 * D4   Encoder 3 (A)
 * D5   Encoder 3 (B)
 * D6   Encoder 4 (A)
 * D7   Encoder 4 (B)
 *
 * D8   Encoder 1 (Z)
 * D9   Encoder 2 (Z)
 * D10  Encoder 3 (Z)
 * D11  Encoder 4 (Z)
 *
 * If fewer encoders are needed, just void connecting them.
 */
#include <stdint.h>

#define DEBUGGING 1
#define OVERFLOW  1


volatile int16_t raw[4] = { 0, 0, 0, 0 };

#if OVERFLOW
volatile uint8_t overflow[4] = { 0, 0, 0, 0 };
volatile int8_t  last[4] = { 0, 0, 0, 0 };
#endif

char line[32] = { 0 };
char *ptr     = line;


void setup()
{
    int pin;

    /* Set D0..D11 in input mode */
    for (pin = 0; pin <= 11; ++pin) {
        pinMode(pin, INPUT);
    }

    // Enable interrupt on any change of D0..D7
    PCMSK2 = 0xFF;
    PCIFR |= bit(PCIF2);
    PCICR |= bit(PCIE2);

    // Enable interrupt on any change of D8..D11
    PCMSK0 = 0x0F;
    PCIFR |= bit(PCIF0);
    PCICR |= bit(PCIE0);

    Serial.begin(115200);
    Serial.print("Started ardecoder\n");
}

void loop()
{
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
        *ptr = '\0';
        if (DEBUGGING) {
            Serial.print("Received command: '");
            Serial.print(line);
            Serial.print("'\n");
        }
        ptr = line;
    } else if (c != -1) {
        *ptr = (char) c;
        ++ptr;
    }
}

/* How decoding works.
 *
 * The code keeps the last state of A B phase (old) and compares it to
 * the new state (now). Any variation will increase or decrease the
 * counter (raw) depending on a lookup table (lut).
 *
 * Furthermore, it is checked if a step is skipped (e.g. because the
 * encoder is running too quick) by leveraging another lookup table
 * (skp). If that is the case, the counter is incremented or decremented
 * by 2, depending on the last adjustment.
 *
 *                  _______         _______
 *         A ______|       |_______|       |______
 * CCW <--      _______         _______         __  --> CW
 *         B __|       |_______|       |_______|
 *
 *  now old lut  skp
 *  B A B A
 *  --- --- --- -----
 *  0 0 0 0   0 false  No movement
 *  0 0 0 1  +1 false
 *  0 0 1 0  -1 false
 *  0 0 1 1   0  true  One step has been skipped
 *  0 1 0 0  -1 false
 *  0 1 0 1   0 false  No movement
 *  0 1 1 0   0  true  One step has been skipped
 *  0 1 1 1  +1 false
 *  1 0 0 0  +1 false
 *  1 0 0 1  -2  true  One step has been skipped
 *  1 0 1 0   0 false  No movement
 *  1 0 1 1  -1 false
 *  1 1 0 0  +2  true  One step has been skipped
 *  1 1 0 1  -1 false
 *  1 1 1 0  +1 false
 *  1 1 1 1   0 false  No movement
 */
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

static void
update_counter(int n, uint8_t baba)
{
    int8_t delta = lut[baba];
    raw[n] += delta;
#if OVERFLOW
    if (skp[baba]) {
        raw[n] += last[n] * 2;
        ++overflow[n];
    } else {
        last[n] = delta;
    }
#endif
}

/**
 * Adjust the counters depending on the encoder phase changes.
 * PIND holds the states of digital inputs D0..D7.
 */
ISR(PCINT2_vect)
{
    uint8_t now = PIND;
    static uint8_t old = 0x00;

    update_counter(0, ((now & 0x03) << 2) + (old & 0x03));
    update_counter(1,  (now & 0x0C) + ((old & 0x0C) >> 2));
    update_counter(2, ((now & 0x30) + ((old & 0x30) >> 2)) >> 2);
    update_counter(3, ((now & 0xC0) + ((old & 0xC0) >> 2)) >> 4);

    old = now;
}

/**
 * Reset counters on zero signal.
 * PINB holds the states of digital inputs D8..D13.
 */
ISR(PCINT0_vect)
{
    uint8_t bits = PINB;

    raw[0] *= ! (bits & 0x01);
    raw[1] *= ! (bits & 0x02);
    raw[2] *= ! (bits & 0x04);
    raw[3] *= ! (bits & 0x08);
}
