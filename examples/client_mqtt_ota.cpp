#include <Arduino.h>
#include "ConMan.h"

ConMan cm("MyDevice");

void mqttCallback(char* topic, char* payload) {
    if (strcmp(topic, "MyDevice/ota") == 0) {
        cm.restartIntoOtaMode();
        return;
    }
}

void loop() {
    cm.loop();
}

void setup() {
    if (!cm.wifiConnectAsClient() || !cm.setupMqtt()) {
        cm.wifiStartConfig();
        return; //should not be needed, controller is reset after configuring
    }
    cm.checkStartOtaMode();

    cm.setupMqtt(mqttCallback);
    cm.mqttSubscribe("MyDevice/#");

    cm.mqttPublish("MyDevice/status", "unknown");
}
