Ardecoder: decoding up to 3 encoder with Arduino Uno or Nano.


ELECTRICAL CONNECTIONS
----------------------

|     |               |
| --- | ------------- |
| D2  | Encoder 1 (A) |
| D3  | Encoder 1 (B) |
| D4  | Encoder 2 (A) |
| D5  | Encoder 2 (B) |
| D6  | Encoder 3 (A) |
| D7  | Encoder 3 (B) |
|     |               |
| D9  | Encoder 1 (Z) |
| D10 | Encoder 2 (Z) |
| D11 | Encoder 3 (Z) |

If fewer encoders are needed, do not connect them. This also works with
incremental encoders without the Z index: in this case the counter will
be set to 0 at startup and never reset.

You can skip any encoder, e.g. you are allowed to connect only encoder 2
and 3 without using encoder 1.

![Only encoder 1 connected](./photo.jpeg)

To improve reliability, you can put photocouplers between encoder and
arduino, as shown by the above example where only encoder 1 is connected
(see [the schematic](./ardecoder.pdf) for details). In that case,
remember to consider the maximum speed of the photocoupler too.


HOW DECODING WORKS
------------------

The code keeps the last state of A B phase (`old`) and compares it to
the new state (`now`). Any variation will increase or decrease the
counter (`raw`) depending on a lookup table (`lut`).

If `OVERFLOW` is defined, it is checked if a phase is skipped (e.g. when
the encoder is turning too quickly) by leveraging another lookup table
(`skp`). If that is the case, the counter is incremented or decremented
by 2, depending on the last adjustment.

```
                 _______         _______
        A ______|       |_______|       |______
CCW <--      _______         _______         __  --> CW
        B __|       |_______|       |_______|
```

| now<br>A B | old<br>A B | lut<br>&nbsp; | skp<br>&nbsp;  | |
| --- | --- | ---:|:-----:| ---
| 0 0 | 0 0 |   0 | false |  No movement
| 0 0 | 0 1 |  +1 | false |
| 0 0 | 1 0 |  -1 | false |
| 0 0 | 1 1 |   0 |  true |  One step has been skipped
| 0 1 | 0 0 |  -1 | false |
| 0 1 | 0 1 |   0 | false |  No movement
| 0 1 | 1 0 |   0 |  true |  One step has been skipped
| 0 1 | 1 1 |  +1 | false |
| 1 0 | 0 0 |  +1 | false |
| 1 0 | 0 1 |  -2 |  true |  One step has been skipped
| 1 0 | 1 0 |   0 | false |  No movement
| 1 0 | 1 1 |  -1 | false |
| 1 1 | 0 0 |  +2 |  true |  One step has been skipped
| 1 1 | 0 1 |  -1 | false |
| 1 1 | 1 0 |  +1 | false |
| 1 1 | 1 1 |   0 | false |  No movement

All 4 counters are adjusted according to the above decoding algorithm by
the PCINT2 interrupt handler, called whenever any of the PIND bits (i.e.
any D0..D7 digital input) changes.


LICENSE
-------

Ardecoder is [MIT licensed](./LICENSE).
