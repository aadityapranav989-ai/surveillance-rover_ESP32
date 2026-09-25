#pragma once

// Drive commands over UDP (same text as the serial commands, e.g. "DRIVE 150 90 400" or
// "STOP"). UDP needs no connection, so a lost packet costs nothing: the next command
// replaces it. HTTP commands need a new connection each time, and one lost packet
// there delays a command by a full second.
// "LCD first line|second line" packets update the 16x2 display the same way.
void initUdpControl();
void processUdpControl();
