#pragma once

#include <TFT_eSPI.h>

// ===== PARAMETROS DE SENSORES (DINAMICOS) =====
float valor_sensor_1 = 0.0; // Temp Aire
float valor_sensor_2 = 0.0; // pH
float valor_sensor_3 = 0.0; // Gas (MQ-9)
float valor_sensor_4 = 0.0; // Temp Agua (DS18B20)
float valor_sensor_5 = 0.0; // Nivel de Agua (cm)
float valor_sensor_6 = 0.0; // Turbidez (NTU)

// ===== METODOS PARA DIBUJAR LA INTERFAZ =====
// Funciones auxiliares genéricas basadas en el diseño original ("interfaces que ya te pase")
void dibujarInterfazGenerica(TFT_eSPI* tft, String titulo, String valor, String unidad, uint16_t colorTema) {
  // 1. Botón de "Atrás" (Hitbox: x < 60, y < 60)
  tft->fillRoundRect(10, 10, 40, 30, 5, 0x52AA); // C_GRIS_SOFT
  tft->setTextColor(0xFFFF); // C_TEXTO
  tft->drawCentreString("<", 30, 16, 2);

  // 2. Título de la sección
  tft->setTextColor(0xFD20); // C_SOL
  tft->drawCentreString(titulo, 160, 15, 4);

  // 3. Área Visual del Sensor
  int cx = 160;
  int cy = 120; // Altura central
  
  // Anillo de diseño
  tft->fillCircle(cx, cy, 65, colorTema);
  tft->fillCircle(cx, cy, 55, 0x0841); // C_FONDO
  
  // Valor dinámico
  tft->setTextColor(0xFFFF); // C_TEXTO
  tft->drawCentreString(valor, cx, cy - 20, 6); // Fuente 6 grande
  
  // Unidad / Estado
  tft->setTextColor(colorTema);
  tft->drawCentreString(unidad, cx, cy + 26, 2);
  
  // 4. FOOTER
  tft->drawFastHLine(20, 210, 280, 0x52AA); // C_GRIS_SOFT
  tft->setTextColor(0x07FF); // C_CYAN
  tft->drawCentreString("Recibiendo de Arduino Uno", 160, 218, 2);
}

void dibujarPantallaSensor1(TFT_eSPI* tft) {
  dibujarInterfazGenerica(tft, "TEMP. DEL AIRE", String(valor_sensor_1, 1), "Grados C", 0xFD20); // Naranja/Sol
}

void dibujarPantallaSensor2(TFT_eSPI* tft) {
  dibujarInterfazGenerica(tft, "pH AGUA", String(valor_sensor_2, 1), "pH Libre", 0x07E0); // Verde
}

void dibujarPantallaSensor3(TFT_eSPI* tft) {
  // Gas sin decimales
  dibujarInterfazGenerica(tft, "CALIDAD DE AIRE", String((int)valor_sensor_3), "Nivel CRUDO", 0xF800); // Rojo
}

void dibujarPantallaSensor4(TFT_eSPI* tft) {
  dibujarInterfazGenerica(tft, "TEMP. DEL AGUA", String(valor_sensor_4, 1), "Grados C", 0x07FF); // Cyan
}

void dibujarPantallaSensor5(TFT_eSPI* tft) {
  dibujarInterfazGenerica(tft, "NIVEL DE AGUA", String(valor_sensor_5, 1), "cm dist.", 0x3C1F); // Azul marino
}

void dibujarPantallaSensor6(TFT_eSPI* tft) {
  dibujarInterfazGenerica(tft, "TURBIDEZ", String(valor_sensor_6, 1), "NTU", 0x8410); // Gris azulado
}
