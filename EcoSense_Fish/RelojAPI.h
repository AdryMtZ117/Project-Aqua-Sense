#pragma once
#include <HTTPClient.h>
#include <WiFiClientSecure.h> // <-- Librería para conexiones con candado
#include <ArduinoJson.h>

String horaActual = "--:-- --";
unsigned long ultimoMinutoMillis = 0;
int horasLocales = 0;
int minutosLocales = 0;
int diaSemanaLocal = 0; // 0=DOM..6=SAB
bool relojSincronizado = false;

void formatearHora() {
  int h = horasLocales;
  String ampm = " AM";
  
  if (h >= 12) {
    ampm = " PM";
    if (h > 12) h -= 12;
  }
  if (h == 0) h = 12;

  String hStr = String(h);
  if (hStr.length() < 2) hStr = " " + hStr; // Alinear con espacio

  String mStr = String(minutosLocales);
  if (minutosLocales < 10) mStr = "0" + mStr; // Cero inicial a minutos

  horaActual = hStr + ":" + mStr + ampm;
}

void obtenerHoraAPI() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Conectando a servidor de tiempo seguro (HTTPS)...");
    
    WiFiClientSecure client;
    client.setInsecure(); // Clave maestra: ignora certificados para evitar bloqueos
    HTTPClient http;
    
    // Pedimos la hora exacta de México Centro
    http.begin(client, "https://timeapi.io/api/Time/current/zone?timeZone=America/Mexico_City");
    
    int httpCode = http.GET();

    if (httpCode == 200) {
      String payload = http.getString();
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, payload);

      // Esta API nos da los datos separados de forma limpia y exacta
      horasLocales = doc["hour"].as<int>();
      minutosLocales = doc["minute"].as<int>();
      
      String dayStr = doc["dayOfWeek"].as<String>();
      if (dayStr == "Sunday") diaSemanaLocal = 0;
      else if (dayStr == "Monday") diaSemanaLocal = 1;
      else if (dayStr == "Tuesday") diaSemanaLocal = 2;
      else if (dayStr == "Wednesday") diaSemanaLocal = 3;
      else if (dayStr == "Thursday") diaSemanaLocal = 4;
      else if (dayStr == "Friday") diaSemanaLocal = 5;
      else if (dayStr == "Saturday") diaSemanaLocal = 6;
      
      relojSincronizado = true;
      ultimoMinutoMillis = millis(); 
      
      formatearHora(); 
      Serial.println("¡Hora HTTPS exitosa! -> " + horaActual);
    } else {
      Serial.println("Error HTTP: " + String(httpCode));
    }
    http.end();
  }
}

// El Jefe llama a esto en cada ciclo
bool actualizarReloj() {
  if (!relojSincronizado) {
     // Si por alguna razón falló, intenta de nuevo cada 10 segundos en silencio
     static unsigned long ultimoIntento = 0;
     if (millis() - ultimoIntento > 10000) {
        ultimoIntento = millis();
        obtenerHoraAPI();
     }
     return false; 
  }

  // Si ya pasaron 60,000 milisegundos (1 minuto)
  if (millis() - ultimoMinutoMillis >= 60000) {
    ultimoMinutoMillis = millis();
    minutosLocales++;

    if (minutosLocales >= 60) {
      minutosLocales = 0;
      horasLocales++;
      if (horasLocales >= 24) horasLocales = 0;
      obtenerHoraAPI(); // Resincronizamos cada hora
    }

    formatearHora();
    return true; // Avisa que hay un nuevo minuto para dibujar
  }
  return false;
}