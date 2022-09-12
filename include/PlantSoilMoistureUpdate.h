#pragma once

#include <Arduino.h>

struct PlantSoilMoistureUpdate
{
    const char* plant_name;
    uint16_t value;
    uint16_t critical_value;
    bool is_critical_value;
};
