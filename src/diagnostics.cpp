#include <Arduino.h>
#include <esp_system.h>

#include "diagnostics.h"

const char *resetReasonName()
{
    switch (esp_reset_reason())
    {
    case ESP_RST_POWERON:
        return "power-on";
    case ESP_RST_SW:
        return "software";
    case ESP_RST_PANIC:
        return "crash";
    case ESP_RST_INT_WDT:
    case ESP_RST_TASK_WDT:
    case ESP_RST_WDT:
        return "watchdog";
    case ESP_RST_BROWNOUT:
        return "brownout";
    case ESP_RST_EXT:
        return "reset-pin";
    case ESP_RST_DEEPSLEEP:
        return "deep-sleep";
    default:
        return "unknown";
    }
}
