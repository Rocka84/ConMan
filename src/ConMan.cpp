#include "ConMan.h"

// #define CONMANDBG

#ifdef CONMANDBG
#define CM_DEBUG Serial.print
#define CM_DEBUGLN Serial.println
#else
#define CM_DEBUG(...) /**/
#define CM_DEBUGLN(...) /**/
#endif

#define CM_RTC_START    13

void(* conManReset) (void) = 0;

ConMan::ConMan(const char *_device_name) {
    init(_device_name, "");
}

ConMan::ConMan(const char *_device_name, const char *_ap_password) {
    init(_device_name, _ap_password);
}


// -----  private  -----

void ConMan::init(const char *_device_name, const char *_ap_password) {
    device_name = _device_name;
    ap_password = _ap_password;
    server.reset(new ESP8266WebServer(80));
    mqtt.reset(new PubSubClient(espClient));
#ifdef CONMANDBG
    wm.setDebugOutput(true);
#else
    wm.setDebugOutput(false);
#endif

    // checkStartConfig();
}

bool ConMan::readEEPROM() {
    unsigned int pos=0;
    char cur_char;

    EEPROM.begin(512);

    if (EEPROM.read(pos) != CM_EEPROM_CHECK) {
        CM_DEBUGLN("eeprom invalid");
        return false;
    }
    pos++;

    cur_char = '.';
    while(cur_char != ';' && pos < 255) {
        cur_char = char(EEPROM.read(pos));
        if (cur_char != ';') {
            mqtt_host_str += String(cur_char);
        }
        pos++;
    }
    mqtt_host_str.trim();
    mqtt_host = mqtt_host_str.c_str();

    cur_char = '.';
    while(cur_char != ';' && pos < 255) {
        cur_char = char(EEPROM.read(pos));
        if (cur_char != ';') {
            mqtt_user_str += String(cur_char);
        }
        pos++;
    }
    mqtt_user_str.trim();
    mqtt_user = mqtt_user_str.c_str();

    cur_char = '.';
    while(cur_char != ';' && pos < 255) {
        cur_char = char(EEPROM.read(pos));
        if (cur_char != ';') {
            mqtt_password_str += String(cur_char);
        }
        pos++;
    }
    mqtt_password_str.trim();
    mqtt_password = mqtt_password_str.c_str();

    EEPROM.end();

    CM_DEBUGLN("readEEPROM");
    CM_DEBUG("mqtt_host ");
    CM_DEBUGLN(mqtt_host);
    CM_DEBUG("mqtt_user ");
    CM_DEBUGLN(mqtt_user);
    CM_DEBUG("mqtt_password '");
    CM_DEBUG(mqtt_password);
    CM_DEBUGLN("'");
    return true;
}

void ConMan::writeEEPROM() {
    CM_DEBUGLN("writeEEPROM");
    String string_val;
    unsigned int pos=0;

    EEPROM.begin(512);

    EEPROM.write(pos, CM_EEPROM_CHECK);
    pos++;

    for (int step = 0; step < 3; step++) {
        switch(step) {
            case 0: string_val = String(mqtt_host); break;
            case 1: string_val = String(mqtt_user); break;
            case 2: string_val = String(mqtt_password); break;
        }
        for(unsigned int n=0; n < string_val.length();n++){
            EEPROM.write(pos, string_val[n]);
            pos++;
        }
        EEPROM.write(pos, ';');
        pos++;
    }

    EEPROM.commit();
    EEPROM.end();
}

void ConMan::loopMqtt() {
    if (!mqttConnect()) {
        conManReset();
        return;
    }
    mqtt->loop();

    if (millis() - heartbeat_time >= 120000) {
        heartbeat_time = millis();
        mqttPublish(topic_availability, "online");
    }
}

void ConMan::saveWifiManager() {
    mqtt_host = wm_param_mqtt_host->getValue();
    mqtt_user = wm_param_mqtt_user->getValue();
    mqtt_password = wm_param_mqtt_password->getValue();
    writeEEPROM();
}

