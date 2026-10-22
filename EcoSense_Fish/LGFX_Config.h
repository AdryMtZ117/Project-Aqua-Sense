// TOUCH
#pragma once

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

class LGFX : public lgfx::LGFX_Device {

  lgfx::Panel_ST7789 panel;
  lgfx::Bus_SPI bus;
  lgfx::Touch_XPT2046 touch;

public:

  LGFX() {

    // SPI
    {
      auto cfg = bus.config();

      cfg.spi_host = SPI2_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = 40000000;
      cfg.freq_read = 16000000;
      cfg.use_lock = false;

      cfg.pin_sclk = 14;
      cfg.pin_mosi = 13;
      cfg.pin_miso = -1;
      cfg.pin_dc   = 2;

      bus.config(cfg);
      panel.setBus(&bus);
    }

    // PANEL
    {
      auto cfg = panel.config();

      cfg.pin_cs   = 15;
      cfg.pin_rst  = -1;
      cfg.pin_busy = -1;

      cfg.panel_width  = 240;
      cfg.panel_height = 320;

      panel.config(cfg);
    }

    // TOUCH
    {
      auto cfg = touch.config();

      cfg.x_min = 0;
      cfg.x_max = 239;
      cfg.y_min = 0;
      cfg.y_max = 319;

      cfg.pin_int = -1;
      cfg.bus_shared = true;

      cfg.offset_rotation = 0;

      cfg.spi_host = SPI2_HOST;
      cfg.freq = 1000000;

      cfg.pin_sclk = 18;
      cfg.pin_mosi = 23;
      cfg.pin_miso = 19;
      cfg.pin_cs   = 5;

      touch.config(cfg);
      panel.setTouch(&touch);
    }

    setPanel(&panel);
  }
};

extern LGFX lcd;