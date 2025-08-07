#include "MQTT_Agent.hpp"

static MQTT_Agent *instance = nullptr;

MQTT_Agent::MQTT_Agent(): mqttClient(wifiClient)
{
    instance = this;
}

void MQTT_Agent::config(const char *ssid, const char *password, const char *mqttServer, const char *mqttUsername, const char *mqttPassword, const char *deviceId, int port, int pingPeriod)
{
    this->knownDevices.clear();

    this->ssid = ssid;
    this->password = password;
    this->mqttServer = mqttServer;
    this->mqttUsername = mqttUsername;
    this->mqttPassword = mqttPassword;
    this->deviceId = deviceId;
    this->mqttPort = port;
    this->pingPeriod = pingPeriod;
}

bool MQTT_Agent::begin(bool enablePing)
{
    stop();
    if (WiFi.status() == WL_CONNECTED)
    {
        WiFi.disconnect(true);
        delay(100);
    }
    if (mqttClient.connected())
    {
        mqttClient.disconnect();
    }

    WiFi.begin(ssid, password);
    Serial.print("\n[INFO] A ligar a ");
    Serial.println(ssid);

    unsigned long startTime = millis();
    const unsigned long wifiTimeout = 10000; // 10 segundos

    while (WiFi.status() != WL_CONNECTED)
    {
        if (millis() - startTime > wifiTimeout)
        {
            Serial.println("\n[ERRO] Timeout na ligação Wi-Fi.");
            return false;
        }
        delay(500);
        Serial.print(".");
    }
    Serial.println("\n[INFO] WiFi ligado.");

    mqttClient.setServer(mqttServer, mqttPort);
    mqttClient.setCallback(MQTT_Agent::internalCallback);

    startTime = millis();
    const unsigned long mqttTimeout = 10000; // 10 segundos

    while (!mqttClient.connected())
    {
        if (millis() - startTime > mqttTimeout)
        {
            Serial.println("[ERRO] Timeout na ligação MQTT.");
            return false;
        }

        Serial.println("[MQTT] A ligar...");
        if (mqttClient.connect(deviceId, mqttUsername, mqttPassword))
        {
            Serial.println("[MQTT] Ligado com sucesso.");
            for (const auto &topic : subscribedTopics)
            {
                if (!topic.isEmpty())
                {
                    mqttClient.subscribe(topic.c_str());
                    Serial.printf("[MQTT] Subscrito a: %s\n", topic.c_str());
                }
            }
            break;
        }
        else
        {
            Serial.print("[MQTT] Falhou, rc=");
            Serial.println(mqttClient.state());
            delay(1000);
        }
    }

    if (enablePing)
    {
        registerCommand("ping", [this](String from, String topic, JsonDocument &doc)
                        { this->_handle_ping_command(from, topic, doc); });
    }
    else
    {
        removeCommand("ping");
    }

    // Subscrição garantida ao tópico de broadcast
    bool alreadySubscribed = false;
    for (const auto &topic : subscribedTopics)
    {
        if (topic == "devices/all/data")
        {
            alreadySubscribed = true;
            break;
        }
    }

    if (!alreadySubscribed)
    {
        addSubscriptionTopic("devices/all/data");
        mqttClient.subscribe("devices/all/data");
        Serial.println("[MQTT] Subscrito a: devices/all/data");
    }

    return true;
}

void MQTT_Agent::loop()
{
    mqttClient.loop();
    if (millis() - lastPingTime >= pingPeriod)
    {
        _send_ping();
        lastPingTime = millis();
    }
}

void MQTT_Agent::stop() {
    if (mqttClient.connected()) {
        mqttClient.disconnect();
    }
    WiFi.disconnect(true);  // true para desligar também o STA mode
    Serial.println("[MQTT] Ligação terminada.");
}


void MQTT_Agent::addSubscriptionTopic(String topic)
{
    if (subscribedTopics.size() < MAX_TOPICS)
    {
        subscribedTopics.push_back(topic);
    }
    else
    {
        Serial.println("[WARN] Máximo de tópicos subscritos atingido");
    }
}

void MQTT_Agent::publish(const String &topic, const String &message)
{
    mqttClient.publish(topic.c_str(), message.c_str());
}

void MQTT_Agent::publishToDevice(const String &devId, const String &message)
{
    String topic = "devices/" + devId + "/cmd";
    publish(topic, message);
}

