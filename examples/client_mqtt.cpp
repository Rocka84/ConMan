#include <Arduino.h>
#include "ConMan.h"

ConMan cm("MyDevice");

void loop() {
    cm.loop();
}

void setup() {
    if (!cm.wifiConnectAsClient() || !cm.setupMqtt()) {
        cm.wifiStartConfig();
        return; //should not be needed, controller is reset after configuring
    }

    cm.mqttPublish("MyDevice/status", "unknown");
}
