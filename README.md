Ardecoder: decoding up to 4 encoder with Arduino Uno or Nano.


ELECTRICAL CONNECTIONS
----------------------

|     |               |
| --- | ------------- |
| D0  | Encoder 1 (A) |
| D1  | Encoder 1 (B) |
| D2  | Encoder 2 (A) |
| D3  | Encoder 2 (B) |
| D4  | Encoder 3 (A) |
| D5  | Encoder 3 (B) |
| D6  | Encoder 4 (A) |
| D7  | Encoder 4 (B) |
|     |               |
| D8  | Encoder 1 (Z) |
| D9  | Encoder 2 (Z) |
| D10 | Encoder 3 (Z) |
| D11 | Encoder 4 (Z) |

If fewer encoders are needed, do not connect them. This also works with
incremental encoders without the Z index: in this case the counter will
be set to 0 at startup and never reset.

You can skip any encoder, e.g. if you need to use the serial line (that
in turn requires the `D0` and `D1` digital lines), just avoid connecting
the first encoder.

![Only encoder 2 connected](https://github.com/ntd/ardecoder/tree/master/photo.jpeg)

To improve reliability, you can put photocouplers between encoder and
arduino, as shown by the above example where only encoder 2 is connected
([schematic](https://github.com/ntd/ardecoder/tree/master/ardecoder.pdf)).
In that case remember to consider the maximum speed of the photocoupler.


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
