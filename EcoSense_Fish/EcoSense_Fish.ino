#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h> 

// 1. IMPORTAMOS TODOS LOS MÓDULOS
#include "ClimaAPI.h"
#include "controlBateria.h"
#include "PantallaPrincipal.h"
#include "PantallaCalendario.h"
#include "PantallaSensores.h" 
#include "PantallaGrafica.h"  
#include "PantallaAjustes.h"
#include "PantallaDetalleSensores.h"
#include "WiFi_Connection.h"
#include "RelojAPI.h"
#include "SubidaDatosAPI.h"   // <--- MÓDULO DE DOCKER WEB
#include "PantallaSplash.h"   // <--- AGREGAMOS TU LOGO
#include <BluetoothSerial.h>

BluetoothSerial SerialBT;


// --- CONFIGURACIÓN DEL TOUCH (CYD) ---
#define XPT2046_IRQ 36  
#define XPT2046_MOSI 32 
#define XPT2046_MISO 39 
#define XPT2046_CLK 25  
#define XPT2046_CS 33   

SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);
// -------------------------------------

TFT_eSPI tft = TFT_eSPI();

#define TFT_BL 21 
#define C_FONDO 0x0841 

// 2. MÁQUINA DE ESTADOS
enum EstadoPantalla {
  PRINCIPAL,
  CALENDARIO,
  SENSORES,   
  GRAFICA,    
  AJUSTES,
  SENSOR_1,
  SENSOR_2,
  SENSOR_3,
  SENSOR_4,
  SENSOR_5,
  SENSOR_6,
  WIFI_CONFIG
};

EstadoPantalla pantallaActual = PRINCIPAL;
ControlGraficoBateria bateria(230, 12, &tft, 0xB71C); // Fondo Azul Cielo

unsigned long ultimoToque = 0; 
unsigned long tiempoUltimoClima = 0;
unsigned long tiempoUltimaSubida = 0; // Timer para envio de datos
const long intervaloClima = 600000; // 10 minutos
const long intervaloSubida = 120000; // 2 minutos (modificable)
bool wifiIniciado = false;          // Bandera de carga en segundo plano

bool wifiHabilitado = true;         // Controla si el UI de WiFi está activo

// --- NUEVAS VARIABLES: BLUETOOTH Y BRILLO ---
bool bluetoothHabilitado = false;
int nivelBrillo = 100; // Va de 0 a 140 (ancho del slider en pixeles)

// Le avisamos al compilador que esta función existe más abajo
void cambiarPantalla(EstadoPantalla nuevaPantalla);

void setup() {
  Serial.begin(4800);
  
  // Inicializar puertos Serial extra para abarcar todas las variantes del CYD
  // NOTA: No usamos el 21 porque es el brillo de la pantalla.
  Serial1.begin(4800, SERIAL_8N1, 22, 27); // RX=22 (Conector CN1 en algunas placas)
  Serial2.begin(4800, SERIAL_8N1, 35, 32); // RX=35 (Conector P3 cerca de la SD)
  
  // Timeout muy corto (50ms) para que si entra "basura" por un pin sin conectar, no congele la pantalla.
  Serial.setTimeout(50);
  Serial1.setTimeout(50);
  Serial2.setTimeout(50);
  
  // Iniciar Touch 
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  touchscreen.setRotation(1); 

  // Iniciar Pantalla
  tft.init();
  tft.setRotation(1);      
  tft.setSwapBytes(true);  
  tft.invertDisplay(1);
  
  // --- CONFIGURACIÓN DE BRILLO (PWM) ---
  ledcAttach(TFT_BL, 5000, 8);
  ledcWrite(TFT_BL, map(nivelBrillo, 0, 140, 10, 255));

  // --- 1. MOSTRAR EL LOGO DE INICIO (SPLASH SCREEN) ---
  dibujarSplash(&tft);
  
  // --- 2. ESPERAR 4 SEGUNDOS ---
  delay(4000); 

  // --- 3. DIBUJAR LA INTERFAZ PRINCIPAL ---
  cambiarPantalla(PRINCIPAL);

  // --- 4. LANZAR CONEXIONES EN SEGUNDO PLANO ---
  conectarWiFi();    
}

