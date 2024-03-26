#include "serializer.h"

#include <ArduinoJson.h>

void Serializer::serializeMeasurement(measure * m, StaticJsonDocument<32> doc)
{
    static char* map[] = {"temp", "humi", "pres"};

    doc["type"] = map[m->type];
    doc["value"] = m->val;
    doc["timestamp"] = m->timestamp;
}

// DynamicJsonDocument * Serializer::generateReport(int device_id, measure * m_lst, size_t count)
// {
//     DynamicJsonDocument * doc = new DynamicJsonDocument(512);
//     JsonArray readings = doc->createNestedArray("readings");

//     (*doc)["device_id"] = device_id;
//     for(int i = 0; i < count; i++)
//         readings.add(serializeMeasurement(&m_lst[i]));

//     return doc;
// }