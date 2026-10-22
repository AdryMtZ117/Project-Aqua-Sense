#pragma once

#include <TFT_eSPI.h>

// Mantenemos la paleta de colores
#define C_FONDO      0x0841 
#define C_BARRA      0x0000 
#define C_SOL        0xFD20 
#define C_GRIS_SOFT  0x52AA 
#define C_TEXTO      0xFFFF 
#define C_VERDE      0x07E0 // Color extra para el interruptor "Encendido"

extern bool wifiHabilitado;
extern bool bluetoothHabilitado;
extern int nivelBrillo;

void dibujarPantallaAjustes(TFT_eSPI* tft) {
  // 1. Botón de "Atrás" (Hitbox: x < 60, y < 60)
  tft->fillRoundRect(10, 10, 40, 30, 5, C_GRIS_SOFT);
  tft->setTextColor(C_TEXTO);
  tft->drawCentreString("<", 30, 16, 2);

  // 2. Título de la sección
  tft->setTextColor(C_SOL);
  tft->drawCentreString("AJUSTES", 160, 15, 4);

  // 3. Opción 1: Conexión WiFi
  tft->setTextColor(C_TEXTO);
  tft->drawString("WiFi", 40, 80, 4); 

  // Switch a la Izquierda y Botón a la Derecha dinámico
  if (wifiHabilitado) {
    // Dibujo del Toggle ON (Verde)
    tft->fillRoundRect(115, 78, 44, 24, 12, C_VERDE);
    // Círculo blanco a la derecha (activado)
    tft->fillCircle(147, 90, 10, C_TEXTO); 
    
    // Botón "Configurar" visible
    tft->fillRoundRect(178, 76, 92, 28, 4, 0x01EB); // Azul oscuro
    tft->setTextColor(C_TEXTO);
    tft->drawCentreString("Configurar", 224, 82, 2); // 178 + 46 = 224
  } else {
    // Dibujo del Toggle OFF (Gris)
    tft->fillRoundRect(115, 78, 44, 24, 12, C_GRIS_SOFT);
    // Círculo blanco a la izquierda (desactivado)
    tft->fillCircle(127, 90, 10, C_TEXTO); 
  }

  // 4. Opción 2: Bluetooth (Interruptor)
  tft->setTextColor(C_TEXTO);
  tft->drawString("Bluetooth", 40, 130, 4); 
  
  if (bluetoothHabilitado) {
    tft->fillRoundRect(220, 128, 44, 24, 12, C_VERDE);
    // Círculo blanco a la derecha (activado)
    tft->fillCircle(252, 140, 10, C_TEXTO);
  } else {
    // Dibujo del Toggle OFF
    // Fondo gris redondeado
    tft->fillRoundRect(220, 128, 44, 24, 12, C_GRIS_SOFT);
    // Círculo blanco a la izquierda (desactivado)
    tft->fillCircle(232, 140, 10, C_TEXTO);
  }


  // 5. Opción 3: Deslizador de Brillo (Slider)
  tft->setTextColor(C_TEXTO);
  tft->drawString("Brillo", 40, 180, 4); 
  
  // Barra de fondo (Gris)
  tft->fillRoundRect(120, 188, 140, 8, 4, C_GRIS_SOFT);
  // Barra de nivel (Amarillo/Sol - dinámico)
  tft->fillRoundRect(120, 188, nivelBrillo, 8, 4, C_SOL);
  // Botón del deslizador (Blanco)
  tft->fillCircle(120 + nivelBrillo, 192, 10, C_TEXTO);
  
  // Línea separadora sutil al final
  tft->drawFastHLine(20, 230, 280, C_GRIS_SOFT);
}