void loop() {
  // --- DETECTOR DE CONEXIÓN EN SEGUNDO PLANO ---
  if (wifiHabilitado && !wifiIniciado && WiFi.status() == WL_CONNECTED) {
    wifiIniciado = true;
    Serial.println("¡WiFi Conectado! Actualizando interfaz...");
    
    obtenerHoraAPI();
    actualizarClima(); // Descargamos los datos reales
    
    if (pantallaActual == PRINCIPAL) cambiarPantalla(PRINCIPAL); 
  }

  // --- LECTURA DE SENSORES DESDE ARDUINO UNO POR SERIAL ---
  String data = "";
  
  // Leer de los puertos disponibles y validar que la cadena contenga "DATA,"
  if (Serial.available() > 0) {
    String tempD = Serial.readStringUntil('\n');
    if (tempD.indexOf("DATA,") >= 0) data = tempD;
  } 
  if (Serial1.available() > 0) {
    String tempD = Serial1.readStringUntil('\n');
    if (tempD.indexOf("DATA,") >= 0) data = tempD;
  }
  if (Serial2.available() > 0) {
    String tempD = Serial2.readStringUntil('\n');
    if (tempD.indexOf("DATA,") >= 0) data = tempD;
  }

  if (data.length() > 0) {
    // --------------------------------

    data.trim();
    // Usamos indexOf en lugar de startsWith para ser tolerantes a caracteres basura al inicio
    int dataIndex = data.indexOf("DATA,");
    if (dataIndex >= 0) {
      // Nuevo formato: DATA,TempAire,pH,Gas,TempAgua,NivelAgua,TurbidezVolt
      int c1 = data.indexOf(',', dataIndex + 5);
      int c2 = data.indexOf(',', c1 + 1);
      int c3 = data.indexOf(',', c2 + 1);
      int c4 = data.indexOf(',', c3 + 1);
      int c5 = data.indexOf(',', c4 + 1);
      
      if (c1 > 0 && c2 > 0 && c3 > 0) {
        valor_sensor_1 = data.substring(dataIndex + 5, c1).toFloat(); // Temp Aire
        valor_sensor_2 = data.substring(c1 + 1, c2).toFloat();        // pH
        valor_sensor_3 = data.substring(c2 + 1, c3 > 0 ? c3 : data.length()).toFloat(); // Gas
        if (c3 > 0 && c4 > 0) valor_sensor_4 = data.substring(c3 + 1, c4).toFloat(); // Temp Agua
        if (c4 > 0 && c5 > 0) valor_sensor_5 = data.substring(c4 + 1, c5).toFloat(); // Nivel Agua
        if (c5 > 0)           valor_sensor_6 = data.substring(c5 + 1).toFloat();      // Turbidez

        // Actualizar en tiempo real si estamos en la pantalla de ese sensor específico
        if      (pantallaActual == SENSOR_1) dibujarPantallaSensor1(&tft);
        else if (pantallaActual == SENSOR_2) dibujarPantallaSensor2(&tft);
        else if (pantallaActual == SENSOR_3) dibujarPantallaSensor3(&tft);
        else if (pantallaActual == SENSOR_4) dibujarPantallaSensor4(&tft);
        else if (pantallaActual == SENSOR_5) dibujarPantallaSensor5(&tft);
        else if (pantallaActual == SENSOR_6) dibujarPantallaSensor6(&tft);
      }
    }
  }

  // --- TEMPORIZADOR DEL CLIMA ---
  if (wifiIniciado && (millis() - tiempoUltimoClima > intervaloClima)) {
    tiempoUltimoClima = millis();
    actualizarClima();
    if (pantallaActual == PRINCIPAL) cambiarPantalla(PRINCIPAL); 
  }

  // --- TEMPORIZADOR SUBIDA DE DATOS AL REPOSITORIO DOCKER ---
  if (wifiIniciado && (millis() - tiempoUltimaSubida > intervaloSubida)) {
    tiempoUltimaSubida = millis();
    subirDatosSensoresServidor();
  }

  // --- RELOJ EN TIEMPO REAL ---
  if (actualizarReloj() && pantallaActual == PRINCIPAL) {
    tft.fillRect(10, 10, 80, 20, 0xB71C); // Parche azul cielo
    tft.setTextColor(0x4A49);
    tft.drawString(horaActual, 15, 12, 2);
  }

  // --- LÓGICA DEL TOUCH ---
  if (touchscreen.tirqTouched() && touchscreen.touched() && (millis() - ultimoToque > 300)) {
    ultimoToque = millis();
    
    TS_Point p = touchscreen.getPoint();
    int x = map(p.x, 200, 3700, 1, 320);
    int y = map(p.y, 240, 3800, 1, 240);

    Serial.print("Tocado -> X: "); Serial.print(x);
    Serial.print(" | Y: "); Serial.println(y);

    // --- NAVEGACIÓN ---
    switch (pantallaActual) {
      case PRINCIPAL:
        if (x > 265) {
          if (y > 60 && y < 100) { 
            cambiarPantalla(CALENDARIO);
          }
          else if (y > 105 && y < 145) {
            cambiarPantalla(SENSORES);
          }
          else if (y > 150 && y < 190) {
            cambiarPantalla(GRAFICA);
          }
          else if (y > 195 && y < 235) {
            cambiarPantalla(AJUSTES);
          }
        }
        break;

      case CALENDARIO:
      case GRAFICA:
        if (x < 60 && y < 60) {
          cambiarPantalla(PRINCIPAL);
        }
        break;
        
      case AJUSTES:
        if (x < 60 && y < 60) {
          cambiarPantalla(PRINCIPAL);
        } else if (x > 110 && x < 165 && y > 75 && y < 105) { 
          // Toggle del switch WiFi (ahora a la izquierda)
          wifiHabilitado = !wifiHabilitado;
          
          if (!wifiHabilitado) {
            WiFi.disconnect(true, true);
            WiFi.mode(WIFI_OFF);
            wifiIniciado = false;
            Serial.println("WiFi Apagado (Ahorro de energía)");
          } else {
            WiFi.mode(WIFI_STA);
            WiFi.begin(); // Utiliza las credenciales guardadas
            Serial.println("WiFi Encendido, intentando conectar...");
          }

          // Redibujamos la pantalla de ajustes para mostrar u ocultar
          cambiarPantalla(AJUSTES);
        } else if (wifiHabilitado && x > 175 && x < 280 && y > 75 && y < 105) {
          // Botón Configurar (ahora a la derecha)
          cambiarPantalla(WIFI_CONFIG);
        } else if (x > 215 && x < 270 && y > 120 && y < 160) {
          // Toggle Bluetooth
          bluetoothHabilitado = !bluetoothHabilitado;
          if (bluetoothHabilitado) {
            SerialBT.begin("ESP32 Amarillo"); // Encender Bluetooth con el nombre deseado
            Serial.println("Bluetooth Encendido: ESP32 Amarillo");
          } else {
            SerialBT.end(); // Apagar para ahorrar energía
            Serial.println("Bluetooth Apagado");
          }
          cambiarPantalla(AJUSTES);

        } else if (x > 115 && x < 265 && y > 175 && y < 210) {
          // Slider de Brillo
          nivelBrillo = x - 120;
          if (nivelBrillo < 0) nivelBrillo = 0;
          if (nivelBrillo > 140) nivelBrillo = 140;
          ledcWrite(TFT_BL, map(nivelBrillo, 0, 140, 10, 255));
          cambiarPantalla(AJUSTES);
        }
        break;

      case WIFI_CONFIG:
        if (procesarTouchWiFi(&tft, x, y)) {
           cambiarPantalla(AJUSTES);
        }
        break;

      case SENSORES:
        if (x < 60 && y < 60) {
          cambiarPantalla(PRINCIPAL);
        } else if (x > 50 && x < 270) {
          // 6 botones: startY=50, btnHeight=27, gap=5 => cada slot ocupa 32px
          if      (y >= 50  && y < 77)  cambiarPantalla(SENSOR_1);
          else if (y >= 82  && y < 109) cambiarPantalla(SENSOR_2);
          else if (y >= 114 && y < 141) cambiarPantalla(SENSOR_3);
          else if (y >= 146 && y < 173) cambiarPantalla(SENSOR_4);
          else if (y >= 178 && y < 205) cambiarPantalla(SENSOR_5);
          else if (y >= 210 && y < 237) cambiarPantalla(SENSOR_6);
        }
        break;

      case SENSOR_1:
      case SENSOR_2:
      case SENSOR_3:
      case SENSOR_4:
      case SENSOR_5:
      case SENSOR_6:
        if (x < 60 && y < 60) {
          cambiarPantalla(SENSORES);
        }
        break;
    }
  } // <--- ESTA LLAVE FALTABA (Cierra el if del touch)

  // Animación de batería solo en principal
  if (pantallaActual == PRINCIPAL) {
    bateria.Paint(85); 
  }
} // <--- ESTA LLAVE TAMBIÉN FALTABA (Cierra el loop)

