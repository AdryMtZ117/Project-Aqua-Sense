#pragma once

#include <TFT_eSPI.h>

// ===== PALETA DE COLORES (TEMA CLARO) =====
#define C_CIELO      0xB71C  // Azul cielo pálido
#define C_BARRA      0xFFFF  // Blanco puro para la barra lateral
#define C_TEXTO_N    0x0000  // Negro para los textos principales
#define C_SOL        0xFD20  // Amarillo Oro
#define C_NARANJA    0xFB00  // Naranja
#define C_GRIS_OSC   0x4A49  // Gris oscuro para que los iconos resalten
#define C_DETALLE    0x9DF3  // Azul intermedio para la barra de abajo
#define C_AZUL_OSC   0x01EB  // Azul oscuro para la descripción del clima

extern String clima_ciudad;       // <-- Variable dinámica para la ubicación
extern String clima_temperatura;
extern String clima_descripcion;
extern String clima_humedad;
extern String clima_viento;
extern String horaActual;

void drawSidebarIcon(TFT_eSPI* tft, int x, int y, String type);

void dibujarPantallaPrincipal(TFT_eSPI* tft) {
  int w = 320;
  int h = 240;
  int bW = 55;   
  int mainW = w - bW; 

  // --- FONDOS TEMA CLARO ---
  tft->fillRect(0, 0, mainW, h, C_CIELO);      
  tft->fillRect(mainW, 0, bW, h, C_BARRA);     
  tft->drawFastVLine(mainW, 0, h, 0xD6BA);     

  // --- BARRA LATERAL ---
  tft->fillRoundRect(mainW + 6, 12, 42, 42, 12, 0xFFE0); 
  tft->fillRoundRect(mainW + 8, 14, 38, 38, 10, C_SOL);  
  
  // Icono Sol en Sidebar
  tft->fillCircle(mainW + 27, 33, 5, C_BARRA);
  for(int i=0; i<360; i+=45) {
    float r = i * 0.0174;
    tft->drawLine(mainW+27+cos(r)*7, 33+sin(r)*7, mainW+27+cos(r)*10, 33+sin(r)*10, C_BARRA);
  }

  drawSidebarIcon(tft, mainW + 27, 80, "cal");
  drawSidebarIcon(tft, mainW + 27, 125, "speed");
  drawSidebarIcon(tft, mainW + 27, 170, "chart");
  drawSidebarIcon(tft, mainW + 27, 215, "settings");

  // --- ÁREA DE CLIMA DINÁMICA ---
  tft->setTextColor(C_TEXTO_N);
  // Usa la variable dinámica; si excede el ancho se puede ajustar la fuente a tamaño 2
  tft->drawCentreString(clima_ciudad, mainW/2, 35, 2); 
  
  tft->setTextColor(C_GRIS_OSC);
  tft->drawString(horaActual, 15, 12, 2);

  tft->drawCircle(mainW - 45, 18, 5, C_GRIS_OSC); // Icono BT

  // Sol Central
  int cx = mainW / 2;
  int cy = 95;
  tft->fillCircle(cx, cy, 22, C_NARANJA);
  for (int i = 0; i < 360; i += 30) {
    float rad = i * 0.0174;
    tft->drawLine(cx+cos(rad)*28, cy+sin(rad)*28, cx+cos(rad)*38, cy+sin(rad)*38, C_SOL);
  }

  // Temperatura real
  tft->setTextColor(C_TEXTO_N);
  tft->drawCentreString(clima_temperatura, cx - 12, 135, 7); 
  tft->setTextColor(C_SOL);
  tft->drawString("oC", cx + 48, 148, 4);

  // Descripción real
  tft->setTextColor(C_AZUL_OSC);
  tft->drawCentreString(clima_descripcion, cx, 202, 2);

  // --- FOOTER INFO ---
  tft->fillRoundRect(20, 220, mainW - 40, 18, 9, C_DETALLE);
  tft->setTextColor(C_TEXTO_N);
  tft->drawString(clima_humedad, cx - 55, 222, 2);
  tft->drawString(clima_viento, cx + 15, 222, 2);
}

void drawSidebarIcon(TFT_eSPI* tft, int x, int y, String type) {
  if(type == "cal") {
    tft->drawRoundRect(x-12, y-10, 24, 20, 3, C_GRIS_OSC);
    tft->drawFastHLine(x-12, y-3, 24, C_GRIS_OSC);
  } else if(type == "speed") {
    tft->drawCircle(x, y, 10, C_GRIS_OSC);
    tft->drawLine(x, y, x+6, y-6, C_GRIS_OSC);
  } else if(type == "chart") {
    tft->drawLine(x-10, y+5, x-3, y-5, C_GRIS_OSC);
    tft->drawLine(x-3, y-5, x+4, y, C_GRIS_OSC);
    tft->drawLine(x+4, y, x+12, y-10, C_GRIS_OSC);
  } else if(type == "settings") {
    tft->drawCircle(x, y, 8, C_GRIS_OSC);
    tft->drawCircle(x, y, 3, C_GRIS_OSC);
  }
}