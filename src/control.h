#pragma once

// Motor control runs in its own high-priority task, so a slow web request or LCD
// update in the main loop can never delay drive commands or the speed ramp.
void initControlTask();

// Commands can arrive from the control task (UDP) and the main loop (web, serial);
// this lock keeps them from changing the motors at the same time. It is recursive,
// so code already holding it can take it again.
void lockControl();
void unlockControl();

struct ControlGuard
{
    ControlGuard() { lockControl(); }
    ~ControlGuard() { unlockControl(); }
};
