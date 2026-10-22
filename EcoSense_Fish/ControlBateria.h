#pragma once

#include <TFT_eSPI.h>

#define BATTERY_WIDTH 30
#define BATTERY_HEIGHT 10
#define COLOUR_FULL 0xFD20
#define COLOUR_LOW  0xFB00
#define COLOUR_MID  0xFF00
#define COLOUR_CONTOUR 0xFFFF

class ControlGraficoBateria {
private:
   int level, x_pos, y_pos;
   bool is_low; 
   uint16_t bg_color; // Color de fondo adaptable
   TFT_eSPI* tft; 

public:
    // Ahora el constructor SI recibe 4 parámetros (el último es el color de fondo)
    ControlGraficoBateria(int x, int y, TFT_eSPI *_tft, uint16_t _bg_color = TFT_BLACK) {
        x_pos = x;
        y_pos = y;
        tft = _tft;
        bg_color = _bg_color;
        level = -1; // Fuerza el primer dibujo
        is_low = false;
    }

    void Paint(int _charge_) {
        if(_charge_ < 0) _charge_ = 0;
        if(_charge_ > 100) _charge_ = 100;

        if(level == _charge_) return;
        level = _charge_;

        // 1. Contorno
        tft->drawRoundRect(x_pos, y_pos, BATTERY_WIDTH, BATTERY_HEIGHT, 2, COLOUR_CONTOUR);
        tft->fillRect(x_pos + BATTERY_WIDTH, y_pos + 3, 2, 4, COLOUR_CONTOUR);

        // 2. Borrar interior con el color de fondo correcto
        tft->fillRoundRect(x_pos + 1, y_pos + 1, BATTERY_WIDTH - 2, BATTERY_HEIGHT - 2, 2, bg_color); 

        // 3. Calcular ancho
        int fill_width = (_charge_ * (BATTERY_WIDTH - 2)) / 100;

        // 4. Dibujar carga
        if (fill_width > 0) {
            uint16_t currentColor;
            
            if(_charge_ >= 80) {
                currentColor = COLOUR_FULL;
                is_low = false;
            } 
            else if(_charge_ > 20 && _charge_ < 80) { 
                currentColor = COLOUR_MID;
                is_low = false;
            } 
            else { 
                currentColor = COLOUR_LOW;
                is_low = true;
            }

            tft->fillRoundRect(x_pos + 1, y_pos + 1, fill_width, BATTERY_HEIGHT - 2, 2, currentColor);
        }
    }
};