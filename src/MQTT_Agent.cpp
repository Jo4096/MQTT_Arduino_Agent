#include "MQTT_Agent.hpp"

static MQTT_Agent* instance = nullptr;

// Construtor atualizado
MQTT_Agent::MQTT_Agent(const char* ssid, const char* password, const char* mqttServer, const char* mqttUsername, const char* mqttPassword, const char* deviceId, int port)
    : ssid(ssid), password(password), mqttServer(mqttServer), mqttUsername(mqttUsername), mqttPassword(mqttPassword), deviceId(deviceId), mqttPort(port),
      mqttClient(wifiClient), subscribedCount(0), commandCount(0), knownCount(0)
{
    instance = this;
}

void MQTT_Agent::begin()
{
    WiFi.begin(ssid, password);
    Serial.print("\n[INFO] A ligar a ");
    Serial.println(ssid);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\n[INFO] WiFi ligado.");

    mqttClient.setServer(mqttServer, mqttPort);
    mqttClient.setCallback(MQTT_Agent::internalCallback);

    while (!mqttClient.connected())
    {
        Serial.println("[MQTT] A ligar...");
        // Linha de conexão alterada para incluir o username e a password
        if (mqttClient.connect(deviceId, mqttUsername, mqttPassword))
        {
            Serial.println("[MQTT] Ligado com sucesso.");
            for (int i = 0; i < subscribedCount; ++i)
            {
                if(subscribedTopics[i] != nullptr)
                {
                    mqttClient.subscribe(subscribedTopics[i]);
                    Serial.printf("[MQTT] Subscrito a: %s\n", subscribedTopics[i]);
                }
            }
        }
        else
        {
            Serial.print("[MQTT] Falhou, rc=");
            Serial.println(mqttClient.state());
            delay(1000);
        }
    }
}

void MQTT_Agent::loop()
{
    mqttClient.loop();
}

void MQTT_Agent::addSubscriptionTopic(const char* topic)
{
    if (subscribedCount < MAX_TOPICS)
    {
        subscribedTopics[subscribedCount++] = topic;
    }
    else
    {
        Serial.println("[WARN] Máximo de tópicos subscritos atingido");
    }
}

void MQTT_Agent::publish(const String& topic, const String& message)
{
    mqttClient.publish(topic.c_str(), message.c_str());
}

void MQTT_Agent::publishToDevice(const String& devId, const String& message)
{
    String topic = "devices/" + devId + "/cmd";
    publish(topic, message);
}

void MQTT_Agent::setOnMessageCallback(std::function<void(String, String, String)> callback)
{
    onMessageCallback = callback;
}

void MQTT_Agent::registerCommand(const String& name, std::function<void(String, String, JsonDocument&)> handler)
{
    if (commandCount < MAX_COMMANDS)
    {
        commandHandlers[commandCount++] = {name, handler};
    }
    else
    {
        Serial.println("[WARN] Máximo de comandos registados atingido");
    }
}

void MQTT_Agent::internalCallback(char* topic, byte* payload, unsigned int length)
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

void MQTT_Agent::addKnownDevice(const String& deviceId)
{
    for (int i = 0; i < knownCount; ++i)
    {
        if (knownDevices[i] == deviceId) return;
    }
    if (knownCount < MAX_KNOWN_DEVICES)
    {
        knownDevices[knownCount++] = deviceId;
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
        for (int i = 0; i < commandCount; ++i)
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



JsonDocument MQTT_Agent::createStandardJson(const String& comand, const String& message)
{
    JsonDocument doc;
    doc["command"] = command;
    doc["message"] = message;
    doc["sender_id"] = getDeviceId();
    doc["timestamp"] = millis();
    return doc;
}