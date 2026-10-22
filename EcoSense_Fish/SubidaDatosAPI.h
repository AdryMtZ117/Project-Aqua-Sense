#pragma once
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h> // Recomendado: Instalala desde el gestor de librerías

extern float valor_sensor_1; // Temp Aire
extern float valor_sensor_2; // pH
extern float valor_sensor_3; // Calidad Aire
extern float valor_sensor_4; // Temp Agua
extern float valor_sensor_5; // Nivel de Agua (cm)
extern float valor_sensor_6; // Turbidez (NTU)

// Configuración del servidor n8n
String endpointServer = "http://79.143.88.218:5678/webhook/lecturas-eco-sense";

void subirDatosSensoresServidor() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[API] Enviando telemetría a eco-sense...");
    
    WiFiClient client;
    
    HTTPClient http;
    http.begin(client, endpointServer);
    http.addHeader("Content-Type", "application/json");

    // Usamos ArduinoJson para un formateo seguro
    StaticJsonDocument<384> doc;
    doc["id_dispositivo"]  = 1;
    doc["temp_aire"]       = valor_sensor_1;   // Temperatura del aire
    doc["ph"]              = valor_sensor_2;   // pH del agua
    doc["calidad_aire_ppm"]= valor_sensor_3;   // Calidad del aire en ppm
    doc["temp_agua"]       = valor_sensor_4;   // Temperatura del agua
    doc["nivel_agua_cm"]   = valor_sensor_5;   // Nivel de agua en cm
    doc["turbidez_volt"]   = valor_sensor_6;   // Turbidez en voltios

    String jsonPayload;
    serializeJson(doc, jsonPayload);

    int httpResponseCode = http.POST(jsonPayload);
    
    if (httpResponseCode > 0) {
      Serial.printf("[API] Éxito. Código: %d\n", httpResponseCode);
    } else {
      Serial.printf("[API] Error: %s\n", http.errorToString(httpResponseCode).c_str());
    }
    
    http.end();
  } else {
    Serial.println("[API] Error: Sin conexión WiFi.");
  }
}