void ConMan::callbackWrapper(char* topic, uint8_t* payload, unsigned int length) {
    char payloadChars[length+1];
    for (unsigned int i = 0; i < length; i++) {
        if (payload[i]>=20 && payload[i]<=126) {
            payloadChars[i] = (char) payload[i];
        }
    }
    payloadChars[length] = '\0';

    mqtt_callback(topic, payloadChars);
}

void ConMan::reset() {
    delay(1000);
    conManReset();
    for(;;) delay(1000);
}


// -----  public  -----

void ConMan::wifiAutoSetup() {
    if (wifiConnectAsClient())   {
        return;
    }

    wifiStartAP();
}

bool ConMan::wifiConnectAsClient() {
    CM_DEBUGLN("wifiConnectAsClient");
    WiFi.mode(WIFI_STA);
    if (wm.connectWifi("", "") == WL_CONNECTED) {
        CM_DEBUG("Client mode, IP address: ");
        CM_DEBUGLN(WiFi.localIP());
        wifi_setup_done = true;
        return true;
    }
    return false;
}

void ConMan::wifiStartAP() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(device_name, ap_password);
    wifi_setup_done = true;
    ap_mode = true;
    CM_DEBUG("AP mode, IP address: ");
    CM_DEBUGLN(WiFi.softAPIP());
}

bool ConMan::isWifiSetupDone() {
    return wifi_setup_done;
}

bool ConMan::isApMode() {
    return ap_mode;
}

IPAddress ConMan::getIP() {
    if (ap_mode) {
        return WiFi.softAPIP();
    }
    return WiFi.localIP();
}

void ConMan::wifiStartConfig() {
    CM_DEBUGLN("startConfigPortal");
    readEEPROM();
    WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
    wm_param_mqtt_host.reset(new WiFiManagerParameter ("mqtt_host", "MQTT Host", mqtt_host, 50));
    wm_param_mqtt_user.reset(new WiFiManagerParameter ("mqtt_user", "MQTT User", mqtt_user, 40));
    wm_param_mqtt_password.reset(new WiFiManagerParameter ("mqtt_password", "MQTT Password", mqtt_password, 100, "type=\"password\""));
    wm.addParameter(wm_param_mqtt_host.get());
    wm.addParameter(wm_param_mqtt_user.get());
    wm.addParameter(wm_param_mqtt_password.get());

    wm.startConfigPortal(device_name);
    saveWifiManager();
}

void ConMan::checkStartConfig() {
    CM_DEBUGLN("ConMan::checkStartConfig");
    int rtcMem;
    system_rtc_mem_read(CM_RTC_START, &rtcMem, sizeof(rtcMem));
    CM_DEBUG("rtcMem ");
    CM_DEBUGLN(rtcMem);
    if (rtcMem != CM_RTC_START) return;
    rtcMem = 0;
    system_rtc_mem_write(CM_RTC_START, &rtcMem, sizeof(rtcMem));

    wifiAutoSetup();
    wifiStartConfig();

    conManReset();
    for(;;) delay(1000);
}

void ConMan::restartIntoConfig() {
    int rtcMem = CM_RTC_START;
    system_rtc_mem_write(CM_RTC_START, &rtcMem, sizeof(rtcMem));
    reset();
}



void ConMan::setupServer(bool enable_default_routes, bool enable_ota) {
    if (enable_ota) ElegantOTA.begin(server.get());

    server->begin();
    server_setup_done = true;

    if (!enable_default_routes) return;
    server->on("/start_config", [=]() {
        server->send(200, "application/json", "{\"start_config\": true, \"reset\": true}");
        restartIntoConfig();
    });
    server->on("/reset_config", [=]() {
        server->send(200, "application/json", "{\"reset_config\": true, \"reset\": true}");
        resetSettings();
        reset();
    });
    server->on("/reset", [=]() {
        server->send(200, "application/json", "{\"reset\": true}");
        reset();
    });
}

