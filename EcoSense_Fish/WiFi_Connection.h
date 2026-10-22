#pragma once
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <Preferences.h>

// Preferencias para guardar WiFi
Preferences prefsWifi;

String wifi_ssid = "";
String wifi_password = "";
String inputTextoWifi = "";

bool mostrarPasswordWifi = false;
bool escribiendoSSIDWifi = true;
bool mayusculasWifi = false;

int pantallaWifi = 0;
// Pantallas:
// 0 = Menú principal
// 1 = QR
// 2 = Teclado
// 3 = Escaneo de Red
// 4 = Conexión

String redesWiFiDescubiertas[20];
int cantidadRedesDescubiertas = 0;

bool touchEnWifi(int x, int y, int w, int h, uint16_t tx, uint16_t ty){
  return (tx>=x && tx<=x+w && ty>=y && ty<=y+h);
}

void botonWifi(TFT_eSPI* tft, int x, int y, int w, int h, String txt, uint16_t colorFoco = 0xFFFF, uint16_t colorFondo = 0x52AA){
  tft->fillRoundRect(x, y, w, h, 8, colorFondo); // Opcional rellenar
  tft->drawRoundRect(x, y, w, h, 8, colorFoco);
  tft->setTextColor(colorFoco, colorFondo);
  tft->setTextDatum(MC_DATUM);
  tft->drawString(txt, x+w/2, y+h/2);
}

// ------ DECLARACIONES ANTICIPADAS ------
void pantallaMenuWifi(TFT_eSPI* tft);
void mostrarQRWifi(TFT_eSPI* tft);
void escanearRedesWifi(TFT_eSPI* tft);
void pantallaTecladoWifi(TFT_eSPI* tft);
void conectarRedWiFi(TFT_eSPI* tft);

// ==========================================

void pantallaMenuWifi(TFT_eSPI* tft){
  pantallaWifi = 0;
  tft->fillScreen(0x0841); // C_FONDO

  // Boton para Regresar a Ajustes
  tft->fillRoundRect(10, 10, 40, 30, 5, 0x52AA);
  tft->setTextColor(0xFFFF);
  tft->drawCentreString("<", 30, 16, 2);

  // Título
  tft->setTextColor(0xFD20); // C_SOL
  tft->drawCentreString("CONFIG WIFI", 160, 15, 4);

  // Estado de la conexion
  tft->setTextColor(0x07FF); // C_CYAN
  if (WiFi.status() == WL_CONNECTED) {
    tft->drawCentreString("Conectado a:", 160, 43, 2);
    tft->drawCentreString(WiFi.SSID(), 160, 58, 2);
  } else {
    tft->drawCentreString("Desconectado", 160, 50, 2);
  }

  botonWifi(tft, 60, 80, 200, 35, "QR");
  botonWifi(tft, 60, 125, 200, 35, "Escanear Redes");
  botonWifi(tft, 60, 170, 200, 35, "Conexion Manual");
}

void mostrarQRWifi(TFT_eSPI* tft){
  pantallaWifi = 1;
  tft->fillScreen(0x0841);
  tft->setTextColor(TFT_WHITE, TFT_BLACK);
  tft->setTextDatum(MC_DATUM);
  tft->drawString("QR Config", 160, 20);

  // QR Placeholder
  tft->fillRect(90, 50, 140, 140, TFT_WHITE);
  tft->fillRect(105, 65, 20, 20, TFT_BLACK);
  tft->fillRect(195, 65, 20, 20, TFT_BLACK);
  tft->fillRect(105, 155, 20, 20, TFT_BLACK);

  botonWifi(tft, 100, 210, 120, 25, "<--| Volver");
}

void escanearRedesWifi(TFT_eSPI* tft){
  pantallaWifi = 3;
  tft->fillScreen(0x0841);
  tft->setTextColor(TFT_WHITE, TFT_BLACK);
  tft->setTextDatum(MC_DATUM);
  tft->drawString("Escaneando...", 160, 120);

  // PREPARACIÓN DEL MÓDULO (Crítico para que el escáner funcione)
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  cantidadRedesDescubiertas = WiFi.scanNetworks();
  
  tft->fillScreen(0x0841);
  tft->drawString("Selecciona Red", 160, 20);

  if (cantidadRedesDescubiertas <= 0) {
    tft->drawString("No se encontraron redes", 160, 120);
    cantidadRedesDescubiertas = 0;
  } else {
    if(cantidadRedesDescubiertas > 5) cantidadRedesDescubiertas = 5; // Limite de 5 por pantalla

    for(int i=0; i<cantidadRedesDescubiertas; i++){
      redesWiFiDescubiertas[i] = WiFi.SSID(i);
      botonWifi(tft, 20, 50+i*30, 280, 25, redesWiFiDescubiertas[i]);
    }
  }
  
  botonWifi(tft, 20, 210, 120, 25, "<--| Volver");
}

