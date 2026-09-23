#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>

#include "config.h"
#include "gps.h"
#include "serial_cmd.h"

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
.stick{position:absolute;width:76px;aspect-ratio:1;left:50%;top:50%;transform:translate(-50%,-50%);border:0;border-radius:50%;background:#2e8bdb;z-index:1}
button{min-height:58px;border:0;border-radius:8px;background:#2e8bdb;color:white;font-size:16px;font-weight:600}
button:active{background:#55d6be;color:#17212b}.stop{background:#d94b4b;grid-column:2}
label{display:block;margin:10px 0 4px}input{width:100%;box-sizing:border-box;padding:10px;border-radius:6px;border:1px solid #647484;background:#243442;color:white;font-size:16px}
small{color:#b9c5cf}#gps{line-height:1.7}
</style></head><body><h1>ESP32 Rover</h1><small>Navigation control and GPS monitor</small>
<div class="status" id="gps">Loading GPS...</div>
<label for="speed">Speed (0-255)</label><input id="speed" type="number" min="1" max="255" value="150">
<div class="mode"><button onclick="setMode('dpad')">D-pad</button><button onclick="setMode('joystick')">Joystick</button></div>
<div id="dpad" class="grid"><span></span><button onclick="move('FORWARD',150,5)">Forward</button><span></span>
<button onclick="move('LEFT',150,10)">Left</button><button class="stop" onclick="stop()">STOP</button><button onclick="move('RIGHT',150,10)">Right</button>
<span></span><button onclick="move('BACKWARD',150,5)">Backward</button><span></span></div>
<div id="joystick" class="joystick"><button class="stick" id="stick"></button></div>
<button class="stop" onclick="stop()">STOP</button>
<script>
async function move(direction,speed,value){await fetch(`/api/command?direction=${direction}&speed=${Math.min(Number(document.querySelector('#speed').value),speed)}&value=${value}`,{method:'POST'});}
async function stop(){await fetch('/api/stop',{method:'POST'});}
function setMode(mode){stop();document.querySelector('#dpad').style.display=mode==='dpad'?'grid':'none';document.querySelector('#joystick').style.display=mode==='joystick'?'block':'none';}
const joystick=document.querySelector('#joystick'),stick=document.querySelector('#stick');let active=false,xAxis=0,yAxis=0,timer;
function joystickCommand(){const distance=Math.hypot(xAxis,yAxis);if(distance<.12)return stop();const speed=Math.max(30,Math.round(distance*255));move(Math.abs(xAxis)>Math.abs(yAxis)?(xAxis<0?'LEFT':'RIGHT'):(yAxis<0?'FORWARD':'BACKWARD'),speed,Math.abs(xAxis)>Math.abs(yAxis)?10:5);}
function updateJoystick(event){const box=joystick.getBoundingClientRect(),radius=box.width/2,limit=radius-40;let x=event.clientX-box.left-radius,y=event.clientY-box.top-radius,length=Math.hypot(x,y);if(length>limit){x*=limit/length;y*=limit/length;}xAxis=x/limit;yAxis=y/limit;stick.style.left=`${50+xAxis*40}%`;stick.style.top=`${50+yAxis*40}%`;joystickCommand();}
function releaseJoystick(){active=false;clearInterval(timer);xAxis=0;yAxis=0;stick.style.left='50%';stick.style.top='50%';stop();}
joystick.addEventListener('pointerdown',event=>{active=true;joystick.setPointerCapture(event.pointerId);updateJoystick(event);timer=setInterval(()=>{if(active)joystickCommand();},120);});joystick.addEventListener('pointermove',event=>{if(active)updateJoystick(event);});joystick.addEventListener('pointerup',releaseJoystick);joystick.addEventListener('pointercancel',releaseJoystick);setMode('joystick');
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
    String json = "{\"wifi\":\"" + address.toString() + "\",\"ap\":" + (accessPointMode ? "true" : "false") + ",\"motionActive\":" + (motionActive ? "true" : "false") + ",\"gps\":{";
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

static void handleStop()
{
    executeCommand("STOP");
    sendJson("{\"ok\":true}");
}

void initWebServer()
{
    accessPointMode = true;
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(WIFI_AP_IP, WIFI_AP_GATEWAY, WIFI_AP_SUBNET);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD);
    Serial.print("\nRover Wi-Fi AP: ");
    Serial.println(WIFI_AP_SSID);
    Serial.print("Dashboard: http://");
    Serial.println(WiFi.softAPIP());

    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/status", HTTP_GET, handleStatus);
    server.on("/api/command", HTTP_POST, handleCommand);
    server.on("/api/stop", HTTP_POST, handleStop);
    server.begin();
}

void processWebServer()
{
    server.handleClient();
}
