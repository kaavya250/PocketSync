# PocketSync 🎧

**PocketSync** is a portable, microSD-card-based offline music player built around an ESP32, a DFPlayer Mini MP3 module, and a 128×64 I²C OLED display. It is designed for simple physical-button control, track browsing, and local music playback without needing an internet connection.

> **Project status:** The ESP32 playback firmware and user-interface code have been prepared. The rechargeable battery and 5 V power-supply stage are still being finalized and must be tested before the player is powered from its battery.

## Features

- Offline MP3 playback from a microSD card using the DFPlayer Mini.
- 128×64 SSD1306 OLED interface.
- Now Playing screen with track number, playback state, volume, and a progress bar.
- Music Library screen for browsing track numbers.
- Five physical buttons for playback, track navigation, and volume control.
- Automatic move to the next track when the DFPlayer reports that a track has finished.
- Adjustable volume from 0 to 30 in the firmware.
- Rechargeable single-cell Li-ion battery system planned around a TP4056 charging module.

## Hardware

### Main components

| Component | Purpose |
|---|---|
| ESP32 development board | Main controller and button/OLED interface |
| DFPlayer Mini | Reads MP3 files from the microSD card and plays audio |
| microSD card | Stores the offline music library |
| 0.96-inch SSD1306 OLED, 128×64, I²C | Displays the player interface |
| 5 momentary push-buttons | Play/Pause, Next, Previous, Volume Up, Volume Down |
| 3.7 V single-cell Li-ion battery | Portable power source (planned) |
| TP4056 Micro-USB charging module | Charges the single-cell battery |
| 2-pin ON/OFF switch | Main player power switch |
| Suitable regulated 5 V DC-DC converter | Supplies the player electronics from the battery (not finalized) |

