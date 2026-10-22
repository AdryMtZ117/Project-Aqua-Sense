#pragma once

#include <TFT_eSPI.h>
#include "SPIFFS.h"

// --- COLORES DEL LOGO ---
#define COLOR_FONDO_S   0x867D  // Azul Cielo
#define COLOR_BLANCO_S  0xFFFF  // Blanco puro
#define COLOR_MOSTAZA_S 0xFEA0  // Amarillo Mostaza
#define COLOR_AMARILLO_S 0xFFE0 // Amarillo Puro
#define COLOR_CREMA_S   0xFFF8  // Amarillo muy pálido

void dibujarSplash(TFT_eSPI* tft) {
  int cx = 160;
  int cy = 120;

  // Iniciamos la memoria interna para poder cargar tu fuente
  if (!SPIFFS.begin()) {
    Serial.println("Error iniciando SPIFFS");
  }

  // PASO 1: Fondo azul cielo
  tft->fillScreen(COLOR_FONDO_S);

  // PASO 2: Círculo grande blanco
  tft->fillCircle(cx, cy, 90, COLOR_BLANCO_S);

  // PASO 3: 12 círculos blancos en órbita
  int radio_orbita = 90; 
  for (int i = 0; i < 12; i++) {
    float angulo = i * (360.0 / 12.0) * 0.0174533; 
    int x_borde = cx + cos(angulo) * radio_orbita;
    int y_borde = cy + sin(angulo) * radio_orbita;
    tft->fillCircle(x_borde, y_borde, 12, COLOR_BLANCO_S);
  }

  // PASO 4: Corte diagonal con triángulos
  tft->fillTriangle(0, 0, 300, 0, 0, 300, COLOR_FONDO_S); 
  tft->fillTriangle(0, 0, 160, 0, 0, 120, COLOR_FONDO_S); 

  // PASO 5: Hueco central del engranaje
  tft->fillCircle(cx, cy, 70, COLOR_FONDO_S);

  // PASO 6: Anillos concéntricos
  tft->fillCircle(cx, cy, 65, COLOR_MOSTAZA_S);
  tft->fillCircle(cx, cy, 45, COLOR_AMARILLO_S);
  tft->fillCircle(cx, cy, 25, COLOR_CREMA_S);
  tft->fillCircle(cx, cy, 10, COLOR_BLANCO_S);

  // PASO 7: Rayos Sólidos (Técnica de Arrastre)
  int radio_inicio = 80; 
  int longitud_rayo = 21;
  int angulos_rayos[] = {135, 180, 225, 270, 315}; 
  int radio_pincel = 8; 

  for (int i = 0; i < 5; i++) {
    float ang = angulos_rayos[i] * 0.0174533; 
    
    float x1 = cx + cos(ang) * radio_inicio;
    float y1 = cy + sin(ang) * radio_inicio;
    float x2 = cx + cos(ang) * (radio_inicio + longitud_rayo);
    float y2 = cy + sin(ang) * (radio_inicio + longitud_rayo);

    float distancia = sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));
    
    for (float k = 0; k <= distancia; k += 1.0) { 
        float ratio = k / distancia;
        int x_actual = x1 + (x2 - x1) * ratio;
        int y_actual = y1 + (y2 - y1) * ratio;
        tft->fillCircle(x_actual, y_actual, radio_pincel, COLOR_AMARILLO_S);
    }
  }

  // PASO 8: Texto con fuente personalizada
  tft->loadFont("NotoSansBold15"); 
  tft->setTextColor(TFT_BLACK);  
  tft->setTextSize(4);  
  tft->setTextDatum(MC_DATUM);  
  tft->drawString("Aqua-sense", tft->width()/2, tft->height()-20);
  tft->unloadFont();
  
  // Regresamos el anclaje de texto a la normalidad para no afectar el resto del sistema
  tft->setTextDatum(TL_DATUM); 
  tft->setTextSize(1); // <--- ¡ESTA ES LA LÍNEA MÁGICA QUE FALTABA!
}