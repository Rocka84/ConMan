#include <Arduino.h>
#include "ConMan.h"

ConMan cm("MyDevice");

void loop() {
    cm.loop();
}

void setup() {
    cm.wifiStartAP();
    cm.setupServer();
    cm.serverOn("/", []() {
        cm.getServer()->send(200, "text/html", "<b>Hello World</b>");
    });
}