String tecladoLetrasWifi[4][10] = {
  {"1","2","3","4","5","6","7","8","9","0"},
  {"q","w","e","r","t","y","u","i","o","p"},
  {"a","s","d","f","g","h","j","k","l","<-"},
  {"SHIFT","z","x","c","v","b","n","m","ESP","OK"}
};

void pantallaTecladoWifi(TFT_eSPI* tft){
  pantallaWifi = 2;
  tft->fillScreen(0x0841);
  tft->setTextColor(TFT_WHITE, TFT_BLACK);
  tft->setTextDatum(TL_DATUM);
  tft->setTextSize(1);
  
  tft->drawString(escribiendoSSIDWifi ? "SSID:" : "PASSWORD:", 10, 8);
  
  String visible = inputTextoWifi;
  if(!escribiendoSSIDWifi && !mostrarPasswordWifi){
    visible = "";
    for(int i=0; i<inputTextoWifi.length(); i++) visible += "*";
  }

  tft->drawString(visible, 10, 22);

  botonWifi(tft, 250, 5, 60, 25, "Volver");

  if(!escribiendoSSIDWifi){
    botonWifi(tft, 180, 5, 60, 25, mostrarPasswordWifi ? "OCULT" : "VER", TFT_CYAN);
  }

  int y = 55;
  for(int fila=0; fila<3; fila++){
    int x = 6;
    for(int col=0; col<10; col++){
      String key = tecladoLetrasWifi[fila][col];
      int w = 28;
      int h = 32;

      String mostrar = key;
      if(mayusculasWifi && mostrar.length()==1 && isAlpha(mostrar[0])) {
         mostrar.toUpperCase();
      }
      botonWifi(tft, x, y, w, h, mostrar);
      x += 30;
    }
    y += 36;
  }
  
  // Ultima fila
  int baseY = 163;
  String filaExtra[3] = {"z","x","c"};
  for(int i=0; i<3; i++){
    int x = 30 + i*30;
    String mostrar = filaExtra[i];
    if(mayusculasWifi) mostrar.toUpperCase();
    botonWifi(tft, x, baseY, 28, 32, mostrar);
  }
  String filaExtra2[5] = {"v", "b", "n", "m", "<-"};
  for(int i=0; i<5; i++){
    int x = 130 + i*30;
    String mostrar = filaExtra2[i];
    if(mayusculasWifi && mostrar.length()==1) mostrar.toUpperCase();
    botonWifi(tft, x, baseY, 28, 32, mostrar);
  }
  
  // Teclas Especiales
  int specialY = 203;
  botonWifi(tft, 10, specialY, 70, 30, "SHIFT", TFT_CYAN);
  botonWifi(tft, 90, specialY, 100, 30, "ESP", TFT_CYAN);
  botonWifi(tft, 200, specialY, 50, 30, "OK", TFT_GREEN);
}

void conectarRedWiFi(TFT_eSPI* tft){
  pantallaWifi = 4;
  tft->fillScreen(0x0841);
  tft->setTextColor(TFT_WHITE, TFT_BLACK);
  tft->setTextDatum(MC_DATUM);
  tft->drawString("Conectando...", 160, 120);
  
  WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());

  int t=0;
  while(WiFi.status() != WL_CONNECTED && t<20){
    delay(500);
    t++;
  }

  if(WiFi.status() == WL_CONNECTED){
    prefsWifi.begin("wifi", false);
    prefsWifi.putString("ssid", wifi_ssid);
    prefsWifi.putString("pass", wifi_password);
    prefsWifi.end();

    tft->fillScreen(0x0841);
    tft->drawString("Conectado", 160, 100);
    tft->drawString(WiFi.localIP().toString(), 160, 140);
    delay(2000); // mostrar por 2 segundos y volver exitosamente
  }else {
    tft->fillScreen(0x0841);
    tft->drawString("Conexion Fallida", 160, 120);
    delay(1500);
  }
  pantallaMenuWifi(tft);
}