void MQTT_Agent::publishToDeviceFormatted(const String &devId, const String &command, const String &message)
{
    JsonDocument doc;
    doc["sender_id"] = deviceId;
    doc["command"] = command;
    doc["timestamp"] = millis();
    doc["message"] = message;

    String payload;
    serializeJson(doc, payload);
    publishToDevice(devId, payload);
}

void MQTT_Agent::setOnMessageCallback(std::function<void(String, String, String)> callback)
{
    onMessageCallback = callback;
}

void MQTT_Agent::registerCommand(const String &name, std::function<void(String, String, JsonDocument &)> handler)
{
    if (commandHandlers.size() < MAX_COMMANDS)
    {
        commandHandlers.push_back({name, handler});
    }
    else
    {
        Serial.println("[WARN] Máximo de comandos registados atingido");
    }
}

void MQTT_Agent::removeCommand(String name)
{
    for (int i = 0; i < commandHandlers.size(); ++i)
    {
        if (commandHandlers[i].name == name)
        {
            commandHandlers.erase(commandHandlers.begin() + i);
            return;
        }
    }
}

std::vector<String> MQTT_Agent::getCommands()
{
    std::vector<String> c;
    c.reserve(commandHandlers.size());

    for (size_t i = 0; i < commandHandlers.size(); ++i)
        c.emplace_back(commandHandlers[i].name);

    return c;
}

const std::vector<String> &MQTT_Agent::getKnownDevices() const
{
    return knownDevices;
}

void MQTT_Agent::internalCallback(char *topic, byte *payload, unsigned int length)
{
    if (instance)
    {
        String t = String(topic);
        String p;
        for (unsigned int i = 0; i < length; ++i)
        {
            p += (char)payload[i];
        }
        instance->handleMessage(t, p);
    }
}

void MQTT_Agent::addKnownDevice(const String &deviceId)
{
    if (strcmp(deviceId.c_str(), this->deviceId) == 0)
    {
        return;
    }

    for (int i = 0; i < knownDevices.size(); ++i)
    {
        if (knownDevices[i] == deviceId)
            return;
    }

    if (knownDevices.size() < MAX_KNOWN_DEVICES)
    {
        knownDevices.push_back(deviceId);
        Serial.println("[NOVO DISPOSITIVO] " + deviceId);
    }
    else
    {
        Serial.println("[WARN] Máximo de dispositivos conhecidos atingido");
    }
}

void MQTT_Agent::handleMessage(String topic, String payload)
{
    int start = topic.indexOf('/') + 1;
    int end = topic.indexOf('/', start);
    if (start == 0 || end == -1)
    {
        Serial.println("[AVISO] Tópico malformado: " + topic);
        return;
    }
    String devId = topic.substring(start, end);

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error)
    {
        if (onMessageCallback)
        {
            onMessageCallback(devId, topic, payload);
        }
        else
        {
            Serial.printf("[%s] %s (Dados brutos)\n", devId.c_str(), payload.c_str());
        }
        return;
    }

    String senderId = doc["sender_id"] | devId;
    String commandName = doc["command"] | "";

    addKnownDevice(senderId);

    if (commandName.length() > 0)
    {
        for (int i = 0; i < commandHandlers.size(); ++i)
        {
            if (commandHandlers[i].name == commandName)
            {
                commandHandlers[i].handler(senderId, topic, doc);
                return;
            }
        }
        Serial.println("[COMANDO DESCONHECIDO] " + commandName);
    }
    else
    {
        if (onMessageCallback)
        {
            onMessageCallback(senderId, topic, payload);
        }
        else
        {
            Serial.printf("[%s] %s\n", senderId.c_str(), payload.c_str());
        }
    }
}

void MQTT_Agent::_handle_ping_command(String from, String topic, JsonDocument &doc)
{
    if (from != String(deviceId))
    {
        Serial.printf("[PONG] Respondido pong para %s\n", from.c_str());
        publishToDeviceFormatted(from, "pong", "Pong! Estou online.");
    }
}

void MQTT_Agent::_send_ping()
{
    Serial.println("[PING] Enviado ping broadcast");
    JsonDocument doc;
    doc["command"] = "ping";
    doc["sender_id"] = deviceId;
    doc["timestamp"] = millis();

    String payload;
    serializeJson(doc, payload);
    publish("devices/all/data", payload);
}

void default_PongResponse(String from, String topic, JsonDocument &doc)
{
    String message = doc["message"] | "sem mensagem";
    Serial.printf("[CMD] 'pong' recebido de %s: %s\n", from.c_str(), message.c_str());
}


MQTT_Agent Agent;
