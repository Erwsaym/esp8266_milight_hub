#pragma once

#include <MqttClient.h>
#include <Settings.h>
#include <ezButton.h>

#ifndef MILIGHT_MQTT_JSON_BUFFER_SIZE
#define MILIGHT_MQTT_JSON_BUFFER_SIZE 1024
#endif

class ImpulsionUpdater {
public:
    ImpulsionUpdater(Settings& settings, MqttClient& mqttClient);

    void sendDiscoverableDevices();
    void loop();

private:
    Settings& m_settings;
    MqttClient& m_mqttClient;

    ezButton button_d1, button_d3, button_d4;
    bool enabled_d1{true}, enabled_d3{false}, enabled_d4{false};

    void _checkButton(ezButton& button, String io);
    void _sendButton(String io);
    void _sendDiscovery(String io);
};