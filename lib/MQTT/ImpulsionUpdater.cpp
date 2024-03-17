#include <ImpulsionUpdater.h>
#include <ESP8266WiFi.h>

ImpulsionUpdater::ImpulsionUpdater(Settings& settings, MqttClient& mqttClient)
    : m_settings(settings)
    , m_mqttClient(mqttClient)
    , button_d1(D1)
    , button_d3(D3)
    , button_d4(D4)
{
    if(enabled_d1) {button_d1.setDebounceTime(50);}
    if(enabled_d3) {button_d3.setDebounceTime(50);}
    if(enabled_d4) {button_d4.setDebounceTime(50);}
}

void ImpulsionUpdater::sendDiscoverableDevices()
{
    if(enabled_d1) {_sendDiscovery("D1");}
    if(enabled_d3) {_sendDiscovery("D3");}
    if(enabled_d4) {_sendDiscovery("D4");}
}

void ImpulsionUpdater::loop()
{
    if(enabled_d1) {_checkButton(button_d1, "D1");}
    if(enabled_d3) {_checkButton(button_d3, "D3");}
    if(enabled_d4) {_checkButton(button_d4, "D4");}
}

void ImpulsionUpdater::_checkButton(ezButton& button, String io)
{
    button.loop();
    if(button.isPressed()) {
        _sendButton(io);
    }
}

void ImpulsionUpdater::_sendButton(String io)
{
    StaticJsonDocument<MILIGHT_MQTT_JSON_BUFFER_SIZE> json;
    JsonObject message = json.to<JsonObject>();

    message["action"] = true;

    if (json.overflowed()) {
        Serial.println(F("ERROR: State is too large for MQTT buffer, continuing anyway. Consider increasing MILIGHT_MQTT_JSON_BUFFER_SIZE."));
    }

    size_t documentSize = measureJson(message);
    char buffer1[documentSize + 1];
    serializeJson(json, buffer1, sizeof(buffer1));

    String topic = m_settings.mqttStateTopicPattern;
    topic.replace(":device_id", "gpio");
    topic.replace(":device_type", "button");
    topic.replace(":group_id", io);

    Serial.println(topic);

    m_mqttClient.send(topic.c_str(), buffer1, false);

    message["action"] = false;

    documentSize = measureJson(message);
    char buffer2[documentSize + 1];
    serializeJson(json, buffer2, sizeof(buffer2));

    m_mqttClient.send(topic.c_str(), buffer2, false);
}

void ImpulsionUpdater::_sendDiscovery(String io)
{
    String topic = m_settings.homeAssistantDiscoveryPrefix;
    if (! topic.endsWith("/")) {
        topic += "/";
    }
    topic += "binary_sensor/milight_hub_" + String(ESP.getChipId()) + "/gpio_button_" + io + "/config";


    DynamicJsonDocument config(1024);

    // Unique ID for this device + alias combo
    char uniqueIdBuffer[30];
    snprintf_P(uniqueIdBuffer, sizeof(uniqueIdBuffer), PSTR("%X-gpio-%s"), ESP.getChipId(), io.c_str());

    // String to ID the firmware version
    char fwVersion[100];
    snprintf_P(fwVersion, sizeof(fwVersion), PSTR("esp8266_milight_hub v%s"), QUOTE(MILIGHT_HUB_VERSION));

    // URL to the device
    char deviceUrl[23];
    snprintf_P(deviceUrl, sizeof(deviceUrl), PSTR("http://%s"), WiFi.localIP().toString().c_str());

    config[F("schema")] = F("json");
    config[F("name")] = io;
    config[F("stat_t")] = "milight/updates/gpio/button/" + io;
    config[F("dev_cla")] = "update";
    config[F("uniq_id")] = uniqueIdBuffer;
    config[F("value_template")] = "{{ value_json.action }}";
    config[F("payload_off")] = false;
    config[F("payload_on")] = true;
    config[F("icon")] = "mdi:gesture-double-tap";

    JsonObject deviceMetadata = config.createNestedObject(F("device"));
    deviceMetadata[F("name")] = m_settings.hostname;
    deviceMetadata[F("sw")] = fwVersion;
    deviceMetadata[F("mf")] = F("espressif");
    deviceMetadata[F("mdl")] = QUOTE(FIRMWARE_VARIANT);
    deviceMetadata[F("identifiers")] = String(ESP.getChipId());
    deviceMetadata[F("cu")] = deviceUrl;

    String message;
    serializeJson(config, message);

    m_mqttClient.send(topic.c_str(), message.c_str(), true);
}
