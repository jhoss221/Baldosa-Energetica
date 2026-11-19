#include <WiFi.h>
#include "time.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ===============================
// DATOS DEL WIFI
// ===============================
const char* ssid = "ProyectoX";
const char* password = "87654321";

// ===============================
// PIN DEL MOSFET
// D25 = GPIO 25
// ===============================
const int pinMosfet = 25;

// ===============================
// PIN PARA MEDIR BATERIA
// ===============================
const int pinBateria = 34;

// Divisor de tension:
// 10k arriba + 10k abajo
const float factorDivisor = 2.0;

// ===============================
// PANTALLA OLED SSD1306
// ===============================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

// ===============================
// HORA DE ENCENDIDO
// CAMBIA SOLO ESTOS 3 VALORES
// ===============================
const int horaInicio = 18;
const int minutoInicio = 5;
const int duracionMinutos = 10;

// ===============================
// HORA DE PERU = UTC-5
// ===============================
const long gmtOffset_sec = -5 * 3600;
const int daylightOffset_sec = 0;

// ===============================
// LEER VOLTAJE DE BATERIA
// ===============================
float leerVoltajeBateria() {

  int lectura = analogRead(pinBateria);

  float voltajeADC = (lectura / 4095.0) * 3.3;

  float voltajeBateria = voltajeADC * factorDivisor;

  return voltajeBateria;
}

// ===============================
// CALCULAR PORCENTAJE APROXIMADO
// BATERIA Li-ion 3.7 V
// ===============================
int calcularPorcentaje(float voltaje) {

  if (voltaje >= 4.20) return 100;
  if (voltaje >= 4.10) return 90;
  if (voltaje >= 4.00) return 80;
  if (voltaje >= 3.90) return 70;
  if (voltaje >= 3.80) return 60;
  if (voltaje >= 3.70) return 50;
  if (voltaje >= 3.60) return 35;
  if (voltaje >= 3.50) return 20;
  if (voltaje >= 3.40) return 10;
  if (voltaje >= 3.30) return 5;

  return 0;
}

void setup() {

  Serial.begin(115200);

  // ===============================
  // MOSFET
  // ===============================
  pinMode(pinMosfet, OUTPUT);
  digitalWrite(pinMosfet, LOW);

  // ===============================
  // ADC BATERIA
  // ===============================
  pinMode(pinBateria, INPUT);

  // ===============================
  // OLED
  // SDA = GPIO21
  // SCL = GPIO22
  // ===============================
  Wire.begin(21, 22);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("No se encontro la pantalla OLED");
    while (true);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // ===============================
  // WIFI
  // ===============================
  WiFi.begin(ssid, password);

  Serial.print("Conectando al WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi conectado");

  // ===============================
  // HORA NTP
  // ===============================
  configTime(
    gmtOffset_sec,
    daylightOffset_sec,
    "pool.ntp.org",
    "time.nist.gov"
  );
}

void loop() {

  struct tm timeinfo;

  // ===============================
  // OBTENER HORA
  // ===============================
  if (!getLocalTime(&timeinfo)) {

    Serial.println("No se pudo obtener la hora");

    digitalWrite(pinMosfet, LOW);

    delay(1000);
    return;
  }

  int hora = timeinfo.tm_hour;
  int minuto = timeinfo.tm_min;
  int segundo = timeinfo.tm_sec;

  int minutosActuales = hora * 60 + minuto;
  int minutosInicio = horaInicio * 60 + minutoInicio;

  bool ledEncendido = false;

  // ===============================
  // CONTROL DEL LED
  // ===============================
  if (minutosActuales >= minutosInicio &&
      minutosActuales < minutosInicio + duracionMinutos) {

    digitalWrite(pinMosfet, HIGH);
    ledEncendido = true;

  } else {

    digitalWrite(pinMosfet, LOW);
    ledEncendido = false;
  }

  // ===============================
  // MEDIR BATERIA
  // ===============================
  float voltajeBateria = leerVoltajeBateria();

  int porcentaje = calcularPorcentaje(voltajeBateria);

  // ===============================
  // MONITOR SERIAL
  // ===============================
  Serial.printf(
    "Hora: %02d:%02d:%02d | Bateria: %.2f V | %d %% | LED: %s\n",
    hora,
    minuto,
    segundo,
    voltajeBateria,
    porcentaje,
    ledEncendido ? "ENCENDIDO" : "APAGADO"
  );

  // ===============================
  // MOSTRAR EN OLED
  // ===============================
  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("ESTADO BATERIA");

  display.setTextSize(2);
  display.setCursor(0, 15);
  display.print(voltajeBateria, 2);
  display.println(" V");

  display.setTextSize(2);
  display.setCursor(0, 38);
  display.print(porcentaje);
  display.println(" %");

  // Barra de carga
  display.drawRect(75, 42, 50, 15, SSD1306_WHITE);

  int anchoBarra = map(porcentaje, 0, 100, 0, 46);

  display.fillRect(
    77,
    44,
    anchoBarra,
    11,
    SSD1306_WHITE
  );

  display.display();

  delay(1000);
}
