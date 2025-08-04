#ifndef MQTT_AGENT_HPP
#define MQTT_AGENT_HPP

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <functional>

// Constantes
#ifndef MAX_KNOWN_DEVICES
#define MAX_KNOWN_DEVICES 10
#endif

#ifndef MAX_TOPICS
#define MAX_TOPICS 5
#endif

#ifndef MAX_COMMANDS
#define MAX_COMMANDS 10
#endif

// Estrutura para os manipuladores de comandos
struct CommandHandler {
    String name;
    std::function<void(String, String, JsonDocument&)> handler;
};

class MQTT_Agent {
private:
    const char* ssid;
    const char* password;
    const char* mqttServer;
    const char* mqttUsername;  // Variável para o nome de utilizador
    const char* mqttPassword;  // Variável para a palavra-passe
    const char* deviceId;
    int mqttPort;

    WiFiClient wifiClient;
    PubSubClient mqttClient;

    const char* subscribedTopics[MAX_TOPICS];
    int subscribedCount = 0;

    CommandHandler commandHandlers[MAX_COMMANDS];
    int commandCount = 0;

    String knownDevices[MAX_KNOWN_DEVICES];
    int knownCount = 0;

    std::function<void(String, String, String)> onMessageCallback;

    static void internalCallback(char* topic, byte* payload, unsigned int length);
    void handleMessage(String topic, String payload);
    void addKnownDevice(const String& deviceId);

public:
    // Construtor atualizado para aceitar o username e password
    MQTT_Agent(const char* ssid, const char* password, const char* mqttServer, const char* mqttUsername, const char* mqttPassword, const char* deviceId, int port = 1883);

    void begin();
    void loop();
    void addSubscriptionTopic(const char* topic);
    void publish(const String& topic, const String& message);
    void publishToDevice(const String& devId, const String& message);
    void setOnMessageCallback(std::function<void(String, String, String)> callback);
    void registerCommand(const String& name, std::function<void(String, String, JsonDocument&)> handler);
    const char* getDeviceId() { return deviceId; }

    JsonDocument createStandardJson(const String& comand, const String& message);
};

#endif