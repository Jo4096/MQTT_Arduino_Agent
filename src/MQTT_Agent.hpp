#ifndef MQTT_AGENT_HPP
#define MQTT_AGENT_HPP

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <functional>
#include <vector>

#ifndef MAX_KNOWN_DEVICES
#define MAX_KNOWN_DEVICES 10
#endif

#ifndef MAX_TOPICS
#define MAX_TOPICS 5
#endif

#ifndef MAX_COMMANDS
#define MAX_COMMANDS 10
#endif

struct CommandHandler {
    String name;
    std::function<void(String, String, JsonDocument &)> handler;
};

class MQTT_Agent {
private:
    const char *ssid;
    const char *password;
    const char *mqttServer;
    const char *mqttUsername;
    const char *mqttPassword;
    const char *deviceId;
    int mqttPort;
    long lastPingTime = 0;
    int pingPeriod;

    WiFiClient wifiClient;
    PubSubClient mqttClient;

    std::vector<String> subscribedTopics;
    std::vector<CommandHandler> commandHandlers;
    std::vector<String> knownDevices;

    std::function<void(String, String, String)> onMessageCallback;

    static void internalCallback(char *topic, byte *payload, unsigned int length);
    void handleMessage(String topic, String payload);
    void addKnownDevice(const String &deviceId);

    void _handle_ping_command(String from, String topic, JsonDocument &doc);
    void _send_ping();

public:
    MQTT_Agent();
    void config(const char *ssid, const char *password, const char *mqttServer,
                const char *mqttUsername, const char *mqttPassword,
                const char *deviceId, int port = 1883, int pingPeriod = 30000);

    bool begin(bool enablePing = true);
    void loop();
    void stop();

    void addSubscriptionTopic(String topic);
    void publish(const String &topic, const String &message);
    void publishToDevice(const String &devId, const String &message);
    void publishToDeviceFormatted(const String &devId, const String &command, const String &message);

    void setOnMessageCallback(std::function<void(String, String, String)> callback);
    void registerCommand(const String &name, std::function<void(String, String, JsonDocument &)> handler);
    void removeCommand(String name);

    const char *getDeviceId() { return deviceId; }
    std::vector<String> getCommands();
    const std::vector<String> &getKnownDevices() const;
};

void default_PongResponse(String from, String topic, JsonDocument &doc);

extern MQTT_Agent Agent;

#endif
