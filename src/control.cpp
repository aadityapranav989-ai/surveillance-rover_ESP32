#include <Arduino.h>

#include "control.h"
#include "motors.h"
#include "udp_control.h"
#include "watchdog.h"

extern bool motionActive;
extern unsigned long motionStart;
extern unsigned long motionDuration;

static SemaphoreHandle_t controlMutex = nullptr;

void lockControl()
{
    if (controlMutex != nullptr)
        xSemaphoreTakeRecursive(controlMutex, portMAX_DELAY);
}

void unlockControl()
{
    if (controlMutex != nullptr)
        xSemaphoreGiveRecursive(controlMutex);
}

static void controlTask(void *)
{
    for (;;)
    {
        {
            ControlGuard guard;
            processUdpControl();
            // Automatic stop after duration, easing down instead of cutting power.
            if (motionActive && millis() - motionStart >= motionDuration)
            {
                easeToStop();
                motionActive = false;
            }
            updateMotors();
            watchdogUpdate();
        }
        vTaskDelay(1);  // 1 ms: lets lower-priority work run
    }
}

void initControlTask()
{
    controlMutex = xSemaphoreCreateRecursiveMutex();
    // Core 1 (where the Arduino loop also runs), at a higher priority than the loop.
    xTaskCreatePinnedToCore(controlTask, "control", 4096, nullptr, 3, nullptr, 1);
}
