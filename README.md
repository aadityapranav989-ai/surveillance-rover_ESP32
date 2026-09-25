# ESP32 Rover Firmware

PlatformIO firmware for the autonomous surveillance rover. The ESP32 is the
rover's self-contained Wi-Fi access point and motor/GPS controller.

## Network contract

```text
Rover Wi-Fi:        TAPIR, hosted by the Raspberry Pi
Wi-Fi password:     set in src/secrets.h (not in Git); the Pi hotspot uses the same one
Raspberry Pi:       192.168.50.1
Pi dashboard:       http://192.168.50.1:8080/
ESP32 address:      192.168.50.2 (fixed)
ESP32 dashboard:    http://192.168.50.2/
```

The Raspberry Pi hosts the rover Wi-Fi (`TAPIR`) and the ESP32 joins it, as
does the laptop. Video then goes straight from the Pi to the laptop, and the
ESP32's small radio only carries drive commands. (Earlier the ESP32 hosted
the network and relayed all the video, which made both the video and the
controls lag.)

If the ESP32 cannot join `TAPIR` for 30 seconds (`FALLBACK_AP_AFTER_MS`),
it also opens its own `ESP32-Robot` network at `192.168.4.1` with the same
password, so it can still be reached. It closes that network again once it
is back on `TAPIR`. The serial monitor shows which network it is on.

## Hardware

Motor driver pins:

- Left BTS7960: RPWM GPIO25, LPWM GPIO26
- Right BTS7960: RPWM GPIO27, LPWM GPIO14

GPS GY-NEO6MV2 on UART2:

- ESP32 RX GPIO16 connected to GPS TX
- ESP32 TX GPIO17 connected to GPS RX
- Baud rate: 9600

## Wi-Fi password

The password lives in `src/secrets.h`, which Git ignores. Create it once
before the first build:

```powershell
copy src\secrets.example.h src\secrets.h
```

Edit `src/secrets.h` and set `ROVER_WIFI_PASSWORD` (8-63 characters). The
build stops with an error if the file is missing or the password is too
short or too long.

Changing the password disconnects every device. Update the Pi's saved Wi-Fi
password before flashing, or you lose SSH access to the headless Pi:

```bash
sudo nmcli connection modify "ESP32-Robot" wifi-sec.psk "NEW_PASSWORD"
```

## Build and upload

From the development computer:

```powershell
cd "D:\Desktop\Hackathon\ESP32 human detection\RadarTest"
pio run
pio device list
pio run --target upload --upload-port COM4
pio device monitor -b 115200
```

Close the serial monitor before uploading because it locks the serial port.
Replace `COM4` with the port shown by `pio device list`.

After reset, the serial monitor should show:

```text
Rover Wi-Fi AP: ESP32-Robot
Dashboard: http://192.168.4.1
```

## HTTP API

```text
GET  http://192.168.4.1/api/status
POST http://192.168.4.1/api/command?direction=FORWARD&speed=80&value=5
POST http://192.168.4.1/api/stop
POST http://192.168.4.1/api/drive?left=180&right=120&ms=500
```

The same commands can be sent as UDP text packets to port 4210
(`CONTROL_UDP_PORT`), for example `DRIVE 180 120 500` or `STOP`. The Pi uses
UDP for driving: it needs no connection, so a lost packet costs nothing (the
next command replaces it), while a lost packet when opening an HTTP
connection delays that command by a full second. `/api/status` reports
`udpPort` so the Pi knows the firmware supports it.

`/api/drive` runs each side at its own speed (-255..255, negative is
reverse) for `ms` milliseconds: equal speeds drive straight, different
speeds drive in a curve, opposite speeds turn on the spot. The Pi uses it
for smooth curved following and for the joystick. The same command works
over the serial monitor as `DRIVE 180 120 500`.

Motor control (the speed ramp, timed stops, the watchdog and UDP drive
commands) runs in its own high-priority FreeRTOS task (`control.cpp`), so a
slow web request or LCD update in the main loop cannot delay it. UDP packets
of the form `LCD first line|second line` update the display without an HTTP
request.

Every start, stop and change of direction ramps over `MOTOR_RAMP_MS`
(250 ms, in `src/config.h`) instead of jumping, so the rover moves without
jolts. `/api/stop`, `STOP` and the watchdog still cut power immediately.

`value` is centimeters for forward/backward and degrees for turns. The
firmware applies timed motion and stops automatically. A new command replaces
the one in progress, so sending short steps faster than they finish gives
continuous movement.

Commands are rejected with `400` unless `speed` is 1-255 and `value` is
greater than 0. A single movement never runs longer than `MAX_MOTION_MS`
(3 s, in `src/config.h`), whatever `value` asks for. The watchdog and the
physical emergency stop remain the primary safety mechanisms.

`GET /api/status` also reports `uptimeMs` (time since the ESP32 started),
`resetReason` (why it last restarted: `brownout` means its supply voltage
dipped, often when the motors start) and `clients` (devices on the rover
Wi-Fi). If the Wi-Fi drops and `uptimeMs` is small afterwards, the ESP32
itself restarted.

## 16x2 LCD

A 16x2 character LCD with an I2C backpack (PCF8574, 4 pins) shows the
rover's security status, sent by the Raspberry Pi: `UNKNOWN PERSON` /
`Tap card: 7s`, `ACCESS GRANTED`, `INTRUDER` / `DETECTED`, `WELCOME` / a
person's name, and so on. If the Pi sends
nothing for 10 seconds (`LCD_PI_TIMEOUT_MS`), the ESP32 shows its own status
instead (`Pi offline`, GPS fix, Wi-Fi clients).

| LCD backpack pin | ESP32 pin |
| --- | --- |
| GND | GND |
| VCC | 5V (VIN) |
| SDA | GPIO21 |
| SCL | GPIO22 |

The ESP32's pins are 3.3 V. Most backpacks pull SDA and SCL up to VCC, so at
5 V they put 5 V on those pins. Most boards tolerate this, but to be safe
remove the backpack's two pull-up resistors (usually marked 472), or use a
small I2C level shifter. The display is found automatically at address 0x27
or 0x3F; the serial monitor prints `LCD: found at 0x27` at start-up. If the
text is invisible, turn the blue contrast screw on the backpack.

The Pi sends text with `POST /api/lcd?line1=...&line2=...` (16 characters
per line). `/api/status` reports `"lcd": true` when a display was found.

## Wi-Fi stability

The access point keeps its radio awake at full transmit power, allows up to
`WIFI_AP_MAX_CLIENTS` (8) devices, and uses `WIFI_AP_CHANNEL` (1). If the
connection drops near other Wi-Fi networks, change the channel to 6 or 11 in
`src/config.h` and flash again.

## Raspberry Pi connection

Join the Pi to `ESP32-Robot`, then assign its Wi-Fi connection the fixed
address `192.168.4.10`:

```bash
nmcli connection show --active
sudo nmcli connection modify "ESP32-Robot" ipv4.method manual ipv4.addresses 192.168.4.10/24 ipv4.gateway 192.168.4.1 ipv4.dns 192.168.4.1
sudo nmcli connection down "ESP32-Robot"
sudo nmcli connection up "ESP32-Robot"
curl --max-time 5 http://192.168.4.1/api/status
```

The Pi gateway uses `ESP32_URL=http://192.168.4.1`.
