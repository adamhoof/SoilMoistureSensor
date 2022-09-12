#include <Arduino.h>
#include <WifiController.h>
#include "credentials.h"
#include <PubSubClient.h>
#include "SoilMoistureUpdate.h"
#include "PlantData.h"
#include <ArduinoJson.h>

void setup()
{
    Serial.begin(115200);
    esp_sleep_enable_timer_wakeup(10000000);
    WifiController wifiController = WifiController();
    PubSubClient mqttClient = PubSubClient();
    WiFiClient wifiClient = WiFiClient();

    wifiController.setHostname(hostname).setSSID(wiFiSSID).setPassword(wiFiPassword);
    wifiController.connect();

    mqttClient.setServer(mqttServer, mqttPort);
    mqttClient.setClient(wifiClient);
    mqttClient.connect(hostname);

    SoilMoistureUpdate soil_moisture_updates[2] {
            SoilMoistureUpdate {.plant_name = "palm_tree_office", .value = analogRead(34), .critical_value = 2000},
            SoilMoistureUpdate {.plant_name = "leafy_tree_office", .value = analogRead(35), .critical_value = 2000}
    };

    char json_buffer[250];
    StaticJsonDocument<250> json_doc;

    for (SoilMoistureUpdate& soil_moisture_update: soil_moisture_updates) {
        if (soil_moisture_update.value > 2000) {
            soil_moisture_update.critical_value = true;
        }

        json_doc["FlowerName"] = soil_moisture_update.plant_name;
        json_doc["SensorValue"] = soil_moisture_update.value;
        json_doc["IsCriticalValue"] = soil_moisture_update.critical_value;
    }

    serializeJson(json_doc, json_buffer);
    mqttClient.publish(publish_topic, json_buffer);

    memset(json_buffer, 0, sizeof json_buffer);

    mqttClient.disconnect();

    wifiController.disconnect();
    esp_deep_sleep_start();
}

void loop()
{

}
