#include <Arduino.h>
#include "ConMan.h"

ConMan cm("MyDevice");

void loop() {
    cm.loop();
}

void setup() {
    cm.checkStartConfig();

    cm.wifiAutoSetup();
    cm.setupServer();
    cm.serverOn("/", []() {
        cm.getServer()->send(200, "text/html", "<b>Hello World</b>");
    });

    if (!cm.isApMode()) {
        cm.setupMqtt();
    }

    char* state = "unknown";
    cm.mqttPublish("mydevice/status", state);
}
