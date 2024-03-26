#include "measure.h"

#include <ArduinoJson.h>

#pragma once

class Serializer
{
public:
    void serializeMeasurement(measure * m, StaticJsonDocument<32> doc);
    //TODO possible fragmentation
    DynamicJsonDocument * generateReport(int device_id, measure * m_lst, size_t count);
};