#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <ElegantOTA.h>
#include <EEPROM.h>

#ifndef ConMan_h
#define ConMan_h


#define EEPROM_CHECK 41

#if defined(ESP8266) || defined(ESP32)
#include <functional>
#define CM_CALLBACK_SIGNATURE std::function<void(char*, char*)> mqtt_callback
#else
#define CM_CALLBACK_SIGNATURE void (*mqtt_callback)(char*, char*)
#endif

typedef std::function<void(void)> THandlerFunction;

class ConMan {
    friend WiFiManager;

    private:
        std::unique_ptr<ESP8266WebServer> server;
        std::unique_ptr<PubSubClient> mqtt;
        WiFiClient espClient;

        WiFiManager wm;
        std::unique_ptr<WiFiManagerParameter> wm_param_mqtt_host;
        std::unique_ptr<WiFiManagerParameter> wm_param_mqtt_user;
        std::unique_ptr<WiFiManagerParameter> wm_param_mqtt_password;

        bool wifi_setup_done = false;
        const char* device_name;
        const char* ap_password;
        bool ap_mode = false;
        bool server_setup_done = false;

        bool mqtt_setup_done = false;
        const char* mqtt_host;
        const char* mqtt_user;
        const char* mqtt_password;
        String mqtt_host_str;
        String mqtt_user_str;
        String mqtt_password_str;
        CM_CALLBACK_SIGNATURE;

        char topic_availability[60];
        char mqtt_client_id[60];
        unsigned int heartbeat_time = 0;


        void init(const char *_device_name, const char *_ap_password);
        bool readEEPROM();
        void writeEEPROM();
        void loopMqtt();
        void saveWifiManager();

        void callbackWrapper(char*, uint8_t*, unsigned int);
        void reset();

    public:
        ConMan(const char *_device_name);
        ConMan(const char *_device_name, const char *_ap_password);

        void wifiAutoSetup();
        bool wifiConnectAsClient();
        void wifiStartAP();
        bool isWifiSetupDone();
        bool isApMode();
        void wifiStartConfig();
        void checkStartConfig();
        void triggerStartConfig();

        void setupServer(bool enable_default_routes, bool enable_ota);
        void setupServer(bool enable_default_routes);
        void setupServer();
        bool isServerSetupDone();
        void serverOn(const String &uri, THandlerFunction handler);
        void serverOn(const String &uri, HTTPMethod method, THandlerFunction fn);
        void serverOn(const String &uri, HTTPMethod method, THandlerFunction fn, THandlerFunction ufn);
        ESP8266WebServer* getServer();

        bool setupMqtt();
        bool setupMqtt(CM_CALLBACK_SIGNATURE);
        bool isMqttSetupDone();
        bool setMqttCallback(CM_CALLBACK_SIGNATURE);
        bool setMqttCallback(MQTT_CALLBACK_SIGNATURE);

        bool mqttConnect();
        bool mqttPublish(const char* topic, const char* payload, bool retain);
        bool mqttPublish(const char* topic, const char* payload);
        bool mqttSubscribe(const char* topic);
        bool mqttSubscribe(const char* topic, uint8_t qos);

        bool payloadToBool(char* payload);
        bool payloadToBool(char* payload, bool before);

        void resetSettings();

        void loop();
};

#endif
