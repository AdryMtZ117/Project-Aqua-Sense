#pragma once

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Credenciales de red y API de Clima
// NOTA: Para no filtrar credenciales a Git, se cargan desde "secrets.h" (protegido en .gitignore).
#if __has_include("secrets.h")
  #include "secrets.h"
#else
  const char* ssid = "TU_WIFI_SSID";
  const char* password = "TU_WIFI_PASSWORD";
  const String apiKey = "TU_OPENWEATHER_API_KEY";
#endif

// Variables globales dinámicas
String clima_ciudad = "BUSCANDO...";
float latitud_actual = 19.4839;   // Coordenada base CDMX (GAM)
float longitud_actual = -99.1136; // Coordenada base CDMX (GAM)

String clima_temperatura = "--";
String clima_descripcion = "Buscando red...";
String clima_humedad = "--%";
String clima_viento = "--km/h";

// --- Variables globales para Gráfica ---
int tempHistoricoGlobal[6] = {0, 0, 0, 0, 0, 0};
String horasGlobal[6] = {"--", "--", "--", "--", "--", "--"};

// --- Variables globales para Pronóstico (Calendario) ---
String diasGlobal[4] = {"", "", "", ""};
int maxTempGlobal[4] = {0, 0, 0, 0};
int minTempGlobal[4] = {0, 0, 0, 0};
int iconosGlobal[4]  = {0, 0, 0, 0}; 

extern int diaSemanaLocal; // Declarado en RelojAPI.h

void conectarWiFi() {
  Serial.println("Iniciando búsqueda de WiFi en segundo plano...");
  WiFi.begin(ssid, password);
}

// 1. Obtener coordenadas y municipio/ciudad según la IP pública
void obtenerUbicacionPorIP() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("http://ip-api.com/json/?fields=status,city,regionName,lat,lon");
    
    int httpCode = http.GET();
    if (httpCode == 200) {
      String payload = http.getString();
      JsonDocument doc;
      DeserializationError err = deserializeJson(doc, payload);
      
      const char* status = doc["status"];
      if (!err && status != nullptr && strcmp(status, "success") == 0) {
        latitud_actual = doc["lat"] | 19.4839f;
        longitud_actual = doc["lon"] | -99.1136f;
        
        const char* cityRaw = doc["city"];
        if (cityRaw != nullptr) {
          String ciudadDetectada = String(cityRaw);
          ciudadDetectada.toUpperCase();
          clima_ciudad = ciudadDetectada;
        }
        
        Serial.printf("[GEO] Ubicación detectada: %s (Lat: %.4f, Lon: %.4f)\n", 
                      clima_ciudad.c_str(), latitud_actual, longitud_actual);
      }
    } else {
      Serial.println("[GEO] Error al consultar geolocalización IP.");
    }
    http.end();
  }
}

// 2. Consulta de clima con coordenadas dinámicas
void actualizarClima() {
  if (WiFi.status() == WL_CONNECTED) {
    if (clima_ciudad == "BUSCANDO...") {
      obtenerUbicacionPorIP();
    }

    HTTPClient http;
    
    String urlOpenWeather = "http://api.openweathermap.org/data/2.5/forecast?lat=" 
                            + String(latitud_actual, 4) 
                            + "&lon=" + String(longitud_actual, 4) 
                            + "&appid=" + apiKey 
                            + "&units=metric&lang=es";

    Serial.println("Consultando OpenWeatherMap con coordenadas dinámicas...");
    http.begin(urlOpenWeather);
    
    int codigoRespuesta = http.GET();
    
    if (codigoRespuesta == 200) {
      String respuestaJSON = http.getString();
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, respuestaJSON);

      if (!error) {
        // Priorizar el nombre de localidad devuelto por OpenWeatherMap si está disponible
        const char* nombreOWMRaw = doc["city"]["name"];
        if (nombreOWMRaw != nullptr && strlen(nombreOWMRaw) > 0) {
          String nombreOWM = String(nombreOWMRaw);
          nombreOWM.toUpperCase();
          clima_ciudad = nombreOWM;
        }

        // --- 1. Clima Actual ---
        float tempActual = doc["list"][0]["main"]["temp"] | 0.0f;
        clima_temperatura = String((int)round(tempActual));
        
        const char* descRaw = doc["list"][0]["weather"][0]["description"];
        if (descRaw != nullptr) {
          String desc = String(descRaw);
          if (desc.length() > 0) {
            desc[0] = toupper(desc[0]);
          }
          clima_descripcion = desc;
        }
        
        int humedadAPI = doc["list"][0]["main"]["humidity"] | 0;
        clima_humedad = String(humedadAPI) + "%";
        
        float vientoAPI_ms = doc["list"][0]["wind"]["speed"] | 0.0f;
        int vientoAPI_kmh = (int)round(vientoAPI_ms * 3.6f);
        clima_viento = String(vientoAPI_kmh) + "km/h";
        
        // --- 2. Datos para la Gráfica (próximas 18 horas) ---
        for (int i = 0; i < 6; i++) {
          if ((int)doc["list"].size() > i) {
            float t = doc["list"][i]["main"]["temp"] | 0.0f;
            tempHistoricoGlobal[i] = (int)round(t);
            
            const char* dtTxtRaw = doc["list"][i]["dt_txt"];
            if (dtTxtRaw != nullptr) {
              String dt_txt = String(dtTxtRaw);
              horasGlobal[i] = dt_txt.substring(11, 13);
            }
          }
        }
        
        // --- 3. Pronóstico a 4 días ---
        String nombresDias[7] = {"DOM", "LUN", "MAR", "MIE", "JUE", "VIE", "SAB"};
        for (int d = 0; d < 4; d++) {
          int startIdx = 8 + (d * 8);
          
          if ((int)doc["list"].size() > startIdx) {
            float dMax = -100.0f;
            float dMin = 100.0f;
            int weatherId = 800;
            
            for (int j = startIdx; j < startIdx + 8 && j < (int)doc["list"].size(); j++) {
              float t_max = doc["list"][j]["main"]["temp_max"] | -100.0f;
              float t_min = doc["list"][j]["main"]["temp_min"] | 100.0f;
              if (t_max > dMax) dMax = t_max;
              if (t_min < dMin) dMin = t_min;
              
              if (j == startIdx + 4) {
                 weatherId = doc["list"][j]["weather"][0]["id"] | 800;
              }
            }
            maxTempGlobal[d] = (int)round(dMax);
            minTempGlobal[d] = (int)round(dMin);
            
            if (weatherId >= 200 && weatherId < 800) iconosGlobal[d] = 1;
            else if (weatherId == 800) iconosGlobal[d] = 0;
            else if (weatherId == 801 || weatherId == 802) iconosGlobal[d] = 2;
            else iconosGlobal[d] = 1;
          }
          
          diasGlobal[d] = nombresDias[(diaSemanaLocal + d + 1) % 7];
        }
        
        Serial.printf("[CLIMA] %s | %s°C | %s\n", clima_ciudad.c_str(), clima_temperatura.c_str(), clima_descripcion.c_str());
      } else {
        Serial.println("Error al leer el JSON: " + String(error.c_str()));
      }
    } else {
      Serial.printf("Error en la petición HTTP. Código: %d\n", codigoRespuesta);
    }
    http.end();
  }
}