An **XL6009E1** boost-converter module is available for the project, but it has not been approved for the battery power stage. The XLSEMI datasheet specifies a 5–32 V operating input range for the IC. Do not assume it will reliably convert the full voltage range of a single-cell 3.7 V Li-ion battery to 5 V. See the [XL6009E1 datasheet](https://www.sunrom.com/download/Sunrom-XL6009E1.pdf).

## Firmware and pin connections

The pin assignments below match `PocketSync_ESP32_Corrected.ino`.

### ESP32 to DFPlayer Mini

| ESP32 pin | DFPlayer Mini pin | Description |
|---|---|---|
| GPIO 16 (RX2) | TX | ESP32 receives serial data from the DFPlayer |
| GPIO 17 (TX2) | RX | ESP32 sends commands to the DFPlayer |
| GND | GND | Common ground |
| Appropriate supply | VCC | Use the supply voltage specified for your DFPlayer module |

The firmware uses UART2 at **9600 baud**.

### ESP32 to SSD1306 OLED

| ESP32 pin | OLED pin |
|---|---|
| GPIO 21 | SDA |
| GPIO 22 | SCL |
| 3.3 V or module-approved supply | VCC |
| GND | GND |

The firmware expects an OLED I²C address of **0x3C**. Check the address and supply requirements of your particular OLED board.

### Buttons

Each button connects between its assigned GPIO and **GND**. The firmware configures the pins with `INPUT_PULLUP`, so a pressed button reads LOW.

| Button | ESP32 GPIO | Short-press action |
|---|---:|---|
| Play | 25 | Play/Pause; selects the highlighted track in Library mode |
| Next | 26 | Next track; next library item in Library mode |
| Previous | 27 | Previous track; previous library item in Library mode |
| Volume Up | 32 | Increase volume |
| Volume Down | 33 | Decrease volume |

**Long press:** Hold the **Next** button for about 700 ms to enter or exit Music Library mode.

## Music-card preparation

1. Insert a compatible microSD card into the DFPlayer Mini while the system is powered off.
2. Format the card as FAT32, subject to the card capacity and DFPlayer module's supported limits.
3. Copy MP3 files using sequential four-digit names, for example `0001.mp3`, `0002.mp3`, `0003.mp3`, and so on.
4. Keep the files in the location and naming arrangement supported by your DFPlayer Mini. The firmware uses numeric track indexes; it does not read or display song titles or artist metadata.

The number of tracks is read at startup using the DFPlayer library. If tracks are not detected, check the card format, file naming, card seating, and DFPlayer wiring.

## Firmware setup

### Required software

- Arduino IDE
- ESP32 board support package for Arduino
- **DFRobotDFPlayerMini** library
- **Adafruit GFX Library**
- **Adafruit SSD1306** library

`HardwareSerial` and `Wire` are included with the ESP32 Arduino core.

### Upload steps

1. Install the ESP32 board support package in Arduino IDE.
2. Install the three libraries listed above using Library Manager.
3. Open `PocketSync_ESP32_Corrected.ino`.
4. Select the correct ESP32 board and COM port.
5. Connect the ESP32 by USB for programming.
6. Compile and upload the sketch.
7. Open Serial Monitor at **115200 baud** for startup messages and diagnostics.

The ESP32 USB connection is used for programming and serial debugging. The TP4056's Micro-USB Type-B socket is a **separate battery-charging input**; it is not the ESP32 USB port.

## Button controls

| Control | Action |
|---|---|
| Play, short press | Play or pause the current track |
| Play, in Library mode | Play the highlighted track |
| Next, short press | Move to the next track |
| Next, long press | Open or close Music Library |
| Previous, short press | Move to the previous track |
| Next/Previous in Library mode | Browse the track list |
| Volume Up | Increase volume by one step |
| Volume Down | Decrease volume by one step |

The player wraps around: Next from the last track returns to track 1, and Previous from track 1 selects the last track.

## Display and progress-bar behaviour

The OLED shows the track number, playback state, volume, and total number of detected tracks in the Library screen.

The current firmware uses an **estimated** progress bar with a default track duration of 180 seconds. It does not read the actual elapsed time or duration from the MP3 file. The estimate can therefore differ from the real song length. The firmware also displays track numbers rather than MP3 metadata such as title or artist.

## Rechargeable power system — work in progress

The intended power arrangement is:

```text
5 V USB charger
       │
       ▼
TP4056 Micro-USB charging input
       │
       ▼
3.7 V single-cell Li-ion battery
       │
       ▼
TP4056 protected output (only if the specific board includes protection)
       │
       ▼
2-pin ON/OFF switch on the positive supply line
       │
       ▼
A suitable 3.7 V battery-input to regulated 5 V converter
       │
       ▼
ESP32 / DFPlayer power rail
```

**The battery power stage is not yet finalized.** The exact TP4056 board version and its protection circuitry must be verified. Do not assume that every TP4056 board has battery-protection circuitry just because it has `OUT+` and `OUT−` pads. The chosen converter must accept the battery's full operating voltage range, provide enough current for the ESP32 and DFPlayer, and deliver a stable 5 V output.

Before connecting the ESP32 or DFPlayer to the converter, measure its output with a multimeter and verify that it is stable at 5.0 V. Do not connect the ESP32's USB cable and an external 5 V supply at the same time unless the specific development board supports that power arrangement. Keep the battery disconnected while assembling or changing wiring, prevent shorts, and have a knowledgeable adult check the battery and charging connections. Never use a swollen, damaged, or overheating battery.

## Troubleshooting

| Problem | Checks |
|---|---|
| OLED stays blank | Check SDA/SCL wiring, OLED supply, I²C address (`0x3C`), and GND. |
| `DFPLAYER ERROR!` appears | Check DFPlayer VCC/GND, UART wiring (GPIO 16/17), microSD card, and serial connection. |
| `NO MUSIC FOUND` appears | Check that the card is inserted, FAT32-formatted, and contains correctly named MP3 files. |
| Buttons do not respond | Check that each button connects its GPIO to GND when pressed and that the GPIO numbers match the sketch. |
| No voltage at the power-converter input | With power disconnected, inspect polarity, switch wiring, TP4056 output terminals, and continuity. Measure battery and TP4056 output separately with a multimeter. |
| ESP32 resets when audio starts | The power source may not supply enough current or may have excessive voltage drop. Verify the supply under load and use a correctly rated power stage. |

## Repository layout

A simple repository layout could be:

```text
PocketSync/
├── README.md
└── PocketSync_ESP32_Corrected.ino
```

## Current development status

- [x] ESP32 firmware structure
- [x] DFPlayer Mini serial-control integration in the sketch
- [x] SSD1306 OLED screens
- [x] Five-button controls and Library mode
- [x] Track-change event handling
- [x] Estimated progress-bar display
- [ ] Verify the exact TP4056 board and protection circuit
- [ ] Finalize a battery-compatible regulated 5 V power supply
- [ ] Test the complete battery-powered player under load
- [ ] Replace estimated progress with real track timing, if supported by the selected playback approach

## License

No license has been selected yet. Add a `LICENSE` file before indicating that others may reuse, modify, or distribute this project.
