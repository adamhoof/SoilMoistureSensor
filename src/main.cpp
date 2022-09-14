#include <Arduino.h>
#include "credentials.h"
#include <WifiController.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "PlantSoilMoistureUpdate.h"
#include <OTAHandler.h>

#define uS_TO_S_FACTOR 1000000ULL
#define TIME_TO_SLEEP 12*60*60

void setOnWakeUpHandlers()
{
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

    switch (wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0 : {
            Serial.println("Wakeup caused by external signal using RTC_0");

            OTAHandler::setEvents();
            OTAHandler::init();

            unsigned long initTime = millis();
            unsigned long lastMeasuredMillis = millis();
            unsigned long keepAlive = 20000;

            while (initTime + lastMeasuredMillis <= keepAlive) {
                OTAHandler::maintainConnection();
                lastMeasuredMillis = millis();
            }
        }
            break;
        case ESP_SLEEP_WAKEUP_EXT1 :
            Serial.println("Wakeup caused by external signal using RTC_CNTL");
            break;
        case ESP_SLEEP_WAKEUP_TIMER :
            Serial.println("Wakeup caused by timer");
            break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD :
            Serial.println("Wakeup caused by touchpad");
            break;
        case ESP_SLEEP_WAKEUP_ULP :
            Serial.println("Wakeup caused by ULP program");
            break;
        default :
            Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
            break;
    }
}

void setup()
{
    Serial.begin(115200);

    WifiController wifiController = WifiController();
    wifiController.setHostname(hostname).setSSID(wiFiSSID).setPassword(wiFiPassword);
    wifiController.connect();

    setOnWakeUpHandlers();

    esp_sleep_enable_ext0_wakeup(GPIO_NUM_37, 1);
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

    PubSubClient mqttClient = PubSubClient();
    WiFiClient wifiClient = WiFiClient();

    mqttClient.setServer(mqttServer, mqttPort);
    mqttClient.setClient(wifiClient);
    mqttClient.connect(hostname);

    PlantSoilMoistureUpdate soil_moisture_updates[2] {
            PlantSoilMoistureUpdate {.plant_name = "leafy_tree_office", .value = analogRead(
                    35), .critical_value = 1750, .is_critical_value = false},
            PlantSoilMoistureUpdate {.plant_name = "palm_tree_office", .value = analogRead(
                    34), .critical_value = 1750, .is_critical_value = false}
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

        bool successfulPublish = mqttClient.publish(final_publish_topic, json_buffer, true);
        while (!successfulPublish){
            mqttClient.disconnect();
            mqttClient.connect(hostname);
            delay(100);
            successfulPublish = mqttClient.publish(final_publish_topic, json_buffer, true);
            delay(100);
        }
        delay(100);
    }

    mqttClient.disconnect();
    wifiController.disconnect();
    delay(100);

    esp_deep_sleep_start();
}

void loop()
{

}
