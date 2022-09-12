#include <Arduino.h>
#include <WifiController.h>
#include "credentials.h"
#include <PubSubClient.h>
#include "PlantSoilMoistureUpdate.h"
#include <ArduinoJson.h>

#define uS_TO_S_FACTOR 1000000ULL
#define TIME_TO_SLEEP 2*60*60

void setup()
{
    Serial.begin(115200);
    esp_sleep_enable_timer_wakeup(15000000);
    WifiController wifiController = WifiController();
    PubSubClient mqttClient = PubSubClient();
    WiFiClient wifiClient = WiFiClient();

    wifiController.setHostname(hostname).setSSID(wiFiSSID).setPassword(wiFiPassword);
    wifiController.connect();

    mqttClient.setServer(mqttServer, mqttPort);
    mqttClient.setClient(wifiClient);
    mqttClient.connect(hostname);

    PlantSoilMoistureUpdate soil_moisture_updates[2] {
            PlantSoilMoistureUpdate {.plant_name = "palm_tree_office", .value = analogRead(
                    34), .critical_value = 2000, .is_critical_value = false},
            PlantSoilMoistureUpdate {.plant_name = "leafy_tree_office", .value = analogRead(
                    35), .critical_value = 2000, .is_critical_value = false}
    };

    char json_buffer[250];
    StaticJsonDocument<250> json_doc;

    for (PlantSoilMoistureUpdate& update: soil_moisture_updates) {
        json_doc["PlantName"] = update.plant_name;
        json_doc["Value"] = update.value;

        if (update.value > update.critical_value) {
            update.is_critical_value = true;
        }
        json_doc["IsCriticalValue"] = update.is_critical_value;

        serializeJson(json_doc, json_buffer);

        char final_publish_topic[strlen(publish_topic) + strlen(update.plant_name)];
        strcpy(final_publish_topic, publish_topic);
        strcat(final_publish_topic, update.plant_name);

        mqttClient.publish(final_publish_topic, json_buffer, true);
    }

    mqttClient.disconnect();
    wifiController.disconnect();
    delay(100);

    esp_deep_sleep_start();
}

void loop()
{

}
