#pragma once

void initMotors();

// Sets the speed each side ramps towards (-255..255).
void setMotor(int leftSpeed, int rightSpeed);

// Moves the outputs towards the requested speeds; call every loop.
void updateMotors();

// Ramps both sides down to a stop (end of a timed movement).
void easeToStop();

// Cuts both sides immediately (STOP command and watchdog).
void stopMotors();