void ConMan::setupServer(bool enable_default_routes) {
    setupServer(enable_default_routes, true);
}

void ConMan::setupServer() {
    setupServer(true, true);
}

bool ConMan::isServerSetupDone() {
    return server_setup_done;
}

void ConMan::serverOn(const String &uri, THandlerFunction handler) {
    server->on(uri, handler);
}

void ConMan::serverOn(const String &uri, HTTPMethod method, THandlerFunction fn) {
    server->on(uri, method, fn);
}

void ConMan::serverOn(const String &uri, HTTPMethod method, THandlerFunction fn, THandlerFunction ufn) {
    server->on(uri, method, fn, ufn);
}

ESP8266WebServer* ConMan::getServer() {
    return server.get();
}



bool ConMan::setupMqtt(CM_CALLBACK_SIGNATURE) {
    (String(device_name) + "/status/availability").toCharArray(topic_availability, 60);
    String(device_name).toCharArray(mqtt_client_id, 60);
    if (!readEEPROM() || !mqttConnect()) {
        CM_DEBUGLN("mqtt connection failed");
        return false;
    }

    mqtt_setup_done = true;
    if (mqtt_callback != NULL) setMqttCallback(mqtt_callback);
    mqttPublish(topic_availability, "online");

    return true;
}

bool ConMan::setupMqtt() {
    return setupMqtt(NULL);
}

bool ConMan::isMqttSetupDone() {
    return mqtt_setup_done;
}

bool ConMan::setMqttCallback(CM_CALLBACK_SIGNATURE) {
    if (!mqtt_setup_done) return false;

    this->mqtt_callback = mqtt_callback;
    mqtt->setCallback([=](char* topic, uint8_t* payload, unsigned int length) {
        this->callbackWrapper(topic, payload, length);
    });
    return true;
}

bool ConMan::setMqttCallback(MQTT_CALLBACK_SIGNATURE) {
    if (!mqtt_setup_done) return false;

    this->mqtt_callback = NULL;
    mqtt->setCallback(callback);
    return true;
}

bool ConMan::mqttConnect() {
    // CM_DEBUGLN("connecting mqtt");
    if (mqtt->connected()) return true;

    mqtt->setServer(mqtt_host, 1883);
    return mqtt->connect(mqtt_client_id, mqtt_user, mqtt_password, topic_availability, 1, true, "offline");
}

bool ConMan::mqttPublish(const char* topic, const char* payload, bool retain) {
    if (!mqtt_setup_done) return false;
    if (!mqttConnect()) return false;

    mqtt->publish(topic, payload, retain);
    return true;
}

bool ConMan::mqttPublish(const char* topic, const char* payload) {
    return mqttPublish(topic, payload, false);
}

bool ConMan::mqttSubscribe(const char* topic) {
    return mqtt->subscribe(topic, 0);
}

bool ConMan::mqttSubscribe(const char* topic, uint8_t qos) {
    return mqtt->subscribe(topic, qos);
}

bool ConMan::payloadToBool(char* payload) {
    if (strcmp("1", payload) == 0 || strcmp("on", payload) == 0 || strcmp("ON", payload) == 0) {
        return true;
    }
    return false;
}

bool ConMan::payloadToBool(char* payload, bool before) {
    if (strcmp("0", payload) == 0 || strcmp("off", payload) == 0 || strcmp("OFF", payload) == 0) {
        return false;
    }
    if (payloadToBool(payload)) {
        return true;
    }
    if (strcmp("2", payload) == 0 || strcmp("toggle", payload) == 0 || strcmp("TOGGLE", payload) == 0) {
        return !before;
    }

    return before;
}

void ConMan::resetSettings() {
    wm.resetSettings();
}

void ConMan::loop() {
    if (server_setup_done) server->handleClient();
    if (mqtt_setup_done) loopMqtt();
}

