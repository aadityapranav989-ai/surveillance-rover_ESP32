# ESP32 Rover Firmware

PlatformIO firmware for the autonomous surveillance rover. The ESP32 is the
rover's self-contained Wi-Fi access point and motor/GPS controller.

## Network contract

```text
ESP32 access point:  ESP32-Robot
Wi-Fi password:     set in src/secrets.h (not in Git)
ESP32 address:      192.168.4.1
Raspberry Pi:       192.168.4.10
Pi dashboard:       http://192.168.4.10:8080/
ESP32 dashboard:    http://192.168.4.1/
```

The ESP32 does not connect to a router or phone hotspot. The Pi, laptop, and
camera device join `ESP32-Robot` directly.

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

`/api/drive` runs each side at its own speed (-255..255, negative is
reverse) for `ms` milliseconds: equal speeds drive straight, different
speeds drive in a curve, opposite speeds turn on the spot. The Pi uses it
for smooth curved following and for the joystick. The same command works
over the serial monitor as `DRIVE 180 120 500`.

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
