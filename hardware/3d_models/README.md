# Printed parts

Four parts, all printable on a stock FDM machine with no exotic settings. I
print them in PLA; PETG is worth it for the body if the bot lives somewhere warm,
since a car dashboard will soften PLA.

## The parts

| File | What it is | Size (mm) | Triangles |
| :--- | :--- | :--- | ---: |
| `body.stl` | Main shell. Holds the battery, both motors, the driver board and the ESP32. | 54 × 72 × 32 | 3,616 |
| `lid.stl` | Top plate. The OLED sits in the window, touch pad underneath. | 54 × 72 × 2 | 1,792 |
| `n20_bracket.stl` | Motor mount. Print two. | 17.3 × 26.5 × 11.5 | 1,204 |
| `sensor_clamp.stl` | Retainer that holds the TTP223 board flat against the lid. | 20 × 10 × 3.4 | 828 |

Wheels aren't here. I used off-the-shelf 34 mm rubber wheels that push onto the
N20 D-shaft, which cost about a dollar a pair and grip far better than anything
I printed. If you want to print your own, the shaft is a 3 mm D with a 0.5 mm flat.

## Print settings

Nothing here is fussy. What I use:

- **Layer height** 0.2 mm. Drop to 0.16 mm if you want a cleaner OLED bezel.
- **Walls** 3 perimeters. The motor bracket takes real load, so don't go below this.
- **Infill** 20% gyroid.
- **Supports** only on `body.stl`, touching build plate. The other three print flat.
- **PLA** 205 °C nozzle, 60 °C bed.

The lid is 2 mm thick and prints in about fifteen minutes. Print a spare — it's
the part you'll re-cut if you move the display.

## Assembly order

Order matters here, because the body gets crowded fast.

1. Press the N20 motors into the two brackets, then screw the brackets into the
   floor of `body.stl`. Do this first: once the battery is in, you can't reach
   the screws.
2. Route the motor leads up the inside wall before anything else goes in.
3. Drop in the LiPo, then the TB6612 board, then the ESP32 last — it's the one
   you'll be unplugging most often.
4. Seat the OLED in the lid window from underneath. It's a friction fit; if it's
   loose, a strip of tape on the long edge takes up the slack.
5. Put the TTP223 face-down on the lid's inner boss and hold it with
   `sensor_clamp.stl`. It reads capacitively through 2 mm of PLA — it does not
   need a hole.
6. Push the wheels onto the D-shafts.

## Fit notes

These are the tolerances I printed against. If your printer runs tight, scale
the body 100.5% rather than reprinting everything.

- Motor bracket bore is sized for a standard 12 mm N20 barrel.
- The OLED window assumes the common 27 × 27 mm 4-pin I²C module. The 0.96"
  boards with the pin header on the long edge fit; the ones with it on the short
  edge do not.
- There's no strain relief on the USB port. Be gentle with it, or add a dab of
  hot glue once you've finished flashing.
