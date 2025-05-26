/*
 * Flow Sensor Monitoring System with WiFi and WebSocket
 * 
 * This code reads pulses from a flow sensor, calculates flow rate,
 * connects to WiFi, synchronizes time via NTP, and sends data to a WebSocket server.
 */

#include <ESP8266WiFi.h>          // ESP8266 WiFi library
#include <ArduinoWebsockets.h>    // WebSocket client library
#include <ArduinoJson.h>          // JSON handling library
#include <time.h>                 // Time functions

using namespace websockets;       // Using WebSockets namespace

// Network credentials and server configuration
const char* ssid = "<wifi_network>";               // WiFi SSID
const char* password = "<wifi_password>";          // WiFi password
const char* websockets_server_host = "<server_ip>"; // WebSocket server IP
const int websockets_server_port = 8000;           // WebSocket server port

// Hardware configuration
#define FlowSensor_INPUT D1       // Flow sensor connected to D1 pin
#define PERIOD 1000              // Measurement period in milliseconds

WebsocketsClient client;         // WebSocket client instance

// Flow measurement variables
volatile unsigned long pulse_counter = 0; // Counter for sensor pulses (volatile for ISR)
unsigned long old_time = 0;       // Timer for flow rate calculation

/**
 * Interrupt Service Routine for flow sensor pulses
 * Called on each rising edge of the sensor signal
 */
void IRAM_ATTR interrupt_handler() {
  pulse_counter++; // Increment pulse counter (must be volatile)
}

/**
 * Waits for NTP time synchronization
 * Retries up to 30 times before restarting ESP
 */
void waitForNTP() {
  Serial.print("⌛ Waiting for NTP");
  time_t now = time(nullptr);
  int attempts = 0;
  while (now < 8 * 3600 * 2) { // Wait for valid time (greater than 8*3600*2)
    delay(500);
    Serial.print(".");
    now = time(nullptr);
    attempts++;
    if (attempts > 30) { // If no response after 30 attempts
      Serial.println("\n❌ Error: NTP not responding. Restarting...");
      ESP.restart();
    }
  }
  Serial.println("\n✅ Time synchronized successfully!");
  Serial.print("🕒 Current time: ");
  Serial.println(ctime(&now));
}

/**
 * Setup function - runs once at startup
 */
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("🚀 Initializing...");

  // Scan and list available WiFi networks
  Serial.println("🔍 Scanning available WiFi networks:");
  int n = WiFi.scanNetworks();
  if (n == 0) {
    Serial.println("No networks found");
  } else {
    Serial.print(n);
    Serial.println(" networks found:");
    for (int i = 0; i < n; ++i) {
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(" dBm)");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
      delay(10);
    }
  }
  Serial.println("========================");

  // Configure flow sensor interrupt
  pinMode(FlowSensor_INPUT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FlowSensor_INPUT), interrupt_handler, RISING);
  Serial.println("✅ Flow sensor configured.");

  // WiFi setup
  Serial.println("🧹 Clearing old WiFi settings...");
  WiFi.disconnect(true);
  delay(1000);
  WiFi.mode(WIFI_STA);

  // Connect to WiFi
  Serial.print("📶 Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  int wifi_attempts = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    wifi_attempts++;
    if (wifi_attempts > 30) { // If not connected after 30 attempts
      Serial.print("\n❌ Failed to connect to WiFi. Status: ");
      Serial.println(WiFi.status());
      Serial.println("🔄 Restarting...");
      ESP.restart();
    }
  }

  Serial.println("\n✅ WiFi connected successfully!");
  Serial.print("🔗 IP: ");
  Serial.println(WiFi.localIP());

  // WebSocket connection
  Serial.print("🌐 Connecting to WebSocket: ");
  Serial.print(websockets_server_host);
  Serial.print(":");
  Serial.println(websockets_server_port);

  bool connected = client.connect(websockets_server_host, websockets_server_port, "/ws/flow-reading/");
  if (connected) {
    Serial.println("✅ WebSocket connected successfully!");
  } else {
    Serial.println("❌ Failed to connect WebSocket.");
  }

  // Set up WebSocket message callback
  client.onMessage([](WebsocketsMessage message) {
    Serial.print("📩 Message received from server: ");
    Serial.println(message.data());
  });

  // NTP time synchronization
  Serial.println("⏱️ Configuring NTP synchronization...");
  configTime(-3 * 3600, 0, "pool.ntp.org", "time.nist.gov"); // -3h timezone
  waitForNTP();
}

/**
 * Main program loop
 */
void loop() {
  client.poll();          // Handle WebSocket events
  sendFlowRate();         // Send flow rate data
  delay(1000);            // Delay between measurements
}

/**
 * Calculates and sends flow rate data to server
 */
void sendFlowRate() {
  unsigned long current_time = millis();
  unsigned long delta_time = current_time - old_time;

  // Only calculate if measurement period has elapsed
  if (delta_time >= PERIOD) {
    old_time = current_time;

    // Safely read and reset pulse counter (disable interrupts temporarily)
    noInterrupts();
    unsigned long pulse_count = pulse_counter;
    pulse_counter = 0;
    interrupts();

    // Calculate flow rate (7.5 pulses per liter per minute for this sensor)
    float pulse_rate = (pulse_count * 1000.0) / delta_time;
    float flow_rate = pulse_rate / 7.5;
    flow_rate = round(flow_rate * 100) / 100.0; // Round to 2 decimal places

    Serial.print("💧 Flow rate: ");
    Serial.print(flow_rate, 2);
    Serial.println(" L/min");

    // Get current time
    time_t now = time(nullptr);
    if (now < 8 * 3600 * 2) { // Check if time is valid
      Serial.println("❌ Invalid time, discarding reading.");
      return;
    }

    // Format timestamp
    struct tm* timeinfo = localtime(&now);
    char times_tamp[30];
    strftime(times_tamp, sizeof(times_tamp), "%d/%m/%Y %H:%M:%S", timeinfo);

    // Create JSON document
    StaticJsonDocument<200> jsonDoc;
    jsonDoc["times_tamp"] = times_tamp;
    jsonDoc["flow_rate"] = flow_rate;

    // Serialize JSON to string
    String jsonString;
    serializeJson(jsonDoc, jsonString);

    Serial.print("📤 Sending to server: ");
    Serial.println(jsonString);

    // Send via WebSocket
    bool ok = client.send(jsonString);
    if (!ok) {
      Serial.println("❌ WebSocket send failed.");
    }
  }
}