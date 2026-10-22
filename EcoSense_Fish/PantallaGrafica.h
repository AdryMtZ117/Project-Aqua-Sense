#pragma once

#include <TFT_eSPI.h>

// ===== PALETA DE COLORES =====
#define C_FONDO      0x0841 
#define C_SOL        0xFD20 
#define C_GRIS_SOFT  0x52AA 
#define C_TEXTO      0xFFFF 
#define C_CYAN       0x07FF 
#define C_LINEA      0x07E0 // Verde para la línea de tendencia

void dibujarPantallaGrafica(TFT_eSPI* tft) {
  // 1. Botón "Atrás"
  tft->fillRoundRect(10, 10, 40, 30, 5, C_GRIS_SOFT);
  tft->setTextColor(C_TEXTO);
  tft->drawCentreString("<", 30, 16, 2);

  // 2. Título
  tft->setTextColor(C_SOL);
  tft->drawCentreString("TENDENCIA TEMP", 160, 15, 4);

  // 3. Área de dibujo de la gráfica
  int origenX = 40;     // Esquina inferior izquierda (X)
  int origenY = 200;    // Esquina inferior izquierda (Y)
  int anchoG  = 240;    // Ancho total del eje X
  int altoG   = 120;    // Alto total del eje Y

  // Dibujar ejes (X y Y)
  tft->drawFastVLine(origenX, origenY - altoG, altoG, C_TEXTO); // Eje Y
  tft->drawFastHLine(origenX, origenY, anchoG, C_TEXTO);        // Eje X

  // 4. Datos reales globales (6 puntos)
  int tempMinima = 100;
  int tempMaxima = -100;
  for (int i = 0; i < 6; i++) {
    if (tempHistoricoGlobal[i] < tempMinima && tempHistoricoGlobal[i] != 0) tempMinima = tempHistoricoGlobal[i];
    if (tempHistoricoGlobal[i] > tempMaxima) tempMaxima = tempHistoricoGlobal[i];
  }
  
  // Dar un margen a la gráfica para que no quede pegada al borde
  if (tempMinima == 100) tempMinima = 10; // Si no hay datos, valor por defecto
  if (tempMaxima == -100) tempMaxima = 30;
  tempMinima -= 2;
  tempMaxima += 2;

  // 5. Variables para calcular coordenadas y dibujar la línea
  int espaciadoX = anchoG / 5; // Separación horizontal entre puntos

  int xAnterior = -1;
  int yAnterior = -1;

  // 6. Bucle para dibujar los puntos y las líneas que los conectan
  for (int i = 0; i < 6; i++) {
    int xActual = origenX + (i * espaciadoX);
    
    // Mapeamos la temperatura al píxel correspondiente en el eje Y
    // (Invertimos la Y porque en pantallas la coordenada 0 está arriba)
    int yActual = map(tempHistoricoGlobal[i], tempMinima, tempMaxima, origenY, origenY - altoG);

    // Dibujamos la línea conectora (si no es el primer punto)
    if (i > 0) {
      // Línea gruesa (dibujando 3 líneas juntas) para que resalte
      tft->drawLine(xAnterior, yAnterior, xActual, yActual, C_LINEA);
      tft->drawLine(xAnterior, yAnterior - 1, xActual, yActual - 1, C_LINEA);
      tft->drawLine(xAnterior, yAnterior + 1, xActual, yActual + 1, C_LINEA);
    }

    // Dibujamos el "punto" circular en cada dato
    tft->fillCircle(xActual, yActual, 4, C_SOL);
    
    // Etiqueta de la temperatura encima del punto
    tft->setTextColor(C_TEXTO);
    tft->drawCentreString(String(tempHistoricoGlobal[i]) + "o", xActual, yActual - 15, 2);

    // Etiqueta de la hora en el eje X
    tft->setTextColor(C_GRIS_SOFT);
    tft->drawCentreString(horasGlobal[i] + "h", xActual, origenY + 8, 2);

    // Guardamos las coordenadas actuales para el siguiente ciclo
    xAnterior = xActual;
    yAnterior = yActual;
  }

  // 7. Etiquetas de referencia en el eje Y (opcional para dar contexto)
  tft->setTextColor(C_GRIS_SOFT);
  tft->drawRightString(String(tempMaxima) + "o", origenX - 5, origenY - altoG, 2); // Tope
  tft->drawRightString(String((tempMaxima + tempMinima) / 2) + "o", origenX - 5, origenY - (altoG / 2), 2); // Medio
  tft->drawRightString(String(tempMinima) + "o", origenX - 5, origenY - 10, 2); // Base
}