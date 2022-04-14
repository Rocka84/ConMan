#include <Arduino.h>
#include "ConMan.h"

ConMan cm("MyDevice");

bool toggle = false;

void loop() {
    cm.loop();
}

void mqttCallback(char* topic, char* payload) {
    if (strcmp(topic, "MyDevice/command/toggle") == 0) {
        toggle = cm.payloadToBool(payload, toggle);
        cm.mqttPublish("MyDevice/status/toggle", toggle ? "ON" : "OFF");
        return;
    }

    if (strcmp(topic, "MyDevice/configure") == 0) {
        cm.restartIntoConfig();
        return;
    }
}

void setup() {
    cm.checkStartConfig();

    cm.wifiAutoSetup();
    cm.setupServer();

    cm.serverOn("/", []() {
        char buffer[80];
        sprintf(buffer, "<b>Hello World</b><br />IP: %s<br />Toggle: <b>%s</b>", cm.getIP().toString().c_str(), toggle ? "ON" : "OFF");
        cm.getServer()->send(200, "text/html", buffer);
    });

    cm.serverOn("/toggle", []() {
        toggle = !toggle;
        cm.getServer()->send(200, "text/html", toggle ? "ON" : "OFF");
    });

    if (!cm.isApMode()) {
        cm.setupMqtt(mqttCallback);
        cm.mqttSubscribe("MyDevice/#");
    }

    cm.mqttPublish("MyDevice/status/toggle", "unknown");
    cm.mqttPublish("MyDevice/status/ip", cm.getIP().toString().c_str());
}

