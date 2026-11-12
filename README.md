# NTP Clock ESP32

A Wi-Fi clock built on an **ESP32-C3 PLUS** and a **4 segments 8x8 MAX7219 LED matrix module**. It sets itself over the internet using NTP, adjusts for your time zone (including daylight saving time), dims automatically at night, and is configured from a web page served by the clock itself, so there is no need to edit or re-flash the code to change Wi-Fi or settings.

## Contents

- [Features](#features)
- [Hardware](#hardware)
- [3D-printed enclosure](#3d-printed-enclosure)
- [Wiring](#wiring)
- [Flashing with the Arduino IDE](#flashing-with-the-arduino-ide)
- [First start and web setup page](#first-start-and-web-setup-page)
- [Web server settings reference](#web-server-settings-reference)
- [Everyday use](#everyday-use)
- [Troubleshooting](#troubleshooting)
- [Customizing the firmware](#customizing-the-firmware)
- [License](#license)

## Features

- Time synchronised via NTP (default server `ua.pool.ntp.org`, configurable)
- Time zone presets with automatic daylight saving time
- `HH:MM` display with an optional seconds counter
- Colon style: blinking, always on, or always off
- Separate day and night brightness, switched automatically at 06:00 and 22:00
- Built-in setup web page, served from the clock's own Wi-Fi access point
- Settings are stored in flash (ESP32 `Preferences`) and survive power loss
- Hold the button for 3 seconds to erase the settings and return to setup mode
- Re-syncs with the NTP server every 24 hours and at midnight

## Hardware

| Part | Notes |
|------|-------|
| ESP32-C3 PLUS development board | Any ESP32-C3 PLUS board with a BOOT button on GPIO 9 and USB Type-C |
| 4 segments 8x8 MAX7219 LED matrix module | **FC16** type. Available in green, red or blue; the colour makes no difference to the firmware, so pick whichever you like |
| USB Type-C cable | For flashing and power |
| 5 jumper wires | Display connection |
| 3D-printed enclosure | Housing, back cover and button, see [3D-printed enclosure](#3d-printed-enclosure) |

As a color filter I used transparent green [binding covers](https://rozetka.com.ua/ua/d_a_art_1220102020400/p24951457/) that were inserted into the groove of the housing. 5 pieces measuring 133 mm x 34 mm should be enough to cover the white dots of the LED matrix.

> [!WARNING]
> **Use a 5V-only power supply.** If you power the clock from an external USB charger or adapter, it must supply a plain **5V**. Do **not** use chargers or cables that negotiate higher voltages, such as **USB Power Delivery (PD 3.0)** or other quick-charge standards (Qualcomm Quick Charge, etc.). Higher voltages can permanently damage the ESP32-C3 PLUS board and the display module. A standard 5V USB adapter (for example 5V / 2A) is enough.

## 3D-printed enclosure

The `3D models` folder contains a three-part enclosure for the display and the ESP32-C3 board.

| Part | File | Size (mm) | Quantity |
|------|------|-----------|----------|
| Housing | `Clock_housing.STL` | 150 × 55.6 × 40 | 1 |
| Back cover | `Clock_back_cover.STL` | 145.5 × 46 × 9 | 1 |
| Button | `Clock_button.STL` | 8 × 3.5 × 3.5 | 1 |

`Clock.3mf` is a ready-to-slice PrusaSlicer project that already contains all three parts laid out on the bed. Use the individual `.STL` files if you prefer another slicer.

## Wiring

The display uses the pins defined at the top of `ntp_clock.ino`:

| MAX7219 module | ESP32-C3 PLUS |
|----------------|---------------|
| `VCC` | 5V |
| `GND` | GND |
| `DIN` | GPIO 6 |
| `CS` | GPIO 7 |
| `CLK` | GPIO 4 |

The setup/reset button is the board's **BOOT** button on GPIO 9, so no extra button is needed.

If you wire the display differently, change `CLK_PIN`, `DATA_PIN` and `CS_PIN` in the firmware.

## Flashing with the Arduino IDE

### 1. Install the Arduino IDE

Download and install **Arduino IDE 2.x** from <https://www.arduino.cc/en/software>.

### 2. Add ESP32 support

1. Open **File → Preferences** (on macOS: **Arduino IDE → Settings**).
2. In **Additional boards manager URLs**, add:
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
3. Click **OK**.
4. Open **Tools → Board → Boards Manager…**, search for **esp32**, and install **esp32 by Espressif Systems**.

### 3. Install the libraries

Open **Sketch → Include Library → Manage Libraries…** and install:

| Library | Author |
|---------|--------|
| **MD_Parola** | MajicDesigns |
| **MD_MAX72XX** | MajicDesigns |

If the Library Manager offers to install dependencies for MD_Parola, choose **Install all**.

`WiFi`, `WebServer`, `Preferences`, `SPI`, `time.h` and `esp_sntp.h` are part of the ESP32 board package, so nothing else needs installing.

### 4. Prepare the sketch

Open `ntp_clock.ino` in the Arduino IDE. `font_data.h` appears as a second tab.

### 5. Select the board and port

1. Connect the ESP32-C3 PLUS to your computer with a USB cable.
2. **Tools → Board → esp32 → ESP32C3 Dev Module** (choose a more specific entry if your board has one).
3. **Tools → Port** and select the serial port of your board (`COMx` on Windows, `/dev/cu.usbmodem…` on macOS, `/dev/ttyACM0` on Linux).
4. Recommended settings for boards with native USB:

   | Setting | Value |
   |---------|-------|
   | USB CDC On Boot | **Enabled** (only needed if you want Serial Monitor output) |
   | Flash Size | 4MB (match your board) |
   | Partition Scheme | Default 4MB with spiffs |
   | Upload Speed | 921600 (lower to 115200 if uploads fail) |

### 6. Upload

1. Click **Verify** (✓) to compile.
2. Click **Upload** (→).
3. When the IDE reports **Done uploading**, the display should show **`Setup`**, which means the clock is ready for configuration.

### If the board is not detected or the upload fails

Put the ESP32-C3 PLUS into download mode manually:

1. Hold the **BOOT** button.
2. Plug in the USB cable while holding **BOOT**.
3. Release **BOOT**, then upload again.

Also try a different USB cable (some are charge-only) and a different USB port.

## First start and web setup page

The web server runs **only in setup mode**. The clock enters setup mode automatically when:

- it is started for the first time (no saved Wi-Fi settings), or
- it cannot connect to the saved Wi-Fi network within 10 seconds of power-up, or
- you [reset the settings](#entering-setup-mode-again) with the button.

In setup mode the display shows **`Setup`**.

### How to open the settings page

1. On your phone or computer, open the Wi-Fi list and connect to the clock's network:

   | | |
   |---|---|
   | **Network name (SSID)** | `NTPClock-Setup` |
   | **Password** | `12345678` |

2. Open a browser and go to:

   **<http://192.168.50.1>**

   Use `http://`, not `https://`. The page is not encrypted, and there is no captive portal, so you must type the address yourself. If your phone warns that the network has no internet access, stay connected. If the page does not load, turn off mobile data.
3. Fill in the form (see the [settings reference](#web-server-settings-reference)) and tap **Save**.
4. The page shows **Settings Saved**, and the clock restarts after about one second, connects to your Wi-Fi, shows `WiFi on` and `Syncing`, and then starts displaying the time.

## Web server settings reference

### Server

| Setting | Value |
|---------|-------|
| Mode | Wi-Fi access point (setup mode only) |
| Access point SSID | `NTPClock-Setup` |
| Access point password | `12345678` |
| IP address / gateway | `192.168.50.1` |
| Subnet mask | `255.255.255.0` |
| Web server port | `80` (HTTP) |
| Routes | `GET /` serves the settings page; `POST /save` stores the settings and restarts the clock |

### Settings on the page

| Field | Options | Default on the page | Notes |
|-------|---------|---------------------|-------|
| **Wi-Fi SSID** | text | – | Your home network. **2.4 GHz only**; the ESP32-C3 PLUS does not support 5 GHz |
| **Wi-Fi Password** | text | – | The lock icon shows or hides the password |
| **Timezone** | see table below | Eastern Europe (Greece, Ukraine) | Daylight saving time is handled automatically |
| **NTP Server Address** | hostname | `ua.pool.ntp.org` | Choose a pool close to you, e.g. `pool.ntp.org` or `europe.pool.ntp.org` |
| **Colon Style** | Blink every second / Always on / Always off | Blink every second | |
| **Show Seconds** | Disabled / Enabled | Disabled | When enabled, seconds appear on the leftmost module and `HH:MM` on the other three |
| **Day Brightness** | Level 1–15 | Level 5 | Applied from 06:00 |
| **Night Brightness** | Level 1–15 | Level 2 | Applied from 22:00 |

### Time zone presets

| Option on the page | POSIX time zone string |
|--------------------|------------------------|
| GMT/UTC (no DST) | `GMT0` |
| Western Europe (UK, Portugal) | `WET0WEST,M3.5.0/1,M10.5.0` |
| Central Europe (Germany, France) | `CET-1CEST,M3.5.0,M10.5.0/3` |
| Eastern Europe (Greece, Ukraine) | `EET-2EEST,M3.5.0/3,M10.5.0/4` |
| US Eastern (New York) | `EST5EDT,M3.2.0,M11.1.0` |
| US Central (Chicago) | `CST6CDT,M3.2.0,M11.1.0` |
| US Mountain (Denver) | `MST7MDT,M3.2.0,M11.1.0` |
| US Pacific (Los Angeles) | `PST8PDT,M3.2.0,M11.1.0` |
| Australia Eastern (Sydney) | `AEST-10AEDT,M10.1.0,M4.1.0/3` |

Need another time zone? See [Customizing the firmware](#customizing-the-firmware).

### Where settings are stored

Settings are saved in the ESP32's non-volatile storage under the namespace `clock-cfg`, with the keys `ssid`, `password`, `tz`, `ntp`, `colon`, `secs`, `brightDay` and `brightNight`. The Wi-Fi password is stored in plain text in the device's flash.

## Everyday use

### Display messages

| Message | Meaning |
|---------|---------|
| `Setup` | Setup mode: connect to `NTPClock-Setup` and open `http://192.168.50.1` |
| `WiFi on` | Connected to your Wi-Fi network |
| `Syncing` | Waiting for the first NTP response |
| `Hold` | BOOT button is being held, keep holding for 3 seconds to reset |
| `Reset` | Settings have been erased, the clock is restarting |

### Entering setup mode again

The web page is only available in setup mode, so to change any setting later:

1. Hold the **BOOT** button for **3 seconds**. The display shows `Hold`.
2. Release **BOOT** button after `Reset` message appears on the display.
3. The clock erases **all** saved settings and restarts into setup mode (`Setup`).
4. Connect to `NTPClock-Setup` Wi-Fi access point, open `http://192.168.50.1` and enter all settings again.

If you release the button before 3 seconds, the clock simply restarts and the settings are kept.

## Troubleshooting

| Problem | Things to try |
|---------|---------------|
| Display shows `Setup` after every reboot | The clock cannot join your Wi-Fi. Check the SSID and password, make sure the network is 2.4 GHz, and move the clock closer to the router. If the router was off at power-up, restart the clock; saved settings are not erased |
| Cannot open `http://192.168.50.1` | Check that you are connected to `NTPClock-Setup`, use `http://` (not `https://`), turn off mobile data on your phone, and disable any VPN |
| Display stays on `Syncing` | Wi-Fi connected but NTP is unreachable. Try another NTP server (e.g. `pool.ntp.org`) via a settings reset |
| Wrong time (off by whole hours) | Wrong time zone selected |
| Display is blank or garbled | Check the wiring and the pin definitions, and make sure the module is an FC16-type 4 segments 8x8 matrix module |
| Upload fails | See [If the board is not detected or the upload fails](#if-the-board-is-not-detected-or-the-upload-fails) |
| Compile error about `MD_MAX72XX` or `MD_Parola` | Install both libraries as described in [step 3](#3-install-the-libraries) |

## Customizing the firmware

- **Different pins:** change `CLK_PIN`, `DATA_PIN` and `CS_PIN`.
- **Access point name and password:** edit the `WiFi.softAP("NTPClock-Setup", "12345678")` line, since the default password is public.
- **Access point address:** edit `local_ip`, `gateway` and `subnet`.
- **Additional time zones:** add a new `<div class="option-item" data-value="…">` entry to the time zone list in `HTML_PAGE`, using a [POSIX TZ string](https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv).
- **Brightness switching times:** the 06:00 and 22:00 switch times are set in `setup()` and `loop()`.
- **Digit shapes:** edit `font_data.h`. `numeric7Se` holds the `HH:MM` digits and `numeric7Seg` the narrower digits used for seconds.

## License

This project is licensed under the **GNU General Public License v3.0**. See [LICENSE](LICENSE) for details.
