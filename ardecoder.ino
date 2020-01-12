#include <stdint.h>

#undef  DEBUGGING
#define OVERFLOW  1


#if DEBUGGING
volatile bool error = false;

ISR(BADISR_vect)
{
    error = true;
}
#endif


volatile int16_t raw[4]   = { 0 };
bool             homed[4] = { false };
char             line[32] = { 0 };
char *           ptr      = line;
const int8_t     lut[]    = {
     0, +1, -1,  0,
    -1,  0,  0, +1,
    +1,  0,  0, -1,
     0, -1, +1,  0
};

#if OVERFLOW
volatile uint8_t skips[4] = { 0 };
volatile int8_t  last[4]  = { 0 };
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
        ++skips[n];
    } else {
        last[n] = delta;
    }
#endif
}

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

static void
reset_counter(int n, uint8_t mask)
{
    bool home = mask > 0;
    homed[n] = homed[n] || home;
    raw[n]  *= home ? 0 : 1;
}

/**
 * Reset counters on zero signal.
 * PINB holds the states of digital inputs D8..D13.
 */
ISR(PCINT0_vect)
{
    uint8_t bits = PINB;

    reset_counter(0, bits & 0x01);
    reset_counter(1, bits & 0x02);
    reset_counter(2, bits & 0x04);
    reset_counter(3, bits & 0x08);
}

static void
setup()
{
    /* Set D0..D11 in input mode */
    for (int pin = 0; pin <= 11; ++pin) {
        pinMode(pin, INPUT_PULLUP);
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
    Serial.println("!Started ardecoder");
}

static void
response(int nenc)
{
    Serial.print(nenc);
    Serial.print(" ");
    Serial.print(raw[nenc - 1]);
    Serial.print(" ");
    Serial.print(homed[nenc] ? "1" : "0");
#if OVERFLOW
    Serial.print(" ");
    Serial.print(skips[nenc]);
#endif
#if DEBUGGING
    Serial.print(" ");
    Serial.print(error ? 1 : 0);
#endif
    Serial.print("\n");
}

static bool
parse_command(const char *command)
{
    if (command[1] == '\0') {
        int nenc = command[0] - '0';
        if (nenc >= 1 && nenc <=4) {
            response(nenc);
            return true;
        }
    }
    return false;
}

static void
loop()
{
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
        *ptr = '\0';
#if DEBUGGING
        /* Echo of the command */
        Serial.print("#'");
        Serial.print(line);
        Serial.print("'\n");
#endif
        if (! parse_command(line)) {
            /* Unrecognized command */
            Serial.print("?'");
            Serial.print(line);
            Serial.print("'\n");
        }
        ptr = line;
    } else if (c != -1 && ptr - line < 32) {
        *ptr = (char) c;
        ++ptr;
    }
}
