#include "MQTT_Agent.hpp"

//Dependencias
//#include <WiFi.h>
//#include <PubSubClient.h> by Nich O'Leary
//#include <ArduinoJson.h> by Benoit



// --- Configurações de Conexão ---
// Defina suas credenciais de Wi-Fi.
const char* ssid = "O_TEU_SSID";
const char* password = "A_TUA_PALAVRA_PASSE";
// Defina os detalhes do servidor MQTT.
const char* mqttServer = "IP_DO_PC"; // O IP do seu computador ou broker MQTT.
const char* mqttUsername = "USER_NAME"; // Nome de usuário para o broker MQTT.
const char* mqttPassword = "PASSWORD"; // Senha para o broker MQTT.
const char* deviceId = "esp32_quarto"; // ID único para o seu dispositivo na rede.


// --- Inicialização do Agente MQTT ---
// Cria uma instância do agente MQTT com as credenciais definidas acima.
// Verifica as variaveis no MQTT_Agent.hpp para mais informações, nomeadamente para poder definir ports e periodos de ping
MQTT_Agent Agent(ssid, password, mqttServer, mqttUsername, mqttPassword, deviceId);

// --- Callback para Respostas Padrão ---
// Esta função é chamada sempre que uma mensagem é recebida no tópico de resposta.
// "from" é o ID do dispositivo que enviou a mensagem.
// "topic" é o tópico da mensagem.
// "payload" é o conteúdo da mensagem.
void onMessage(String from, String topic, String payload)
{
  // Imprime a resposta recebida, caso queiras processar a informação sem ser por jsons prefeitos por agents (python ou outras esp) implementa o teu codigo aqui.
  Serial.printf("[RESPOSTA] de %s: %s\n", from.c_str(), payload.c_str());
}


// --- Funções de Comando ---

// Função que é executada quando o comando "ligar_luz" é recebido.
void ligar_luz(String from, String topic, JsonDocument& doc)
{
  // Extrai o ID do remetente e a mensagem do documento JSON.
  String sender_id = doc["sender_id"] | "unknown";
  String message = doc["message"] | "sem mensagem";

  // Imprime no monitor serial que o comando foi recebido.
  Serial.print("[Agente 2: ");
  Serial.print(Agent.getDeviceId());
  Serial.print("] -> RECEBIDO comando 'ligar_luz' de '");
  Serial.print(sender_id);
  Serial.println("'.");

  // Imprime a mensagem recebida.
  Serial.print("[Agente 2: ");
  Serial.print(Agent.getDeviceId());
  Serial.print("] -> Mensagem: ");
  Serial.println(message);
  
  // Aqui adicionarias o código para ligar a luz (ex: digitalWrite) dependendo dos conteudos de "message"
}

// Função que é executada quando o comando "ler_temperatura" é recebido.
void ler_temperatura(String from, String topic, JsonDocument& doc)
{
  // Extrai o ID do remetente e a mensagem do documento JSON.
  String sender_id = doc["sender_id"] | "unknown"; 
  String message = doc["message"] | "sem mensagem"; // A mensagem pode ser ignorada se não for necessária.

  // Imprime no monitor serial que o comando foi recebido.
  Serial.print("[Agente 2: ");
  Serial.print(Agent.getDeviceId());
  Serial.print("] -> RECEBIDO comando 'ler_temperatura' de '");
  Serial.print(sender_id);
  Serial.println("'.");

  // Imprime a intenção de enviar a temperatura de volta.
  Serial.print("[Agente 2: ");
  Serial.print(Agent.getDeviceId());
  Serial.print("] -> A enviar a temperatura de volta para '");
  Serial.print(sender_id);
  Serial.println("'...");

  // Cria um documento JSON para a resposta.
  JsonDocument resposta;
  String resposta_str;
  
  // Adiciona valores de temperatura e unidade ao JSON.
  resposta["valor"] = 23.5; // Valor de temperatura simulado.
  resposta["unidade"] = "C";
  
  // Serializa o JSON para uma string.
  serializeJson(resposta, resposta_str);

  // Envia a resposta de volta ao remetente usando a função do agente.
  // O comando "temperatura_report" é o nome da ação que estamos a reportar.
  Agent.publishToDeviceFormatted(sender_id, "temperatura_report", resposta_str);
}




// --- Função de Configuração (setup) ---
void setup() 
{
  Serial.begin(115200); // Inicia a comunicação serial com um baud rate de 115200.
  while(!Serial){}; // Espera a porta serial estar pronta.
    
  // Adiciona os tópicos que o agente irá subscrever.
  // 1. Tópico para comandos gerais ("devices/all/data").
  Agent.addSubscriptionTopic("devices/all/data");
  // 2. Tópico específico para comandos enviados diretamente para este dispositivo.
  String ownTopic = String("devices/") + deviceId + "/cmd";
  Agent.addSubscriptionTopic(ownTopic.c_str());

  // Registra os comandos que o agente pode processar.
  // default_PongResponse é uma função padrão da biblioteca para responder a pings.
  Agent.registerCommand("pong", default_PongResponse);
  Agent.registerCommand("ligar_luz", ligar_luz);
  Agent.registerCommand("ler_temperatura", ler_temperatura);

  // Define a função de callback para processar mensagens de resposta.
  Agent.setOnMessageCallback(onMessage);
    
  // Inicia o agente, conectando-se ao Wi-Fi e ao broker MQTT.
  // O parâmetro `true` habilita a funcionalidade de ping (default value: true).
  Agent.begin(true);
}




// --- Função de Loop Principal (loop) ---
void loop() 
{
  // A chamada Agent.loop() é crucial para o funcionamento do agente.
  // Ela mantém a conexão MQTT ativa, processa mensagens recebidas
  // e executa a lógica de tempo da biblioteca.
  Agent.loop();
}
