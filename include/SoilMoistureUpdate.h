#pragma once

#include <Arduino.h>

struct SoilMoistureUpdate
{
    const char* plant_name;
    uint16_t value;
    uint16_t critical_value;
};