#include <ArduinoJson.h>

enum MType {Temp, Humidity, Pressure};

typedef struct measure
{
    MType type;
    double val;
    long timestamp;
} 
measure;