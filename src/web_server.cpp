#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>

#include "config.h"
#include "gps.h"
#include "serial_cmd.h"
#include "diagnostics.h"
#include "lcd.h"

static WebServer server(80);
static bool accessPointMode = false;

extern bool motionActive;

static const char DASHBOARD[] PROGMEM = R"rawliteral(
<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32 Rover</title><style>
body{font-family:system-ui,sans-serif;max-width:680px;margin:0 auto;padding:20px;background:#17212b;color:#f5f7fa}
h1{margin:0 0 8px;color:#55d6be}.status{padding:12px;background:#243442;border-radius:8px;margin:12px 0}
.grid{display:grid;grid-template-columns:repeat(3,1fr);gap:10px;max-width:360px;margin:20px auto}
.mode{display:flex;gap:8px;margin:16px 0}.mode button{min-height:42px;flex:1}
.joystick{display:none;width:min(72vw,280px);aspect-ratio:1;margin:20px auto;background:#243442;border:2px solid #647484;border-radius:50%;position:relative;touch-action:none}
.joystick:after{content:'';position:absolute;inset:18%;border:1px dashed #647484;border-radius:50%}
.stick{position:absolute;width:76px;aspect-ratio:1;left:50%;top:50%;transform:translate(-50%,-50%);border:0;border-radius:50%;background:#2e8bdb;z-index:1;pointer-events:none}
button{min-height:58px;border:0;border-radius:8px;background:#2e8bdb;color:white;font-size:16px;font-weight:600}
button:active{background:#55d6be;color:#17212b}.stop{background:#d94b4b;grid-column:2}
label{display:block;margin:10px 0 4px}input{width:100%;box-sizing:border-box;padding:10px;border-radius:6px;border:1px solid #647484;background:#243442;color:white;font-size:16px}
small{color:#b9c5cf}#gps{line-height:1.7}
</style></head><body><h1>ESP32 Rover</h1><small>Navigation control and GPS monitor</small>
<div class="status" id="gps">Loading GPS...</div>
<label for="speed">Speed (1-255)</label><input id="speed" type="number" min="1" max="255" value="150">
<div class="mode"><button onclick="stop();setMode('dpad')">D-pad</button><button onclick="stop();setMode('joystick')">Joystick</button></div>
<div id="dpad" class="grid"><span></span><button onclick="move('FORWARD',speedLimit(),STEP.FORWARD)">Forward</button><span></span>
<button onclick="move('LEFT',speedLimit(),STEP.LEFT)">Left</button><button class="stop" onclick="stop()">STOP</button><button onclick="move('RIGHT',speedLimit(),STEP.RIGHT)">Right</button>
<span></span><button onclick="move('BACKWARD',speedLimit(),STEP.BACKWARD)">Backward</button><span></span></div>
<div id="joystick" class="joystick"><div class="stick" id="stick"></div></div>
<button class="stop" onclick="stop()">STOP</button>
<script>
const $=s=>document.querySelector(s),speedEl=$('#speed'),joystick=$('#joystick'),stick=$('#stick');
// cm for FORWARD/BACKWARD, degrees for LEFT/RIGHT. Each step lasts slightly longer than the 150 ms joystick tick, so held movement is continuous.
const STEP={FORWARD:12,BACKWARD:12,LEFT:15,RIGHT:15};
function speedLimit(){return Math.max(1,Math.min(255,Number(speedEl.value)||150));}
// One request at a time: extra joystick ticks are dropped instead of queuing on the ESP32.
let busy=false;
async function move(direction,speed,value){if(busy)return;busy=true;try{await fetch(`/api/command?direction=${direction}&speed=${speed}&value=${value}`,{method:'POST'});}catch(e){}finally{busy=false;}}
async function stop(){try{await fetch('/api/stop',{method:'POST'});}catch(e){}}
function setMode(mode){$('#dpad').style.display=mode==='dpad'?'grid':'none';joystick.style.display=mode==='joystick'?'block':'none';}
let active=false,xAxis=0,yAxis=0,timer,stopped=true;
function joystickTick(){const d=Math.hypot(xAxis,yAxis);if(d<.12){if(!stopped){stopped=true;stop();}return;}stopped=false;const dir=Math.abs(xAxis)>Math.abs(yAxis)?(xAxis<0?'LEFT':'RIGHT'):(yAxis<0?'FORWARD':'BACKWARD');move(dir,Math.min(speedLimit(),Math.max(30,Math.round(d*255))),STEP[dir]);}
function updateJoystick(event){const box=joystick.getBoundingClientRect(),radius=box.width/2,limit=radius-40;let x=event.clientX-box.left-radius,y=event.clientY-box.top-radius,length=Math.hypot(x,y);if(length>limit){x*=limit/length;y*=limit/length;}xAxis=x/limit;yAxis=y/limit;stick.style.left=`${50+xAxis*40}%`;stick.style.top=`${50+yAxis*40}%`;}
function releaseJoystick(){if(!active)return;active=false;clearInterval(timer);xAxis=0;yAxis=0;stick.style.left='50%';stick.style.top='50%';stopped=true;stop();}
joystick.addEventListener('pointerdown',event=>{active=true;joystick.setPointerCapture(event.pointerId);updateJoystick(event);joystickTick();clearInterval(timer);timer=setInterval(joystickTick,150);});
joystick.addEventListener('pointermove',event=>{if(active)updateJoystick(event);});
joystick.addEventListener('pointerup',releaseJoystick);joystick.addEventListener('pointercancel',releaseJoystick);
window.addEventListener('blur',releaseJoystick);document.addEventListener('visibilitychange',()=>{if(document.hidden)releaseJoystick();});
setMode('joystick');
async function refresh(){try{const d=await (await fetch('/api/status')).json();document.querySelector('#gps').innerHTML=d.gps.fix?`GPS fix<br>Lat: ${d.gps.latitude.toFixed(6)}<br>Lon: ${d.gps.longitude.toFixed(6)}<br>Alt: ${d.gps.altitude.toFixed(1)} m | Satellites: ${d.gps.satellites}`:'Waiting for GPS fix';}catch(e){document.querySelector('#gps').textContent='Connection lost';}}
setInterval(refresh,1000);refresh();
</script></body></html>)rawliteral";

static void sendJson(const String &json, int status = 200)
{
    server.send(status, "application/json", json);
}

static void handleRoot()
{
    server.send_P(200, "text/html", DASHBOARD);
}

static void handleStatus()
{
    IPAddress address = accessPointMode ? WiFi.softAPIP() : WiFi.localIP();
    String json = "{\"wifi\":\"" + address.toString() + "\",\"ap\":" + (accessPointMode ? "true" : "false") + ",\"motionActive\":" + (motionActive ? "true" : "false");
    json += ",\"uptimeMs\":" + String(millis()) + ",\"resetReason\":\"" + resetReasonName() + "\"";
    json += ",\"clients\":" + String(WiFi.softAPgetStationNum()) + ",\"lcd\":" + (lcdFound() ? "true" : "false") + ",\"gps\":{";
    json += "\"fix\":" + String(gpsHasFix() ? "true" : "false");
    json += ",\"latitude\":" + String(gpsLatitude(), 6);
    json += ",\"longitude\":" + String(gpsLongitude(), 6);
    json += ",\"altitude\":" + String(gpsAltitudeMeters(), 1);
    json += ",\"satellites\":" + String(gpsSatellites()) + "}}";
    sendJson(json);
}

static void handleCommand()
{
    if (!server.hasArg("direction") || !server.hasArg("speed") || !server.hasArg("value"))
    {
        sendJson("{\"error\":\"direction, speed and value are required\"}", 400);
        return;
    }

    String command = server.arg("direction") + " " + server.arg("speed") + " " + server.arg("value");
    if (!executeCommand(command))
    {
        sendJson("{\"error\":\"invalid navigation command\"}", 400);
        return;
    }
    sendJson("{\"ok\":true}");
}

static void handleDrive()
{
    if (!server.hasArg("left") || !server.hasArg("right") || !server.hasArg("ms"))
    {
        sendJson("{\"error\":\"left, right and ms are required\"}", 400);
        return;
    }
    if (!executeDrive(server.arg("left").toInt(), server.arg("right").toInt(), server.arg("ms").toInt()))
    {
        sendJson("{\"error\":\"left and right must be -255..255 and ms at least 1\"}", 400);
        return;
    }
    sendJson("{\"ok\":true}");
}

static void handleLcd()
{
    // Two lines of text from the Pi, e.g. /api/lcd?line1=UNKNOWN%20PERSON&line2=Tap%20card%3A%207s
    lcdShow(server.arg("line1"), server.arg("line2"));
    sendJson(String("{\"ok\":true,\"lcd\":") + (lcdFound() ? "true" : "false") + "}");
}

static void handleStop()
{
    executeCommand("STOP");
    sendJson("{\"ok\":true}");
}

void initWebServer()
{
    accessPointMode = true;
    WiFi.mode(WIFI_AP);
    // Keep the radio fully awake and at full power so the Pi and laptop stay connected.
    WiFi.setSleep(false);
    WiFi.softAPConfig(WIFI_AP_IP, WIFI_AP_GATEWAY, WIFI_AP_SUBNET);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD, WIFI_AP_CHANNEL, 0, WIFI_AP_MAX_CLIENTS);
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    Serial.print("\nRover Wi-Fi AP: ");
    Serial.println(WIFI_AP_SSID);
    Serial.print("Dashboard: http://");
    Serial.println(WiFi.softAPIP());

    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/status", HTTP_GET, handleStatus);
    server.on("/api/command", HTTP_POST, handleCommand);
    server.on("/api/drive", HTTP_POST, handleDrive);
    server.on("/api/stop", HTTP_POST, handleStop);
    server.on("/api/lcd", HTTP_POST, handleLcd);
    server.begin();
}

void processWebServer()
{
    server.handleClient();
}
