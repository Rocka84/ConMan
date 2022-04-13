#include <Arduino.h>
#include "ConMan.h"

ConMan cm("MyDevice");

bool toggle = false;

void loop() {
    cm.loop();
}

void mqttCallback(char* topic, char* payload) {
    if (strcmp(topic, "MyDevice/sometopic") == 0) {
        toggle = cm.payloadToBool(payload, toggle);
        return;
    }

    if (strcmp(topic, "MyDevice/othertopic") == 0) {
        cm.mqttPublish("MyDevice/thirdtopic", payload);
        return;
    }
}

void setup() {
    cm.checkStartConfig();

    cm.wifiAutoSetup();
    cm.setupServer();
    cm.serverOn("/", []() {
        cm.getServer()->send(200, "text/html", "<b>Hello World</b>");
    });

    if (!cm.isApMode()) {
        cm.setupMqtt(mqttCallback);
    }

    cm.mqttPublish("MyDevice/status", "unknown");
}

