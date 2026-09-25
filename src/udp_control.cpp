#include <Arduino.h>
#include <WiFiUdp.h>

#include "config.h"
#include "serial_cmd.h"
#include "lcd.h"
#include "udp_control.h"

static WiFiUDP udp;

void initUdpControl()
{
    udp.begin(CONTROL_UDP_PORT);
    Serial.printf("UDP drive commands on port %u\n", CONTROL_UDP_PORT);
}

void processUdpControl()
{
    // Handle every waiting packet; commands are short text lines.
    for (int size = udp.parsePacket(); size > 0; size = udp.parsePacket())
    {
        char text[48];
        int length = udp.read(text, sizeof(text) - 1);
        text[length > 0 ? length : 0] = '\0';
        String command(text);
        command.trim();
        if (command.startsWith("LCD "))
        {
            // "LCD first line|second line": shown by the main loop, which owns the display.
            int split = command.indexOf('|');
            lcdQueue(command.substring(4, split < 0 ? command.length() : split),
                     split < 0 ? String("") : command.substring(split + 1));
            continue;
        }
        executeCommand(command);
    }
}
