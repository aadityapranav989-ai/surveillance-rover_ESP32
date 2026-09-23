#pragma once

bool executeCommand(const String &cmd);
// Runs the left and right sides at their own speeds (-255..255) for durationMs.
bool executeDrive(int leftSpeed, int rightSpeed, long durationMs);
void processSerial();