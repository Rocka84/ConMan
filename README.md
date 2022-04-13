# ConMan

Connection Manager wrapping WiFi-Setup using WiFiManager, a WebServer, Updates using ElegantOTA and MQTT using PubSubClient all in one easy to use package.

Covers the needs for most of my esp8266 projects using wifi.

## Features

* connect as client or open an AP
    * auto mode tries connecting as client and falls back to opening an AP
* start WebServer
    * default routes for config and ota updates
    * including ota updates using ElegantOTA
* connect to MQTT broker
    * sends heartbeat to availalability topic
    * helper functions for callback function and payload conversion
* Everything is optional
    * connect as client or start ap or use auto mode
    * use webserver and MQTT or only one or none of both
    * ota updates and default routes can be suppressed


## Examples

### Full blown

Auto connect wifi and setup web server. If connected as wifi client also connect mqtt.

```cpp
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
```

### mqtt client

Connect as wifi client and setup mqtt. On failure launch config portal.

```cpp
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
```

### AP web server

Start AP and setup webserver.

```cpp
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
```