// Retorna `true` SI SE DEBE SALIR del submodulo hacia la Pantalla Ajustes Principal
bool procesarTouchWiFi(TFT_eSPI* tft, int tx, int ty) {
  
  if(pantallaWifi == 0){
    // Salir del menú Wi-Fi completo (Botón de atrás)
    if(touchEnWifi(10, 10, 40, 30, tx, ty)) return true;

    if(touchEnWifi(60, 80, 200, 35, tx, ty)) mostrarQRWifi(tft);
    else if(touchEnWifi(60, 125, 200, 35, tx, ty)) escanearRedesWifi(tft);
    else if(touchEnWifi(60, 170, 200, 35, tx, ty)){
      escribiendoSSIDWifi = true;
      inputTextoWifi = "";
      pantallaTecladoWifi(tft);
    }
    return false;
  }
  
  if(pantallaWifi == 1){
    if(touchEnWifi(100, 210, 120, 25, tx, ty)) pantallaMenuWifi(tft);
    return false;
  }
  
  if(pantallaWifi == 3){
    for(int i=0; i<cantidadRedesDescubiertas; i++){
      if(touchEnWifi(20, 50+i*30, 280, 25, tx, ty)){
        wifi_ssid = redesWiFiDescubiertas[i];
        escribiendoSSIDWifi = false;
        inputTextoWifi = "";
        pantallaTecladoWifi(tft);
        return false;
      }
    }
    if(touchEnWifi(20, 210, 120, 25, tx, ty)) pantallaMenuWifi(tft);
    return false;
  }
  
  if(pantallaWifi == 2){
    // Salir del teclado
    if(touchEnWifi(250, 5, 60, 25, tx, ty)){
      pantallaMenuWifi(tft);
      return false;
    }
    
    // Ocultar / Mostrar Pass
    if(!escribiendoSSIDWifi && touchEnWifi(180, 5, 60, 25, tx, ty)){
      mostrarPasswordWifi = !mostrarPasswordWifi;
      pantallaTecladoWifi(tft);
      return false;
    }

    // Teclas 3 primeras filas
    int y=55;
    for(int fila=0; fila<3; fila++){
      int x=6;
      for(int col=0; col<10; col++){
        String key = tecladoLetrasWifi[fila][col];
        if(touchEnWifi(x,y,28,35, tx, ty)){
          if(key == "<-"){
            if(inputTextoWifi.length()>0) inputTextoWifi.remove(inputTextoWifi.length()-1);
          }else{
            if(mayusculasWifi) key.toUpperCase();
            inputTextoWifi += key;
          }
          pantallaTecladoWifi(tft);
          return false;
        }
        x += 30;
      }
      y += 36;
    }

    // Fila 4 Letras adicionales
    String filaExtra[8] = {"z", "x", "c", "v", "b", "n", "m", "<-"};
    for(int i=0; i<8; i++){
      int x = (i < 3) ? (30 + i*30) : (130 + (i-3)*30);
      if(touchEnWifi(x, 163, 28, 32, tx, ty)){
        String key = filaExtra[i];
        if(key == "<-"){
          if(inputTextoWifi.length()>0) inputTextoWifi.remove(inputTextoWifi.length()-1);
        }else{
          if(mayusculasWifi) key.toUpperCase();
          inputTextoWifi += key;
        }
        pantallaTecladoWifi(tft);
        return false;
      }
    }

    // Especiales
    if(touchEnWifi(10, 203, 70, 30, tx, ty)){
      mayusculasWifi = !mayusculasWifi;
      pantallaTecladoWifi(tft);
      return false;
    }
    if(touchEnWifi(90, 203, 100, 30, tx, ty)){
      inputTextoWifi += " ";
      pantallaTecladoWifi(tft);
      return false;
    }
    if(touchEnWifi(200, 203, 50, 30, tx, ty)){
      if(escribiendoSSIDWifi){
        wifi_ssid = inputTextoWifi;
        inputTextoWifi = "";
        escribiendoSSIDWifi = false;
        pantallaTecladoWifi(tft);
      }else{
        wifi_password = inputTextoWifi;
        conectarRedWiFi(tft);
      }
      return false;
    }
  }

  return false;
}

// Llama a esto cuando entras desde "Ajustes > Config"
void iniciarConfiguracionWiFi(TFT_eSPI* tft) {
  pantallaMenuWifi(tft);
}