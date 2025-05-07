// main.cpp (ESP32 PlatformIO)
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>
#include <PubSubClient.h>
#include <SPIFFS.h>
// Serial is part of the ESP32 core, no separate include needed in Arduino/PlatformIO

// --- WiFi and MQTT Configuration ---
const char* ssid = "Lahsiv"; // <-- CHANGE THIS
const char* password = "DeviO007"; // <-- CHANGE THIS
// IMPORTANT: Change this to the actual IP address of your MQTT broker.
// This should be the local network IP address of the machine running your Mosquitto broker.
const char* mqtt_server = "192.168.71.132"; // <-- Use your PC's IP
//const char* mqtt_server = "127.0.0.1"; // <-- CHANGE THIS (e.g., "192.168.1.100")
const int mqtt_port = 1883;
const char* mqtt_client_id = "ESP32ClientPublisherSubscriber"; // Unique client ID

// --- Global Objects ---
WiFiClient espClient;
PubSubClient client(espClient); // MQTT client instance
AsyncWebServer server(80); // Web server instance on port 80
WebSocketsServer webSocket = WebSocketsServer(81); // WebSocket server instance on port 81

// --- Global Variables to Hold Traffic Light States ---
String light1 = "red"; // Default state
String light2 = "green"; // Default state

// --- Function to Send Traffic Light States via WebSocket ---
void notifyClients() {
  // Format the states as a comma-separated string
  String msg = light1 + "," + light2;
  // Serial.print("Sending WebSocket message: "); // Optional: Debug print
  // Serial.println(msg); // Optional: Debug print

  // Broadcast the message to all connected WebSocket clients
  webSocket.broadcastTXT(msg);
}

// --- MQTT Message Callback Function ---
// This function is called whenever a message is received on a subscribed topic.
void callback(char* topic, byte* payload, unsigned int length) {
  String topicStr = String(topic);
  String msg = "";
  // Convert the payload bytes to a String
  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  // Serial.print("Message arrived ["); // Optional: Debug print
  // Serial.print(topicStr); // Optional: Debug print
  // Serial.print("] "); // Optional: Debug print
  // Serial.println(msg); // Optional: Debug print

  // Update the global state variables based on the topic
  if (topicStr == "traffic/light1") {
    light1 = msg;
  } else if (topicStr == "traffic/light2") {
    light2 = msg;
  }

  // Notify connected web clients via WebSocket about the state change
  notifyClients();
}

// --- Function to Handle MQTT Reconnection ---
void reconnect() {
  // Loop until we're reconnected to the MQTT broker
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection..."); // Optional: Status
    // Attempt to connect using the defined client ID
    if (client.connect(mqtt_client_id)) {
      Serial.println("connected!"); // Optional: Status

      // Subscribe to the traffic light topics
      client.subscribe("traffic/light1");
      client.subscribe("traffic/light2");
      Serial.println("Subscribed to traffic topics."); // Optional: Status

      // Send the current state to clients immediately upon (re)connection
      notifyClients();

    } else {
      Serial.print("failed, rc="); // Optional: Status (rc = return code)
      Serial.print(client.state()); // Optional: Status (client.state() provides more detail)
      Serial.println(" retrying in 5 seconds..."); // Optional: Status
      // Wait 5 seconds before retrying the connection
      delay(5000);
    }
  }
}

// --- WebSocket Event Handler ---
// This function is called when a WebSocket client connects, disconnects,
// sends a message, etc. For this simulation, we only need to broadcast
// FROM the ESP32 TO the clients, so the handler can be simple.
void onWebSocketEvent(uint8_t client_num, WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      Serial.printf("[%u] WebSocket client connected\n", client_num);
      // When a new client connects, send them the current state immediately
      notifyClients();
      break;
    case WStype_DISCONNECTED:
      Serial.printf("[%u] WebSocket client disconnected\n", client_num);
      break;
    case WStype_TEXT:
      // Handle text messages from clients if needed (e.g., to control something)
      // Serial.printf("[%u] WebSocket received text: %s\n", client_num, payload);
      // Example: If a client sends "getState", respond with current state
      // if (length == 8 && strncmp((char*)payload, "getState", 8) == 0) {
      //   notifyClients();
      // }
      break;
    case WStype_BIN: // Handle binary messages if needed
    case WStype_ERROR: // Handle errors
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
      break;
  }
}


// --- Setup Function (Runs once on boot) ---
void setup() {
  // Start serial communication for debugging output
  Serial.begin(115200);
  while (!Serial); // Wait for serial port to connect (useful for some boards)

  Serial.println("\nESP32 Traffic Signal Web UI");
  Serial.println("---------------------------");

  // --- Connect to WiFi ---
  Serial.print("Connecting to WiFi network: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000); // Wait 1 second between attempts
    Serial.print(".");
  }
  Serial.println(""); // Newline after connection dots
  Serial.println("WiFi connected successfully!");
  Serial.print("ESP32 Local IP Address: ");
  Serial.println(WiFi.localIP());    // <<< THIS LINE PRINTS THE IP ADDRESS >>>

  // --- Initialize SPIFFS ---
  Serial.println("Initializing SPIFFS...");
  if (!SPIFFS.begin(true)) { // Start SPIFFS, format if needed (true)
    Serial.println("SPIFFS Mount Failed! Make sure you have uploaded files.");
    // Consider putting the ESP32 into a safe mode or indicating the error visually
    // while(true); // Optional: halt execution if SPIFFS fails
    return; // Stop setup if SPIFFS fails
  }
  Serial.println("SPIFFS mounted successfully.");

  // --- Setup MQTT Client ---
  client.setServer(mqtt_server, mqtt_port); // Set the MQTT broker IP and port
  client.setCallback(callback); // Set the function to call when messages arrive
  Serial.printf("MQTT client configured for server %s:%d\n", mqtt_server, mqtt_port);

  // --- Setup WebSocket Server ---
  webSocket.begin(); // Start the WebSocket server on port 81
  webSocket.onEvent(onWebSocketEvent); // Register the event handler function
  Serial.println("WebSocket server started on port 81.");

  // --- Setup Web Server ---
  // Serve files from the SPIFFS filesystem.
  // The root path "/" will serve "index.html" by default if requested.
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

  // Add a fallback for requests that don't match any file (optional 404 handler)
  // server.onNotFound([](AsyncWebServerRequest *request){
  //   request->send(404, "text/plain", "File Not Found");
  // });

  server.begin(); // Start the HTTP web server on port 80
  Serial.println("HTTP server started on port 80.");

  // --- Initial MQTT Connection Attempt ---
  // The loop() function will handle reconnections if this initial attempt fails.
  if (!client.connected()) {
    reconnect();
  }
}

// --- Loop Function (Runs repeatedly) ---
void loop() {
  // --- MQTT Client Maintenance ---
  // If the MQTT client is not connected, attempt to reconnect.
  if (!client.connected()) {
    reconnect();
  }
  // Allow the MQTT client to process incoming/outgoing messages and maintain connection.
  client.loop();

  // --- WebSocket Server Maintenance ---
  // Handle incoming connections and process messages for WebSocket clients.
  webSocket.loop();

  // --- Other Tasks ---
  // You can add other non-blocking tasks here if needed (e.g., reading sensors).
  // Avoid using delay() in loop() for long periods if you need responsive
  // MQTT or WebSocket communication.
}