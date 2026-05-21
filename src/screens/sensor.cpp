#include <DHT.h>
#include "config.h"
#include "sensors.h"

DHT dht(DHT_PIN, DHT_TYPE);

static float current_temperature = 0;
static float current_humidity = 0;

void sensors_init() { dht.begin(); }

float get_temperature() {
    float t = dht.readTemperature();
    if (!isnan(t)) current_temperature = t;
    return current_temperature;
}

int get_humidity() {
    float h = dht.readHumidity();
    if (!isnan(h)) current_humidity = h;
    return (int)current_humidity;
}