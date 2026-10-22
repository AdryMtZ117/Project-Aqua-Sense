#pragma once

#include <TFT_eSPI.h>

// ===== PALETA DE COLORES =====
#define C_FONDO      0x0841 
#define C_SOL        0xFD20 
#define C_GRIS_SOFT  0x52AA 
#define C_TEXTO      0xFFFF 
#define C_CYAN       0x07FF 
#define C_TARJETA    0x10A2 // Un azul un poco más claro que el fondo para las tarjetas

// Función auxiliar (inline) para dibujar pequeños iconos de clima
inline void dibujarIconoClimaMini(TFT_eSPI* tft, int cx, int cy, int tipo) {
  if (tipo == 0) { // Sol
    tft->fillCircle(cx, cy, 12, C_SOL);
  } else if (tipo == 1) { // Nube
    tft->fillCircle(cx - 6, cy + 2, 8, C_TEXTO);
    tft->fillCircle(cx + 6, cy + 2, 8, C_TEXTO);
    tft->fillCircle(cx, cy - 4, 10, C_TEXTO);
    tft->fillRoundRect(cx - 10, cy + 2, 20, 8, 4, C_TEXTO);
  } else if (tipo == 2) { // Sol y Nube
    tft->fillCircle(cx - 8, cy - 8, 10, C_SOL);
    tft->fillCircle(cx, cy, 10, C_TEXTO);
    tft->fillCircle(cx + 10, cy + 4, 8, C_TEXTO);
    tft->fillRoundRect(cx - 5, cy + 4, 20, 8, 4, C_TEXTO);
  }
}

void dibujarPantallaCalendario(TFT_eSPI* tft) {
  // 1. Botón de "Atrás" (Hitbox: x < 60, y < 60)
  tft->fillRoundRect(10, 10, 40, 30, 5, C_GRIS_SOFT);
  tft->setTextColor(C_TEXTO);
  tft->drawCentreString("<", 30, 16, 2);

  // 2. Título de la sección
  tft->setTextColor(C_SOL);
  tft->drawCentreString("PRONOSTICO", 160, 15, 4);

  // 3. Ya no usamos variables de prueba, usamos los globales
  // de ClimaAPI.h (diasGlobal, maxTempGlobal, minTempGlobal, iconosGlobal)

  // 4. Medidas matemáticas para centrar 4 tarjetas
  int startX = 15;   // Margen izquierdo general
  int startY = 70;   // Altura a la que empiezan las tarjetas
  int cardW = 65;    // Ancho de cada tarjeta
  int cardH = 130;   // Alto de cada tarjeta
  int spacing = 10;  // Espacio entre tarjetas

  // 5. Bucle para dibujar las 4 tarjetas
  for (int i = 0; i < 4; i++) {
    int x = startX + (i * (cardW + spacing));
    int centroTarjeta = x + (cardW / 2);
    
    // Dibujar el fondo de la tarjeta con esquinas redondeadas
    tft->fillRoundRect(x, startY, cardW, cardH, 8, C_TARJETA); 
    
    // Dibujar el texto del Día (ej. "LUN")
    tft->setTextColor(C_CYAN);
    tft->drawCentreString(diasGlobal[i], centroTarjeta, startY + 10, 2);
    
    // Dibujar el Icono del clima usando nuestra función auxiliar
    dibujarIconoClimaMini(tft, centroTarjeta, startY + 50, iconosGlobal[i]);
    
    // Dibujar la Temperatura Máxima (Grande)
    tft->setTextColor(C_TEXTO);
    tft->drawCentreString(String(maxTempGlobal[i]) + "o", centroTarjeta, startY + 80, 4);
    
    // Dibujar la Temperatura Mínima (Pequeña y gris)
    tft->setTextColor(C_GRIS_SOFT);
    tft->drawCentreString(String(minTempGlobal[i]) + "o", centroTarjeta, startY + 105, 2);
  }
}