// 3. CONTROLADOR DE DIBUJO
void cambiarPantalla(EstadoPantalla nuevaPantalla) {
  pantallaActual = nuevaPantalla;
  tft.setTextSize(1); // Restablecer tamaño de texto por defecto para evitar texto gigante
  tft.setTextDatum(TL_DATUM); // Restablecer justificación a la izquierda (por defecto)
  tft.fillScreen(C_FONDO); 

  switch (pantallaActual) {
    case PRINCIPAL:
      dibujarPantallaPrincipal(&tft); 
      bateria.Paint(-1); 
      break;
    case CALENDARIO:
      dibujarPantallaCalendario(&tft);
      break;
    case SENSORES:
      dibujarPantallaSensores(&tft); 
      break;
    case SENSOR_1:
      dibujarPantallaSensor1(&tft);
      break;
    case SENSOR_2:
      dibujarPantallaSensor2(&tft);
      break;
    case SENSOR_3:
      dibujarPantallaSensor3(&tft);
      break;
    case SENSOR_4:
      dibujarPantallaSensor4(&tft);
      break;
    case SENSOR_5:
      dibujarPantallaSensor5(&tft);
      break;
    case SENSOR_6:
      dibujarPantallaSensor6(&tft);
      break;
    case GRAFICA:
      dibujarPantallaGrafica(&tft);  
      break;
    case AJUSTES:
      dibujarPantallaAjustes(&tft);
      break;
    case WIFI_CONFIG:
      iniciarConfiguracionWiFi(&tft);
      break;
  }
}