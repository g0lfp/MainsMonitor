#include <EmonLib.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WiFi.h>

// Create an instance of the emonlib library
EnergyMonitor emon1;

// Create an instance of the ESP8266WebServer library
ESP8266WebServer server(80);

// Variables to store the voltage and current readings
float voltage = 240.0; // Set to the known voltage value
float current;
float power;

// Set up the OTA update server
const char* host = "ESP8266_Power_Monitor";
ESP8266HTTPUpdateServer httpUpdater;

void setup() {
  // Initialize the serial communication
  Serial.begin(115200);

  // Initialize the emonlib library
  emon1.current(1, 20); // Current: input pin, calibration.

  // Initialize the WiFi Manager library
  WiFiManager wifiManager;
  wifiManager.autoConnect("ESP8266_Power_Monitor");

  // Set up the web server
  server.on("/", HTTP_GET, handleOnlineState);

  server.begin();
  Serial.println("HTTP server started");

  // Set up the OTA update server
  httpUpdater.setup(&server, host, "/update");
  Serial.println("OTA server started");

  // Enable the watchdog timer with a timeout of 5 seconds
  ESP.wdtEnable(5);

  // Feed the watchdog timer to prevent it from resetting the ESP8266
  ESP.wdtFeed();
}

void loop() {
  // Check if the ESP8266 is connected to the WiFi network
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Disconnected from WiFi. Trying to reconnect...");
    WiFiManager wifiManager;
    wifiManager.autoConnect("ESP8266_Power_Monitor");
  }

  server.handleClient();

  // Read the current and voltage values
  current = emon1.calcIrms(1480); // Calculate Irms only.
  power = voltage * current;

  // Print the power value to the serial monitor
  Serial.print("Power: ");
  Serial.print(power);
  Serial.println(" W");

  // Send the power value to the web server every 10 seconds
  if (millis() % (10 * 1000) == 0) {
    sendPowerValue(power);
  }

  // Handle OTA updates
  httpUpdater.handleClient();

  // Feed the watchdog timer to prevent it from resetting the ESP8266
  ESP.wdtFeed();

  // Add any necessary error handling or recovery code here

  // If an error occurs and the necessary recovery steps are not taken, the watchdog timer will reset the ESP8266 after the specified timeout period
}

void handleOnlineState() {
  server.send(200, "text/plain", "Online");
}

void sendPowerValue(float power) {
  // Construct the URL with the power value
  String url = serverName;
  url += "?power=";
  url += power;

  // Send the HTTP request
  Serial.println("Sending power value to web server...");
  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();
  if (httpCode > 0) {
    Serial.printf("HTTP response code: %d\n", httpCode);
  } else {