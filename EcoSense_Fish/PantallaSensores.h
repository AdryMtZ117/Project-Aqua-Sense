#pragma once

#include <TFT_eSPI.h>

// ===== PALETA DE COLORES BASE =====
#define C_FONDO      0x0841 
#define C_SOL        0xFD20 
#define C_GRIS_SOFT  0x52AA 
#define C_TEXTO      0xFFFF 
#define C_CYAN       0x07FF 

void dibujarPantallaSensores(TFT_eSPI* tft) {
  // 1. Botón de "Atrás" (Hitbox: x < 60, y < 60)
  tft->fillRoundRect(10, 10, 40, 30, 5, C_GRIS_SOFT);
  tft->setTextColor(C_TEXTO);
  tft->drawCentreString("<", 30, 16, 2);

  // 2. Título de la sección
  tft->setTextColor(C_SOL);
  tft->drawCentreString("MENU DE SENSORES", 160, 15, 4);

  // 3. Opciones del Menú (6 Sensores)
  int startY = 50;    // Inicio más arriba para caber 6 botones
  int btnHeight = 27; // Botones más compactos
  int btnWidth = 220;
  int startX = (320 - btnWidth) / 2; // Centrado
  int gap = 5;        // Separación entre botones
  
  String nombresMenu[6] = {
    "1. Temp. del Aire",
    "2. pH del Agua",
    "3. Calidad de Aire",
    "4. Temp. del Agua",
    "5. Nivel de Agua",
    "6. Turbidez"
  };

  for (int i = 0; i < 6; i++) {
    int yPos = startY + (i * (btnHeight + gap));
    
    // Dibujar fondo del botón
    tft->fillRoundRect(startX, yPos, btnWidth, btnHeight, 6, C_GRIS_SOFT);
    
    // Texto del botón
    tft->setTextColor(C_TEXTO);
    tft->drawString(nombresMenu[i], startX + 12, yPos + 8, 2);
    
    // Flecha indicativa de navegación
    tft->setTextColor(C_CYAN);
    tft->drawString(">", startX + btnWidth - 18, yPos + 8, 2);
  }
}
