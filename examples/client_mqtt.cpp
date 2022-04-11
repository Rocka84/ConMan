#include <Arduino.h>
#include "ConMan.h"

ConMan cm("MyDevice");

void loop() {
    cm.loop();
}

void setup() {
    if (!cm.wifiConnectAsClient() || !cm.setupMqtt()) {
        cm.startConfigPortal();
        return; //should not be needed, controller is reset after configuring
    }

    char* state = "unknown";
    cm.mqttPublish("mydevice/status", state);
}
