# MQTT_Agent para ESP32: Comunicação de Agentes com Arduino

O **MQTT_Agent para ESP32** é uma biblioteca C++ de alto nível para a plataforma Arduino/ESP32. Inspirada na sua contraparte em Python, esta biblioteca foi concebida para simplificar a comunicação em projetos de Internet das Coisas (IoT), proporcionando uma arquitetura de mensagens flexível e um mecanismo de descoberta automática de dispositivos.

Com uma arquitetura robusta baseada em JSON e na troca de comandos entre agentes, a biblioteca permite que os dispositivos ESP32 comuniquem de forma simples e eficaz com outros agentes, incluindo os baseados na versão Python. Ideal para domótica, o **MQTT_Agent** garante que os dispositivos encontram e interagem dinamicamente, sem a necessidade de configurações manuais complicadas.

---

## Como Funciona

A biblioteca **MQTT_Agent** para ESP32 baseia-se no mesmo protocolo de comunicação robusto, que assenta em três pilares principais:

### 1. Mensagens Estruturadas (Payload JSON)

Em vez de enviar dados brutos, a biblioteca padroniza cada mensagem numa estrutura **JSON** consistente. Esta estrutura inclui os mesmos campos essenciais para uma comunicação robusta:

-   `sender_id`: O identificador único do dispositivo que enviou a mensagem.
-   `command`: A intenção da mensagem (por exemplo, `"ligar_luz"`, `"ler_sensor"`).
-   `message`: O payload real da mensagem, que pode ser uma string simples ou um JSON aninhado.

A biblioteca utiliza `ArduinoJson` internamente para lidar com a serialização e desserialização dessas mensagens.

### 2. Descoberta Automática de Dispositivos (Ping-Pong)

A biblioteca elimina a necessidade de configurar manualmente a lista de dispositivos. O processo de descoberta é idêntico ao da versão Python:

1.  Um agente envia periodicamente um comando **`"ping"`** para o tópico de transmissão (`devices/all/data`).
2.  Todos os outros dispositivos que estão a ouvir recebem o `ping`.
3.  Quando um dispositivo recebe um `ping`, ele responde com um **`"pong"`** para o `sender_id` do dispositivo que o enviou.

### 3. Abstração e Simplicidade

A complexidade da comunicação MQTT, da gestão de rede (WiFi) e da serialização JSON é completamente abstraída. Como utilizador, a interação com a biblioteca é feita através de uma API simples e intuitiva.

A biblioteca cuida de todos os detalhes:

-   **Conectividade**: Gere a conexão Wi-Fi e a reconexão automática ao broker MQTT.
-   **Na Publicação**: Recebe o `command` e a `message`, e constrói o payload JSON completo antes de publicar.
-   **Na Receção**: Analisa o JSON recebido, extrai o `command` e a `message`, e encaminha-os para as funções de "callback" (registadas com `Agent.registerCommand`).

Isso permite que haja foco na lógica da aplicação em C++ para controlar o hardware e responder aos comandos.
