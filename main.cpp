#include <Arduino.h>
#include <ESP32SPISlave.h>
#include <Wire.h>
#include <LiquidCrystal.h>

// PARTE 2 - LEDs
// =====================================================

#define LED_ROJO  17
#define LED_VERDE 16
#define LED_AZUL  4

// PARTE 3 - POTENCIOMETRO
// =====================================================

#define POT 34

// I2C
// =====================================================

#define I2C_DEV_ADDR 0x32

// =====================================================
// SPI VSPI
// ESP32:
// SCK  = GPIO18
// MISO = GPIO19
// MOSI = GPIO23
// CS   = GPIO5
// =====================================================

#define SPI_SCK  18
#define SPI_MISO 19
#define SPI_MOSI 23
#define SPI_CS   5

#define QUEUE_SIZE 1

ESP32SPISlave slave;

uint8_t spi_tx[3] = {0, 0, 0};
uint8_t spi_rx[3] = {0, 0, 0};

// LCD 16x2 - MODO 8 BITS
// =====================================================

#define RS 2 ****
#define E  15

#define LCD_D0 32
#define LCD_D1 33
#define LCD_D2 25
#define LCD_D3 26
#define LCD_D4 27
#define LCD_D5 14
#define LCD_D6 12
#define LCD_D7 13

LiquidCrystal lcd(RS, E, LCD_D0, LCD_D1, LCD_D2, LCD_D3, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

// =====================================================
// VARIABLES GENERALES
// =====================================================

volatile uint16_t sensorADCraw = 0;
volatile uint8_t sensorADC = 0;
volatile uint8_t comandoI2C = 0;

char ultimoLED = '-';

bool ledActivo = false;

unsigned long tiempoInicioLED = 0;
unsigned long tiempoLED = 0;

int ledActual = -1;

unsigned long tiempoLCD = 0;
unsigned long tiempoADC = 0;

// =====================================================
// PROTOTIPOS
// =====================================================

void apagarLEDs();
void activarLED(uint8_t led, unsigned long tiempo);
void actualizarLED();
void actualizarLCD();

void onReceive(int len);
void onRequest();

// =====================================================
// APAGAR LEDs
// =====================================================

void apagarLEDs()
{
  digitalWrite(LED_ROJO, LOW);
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_AZUL, LOW);
}

// =====================================================
// ACTIVAR LED
// =====================================================

void activarLED(uint8_t led, unsigned long tiempo)
{
  apagarLEDs();

  ledActual = led;
  tiempoLED = tiempo;
  tiempoInicioLED = millis();
  ledActivo = true;

  if (led == 1)
  {
    digitalWrite(LED_ROJO, HIGH);
    ultimoLED = 'R';

    Serial.println("LED ROJO -> ON");
  }

  else if (led == 2)
  {
    digitalWrite(LED_VERDE, HIGH);
    ultimoLED = 'G';

    Serial.println("LED VERDE -> ON");
  }

  else if (led == 3)
  {
    digitalWrite(LED_AZUL, HIGH);
    ultimoLED = 'B';

    Serial.println("LED AZUL -> ON");
  }
}

// =====================================================
// ACTUALIZAR ESTADO DEL LED
// =====================================================

void actualizarLED()
{
  if (!ledActivo)
    return;

  if (millis() - tiempoInicioLED >= tiempoLED)
  {
    apagarLEDs();

    ledActivo = false;
    ledActual = -1;

    Serial.println("LED -> OFF");
    Serial.println("Operacion SPI terminada");
    Serial.println();
  }
}

// =====================================================
// ACTUALIZAR LCD
// =====================================================

void actualizarLCD()
{
  float voltaje = sensorADCraw * (3.3 / 4095.0);

  // ===================================================
  // FILA 1
  // Volt   ADC   LED
  // ===================================================

  lcd.setCursor(0, 0);
  lcd.print("Volt");

  lcd.setCursor(7, 0);
  lcd.print("ADC");

  lcd.setCursor(13, 0);
  lcd.print("LED");

  // ===================================================
  // FILA 2
  // ===================================================

  // Voltaje
  lcd.setCursor(0, 1);
  lcd.print("     ");

  lcd.setCursor(0, 1);
  lcd.print(voltaje, 2);

  // ADC
  lcd.setCursor(7, 1);
  lcd.print("    ");

  lcd.setCursor(7, 1);
  lcd.print(sensorADC);

  // Ultimo LED
  lcd.setCursor(14, 1);
  lcd.print(" ");

  lcd.setCursor(14, 1);
  lcd.print(ultimoLED);
}

// =====================================================
// SETUP
// =====================================================

void setup()
{
  Serial.begin(115200);

  // ===================================================
  // ADC
  // ===================================================

  pinMode(POT, INPUT);

  // ESP32 ADC = 12 bits
  analogReadResolution(12);

  // ===================================================
  // LEDs
  // ===================================================

  pinMode(LED_ROJO, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_AZUL, OUTPUT);

  apagarLEDs();

  // ===================================================
  // LCD
  // ===================================================

  lcd.begin(16, 2);
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Iniciando...");

  // ===================================================
  // SPI
  // ===================================================

  slave.setDataMode(SPI_MODE0);
  slave.setQueueSize(QUEUE_SIZE);

  slave.begin(
    VSPI,
    SPI_SCK,
    SPI_MISO,
    SPI_MOSI,
    SPI_CS
  );

  Serial.println();
  Serial.println("=============================");
  Serial.println("ESP32 - PROYECTO 2");
  Serial.println("=============================");
  Serial.println();

  Serial.println("SPI SLAVE configurado:");
  Serial.println("SCK  = GPIO18");
  Serial.println("MISO = GPIO19");
  Serial.println("MOSI = GPIO23");
  Serial.println("CS   = GPIO5");

  Serial.println();

  // ===================================================
  // I2C
  // ===================================================

  Wire.onReceive(onReceive);
  Wire.onRequest(onRequest);

  Wire.begin((uint8_t)I2C_DEV_ADDR);

  Serial.print("I2C esclavo listo. Direccion: 0x");
  Serial.println(I2C_DEV_ADDR, HEX);

  delay(1000);

  lcd.clear();

  // Primera lectura
  sensorADCraw = analogRead(POT);
  sensorADC = map(sensorADCraw, 0, 4095, 0, 255);

  actualizarLCD();
}

// =====================================================
// LOOP
// =====================================================

void loop()
{
  // ===================================================
  // ACTUALIZAR ADC
  // ===================================================

  if (millis() - tiempoADC >= 20)
  {
    tiempoADC = millis();

    sensorADCraw = analogRead(POT);
    sensorADC = map(sensorADCraw, 0, 4095, 0, 255);
  }

  // ===================================================
  // PARTE 2 - SPI
  // ===================================================

  /*
   * La NUCLEO envia:
   *
   * Byte 0 = LED
   *          1 = rojo
   *          2 = verde
   *          3 = azul
   *
   * Byte 1 = MSB tiempo
   * Byte 2 = LSB tiempo
   */

  if (slave.hasTransactionsCompletedAndAllResultsHandled())
  {
    slave.queue(spi_tx, spi_rx, 3);
    slave.trigger();
  }

  if (slave.hasTransactionsCompletedAndAllResultsReady(QUEUE_SIZE))
  {
    // Marcar resultado como manejado
    slave.numBytesReceivedAll();

    uint8_t ledRecibido = spi_rx[0];

    uint16_t tiempoRecibido =
      ((uint16_t)spi_rx[1] << 8) |
      spi_rx[2];

    Serial.println();
    Serial.println("=============================");
    Serial.println("COMANDO SPI RECIBIDO");
    Serial.println("=============================");

    Serial.print("LED: ");
    Serial.println(ledRecibido);

    Serial.print("MSB: ");
    Serial.println(spi_rx[1]);

    Serial.print("LSB: ");
    Serial.println(spi_rx[2]);

    Serial.print("Tiempo: ");
    Serial.print(tiempoRecibido);
    Serial.println(" ms");

    // Validar LED
    if (ledRecibido >= 1 && ledRecibido <= 3)
    {
      // Validar tiempo
      if (tiempoRecibido > 0)
      {
        Serial.println("Comando valido");

        activarLED(
          ledRecibido,
          tiempoRecibido
        );
      }

      else
      {
        Serial.println("ERROR: tiempo invalido");
      }
    }

    else
    {
      Serial.println("ERROR: LED invalido");
    }

    Serial.println();
  }

  // ===================================================
  // CONTROL NO BLOQUEANTE DEL LED
  // ===================================================

  actualizarLED();

  // ===================================================
  // PARTE 4 - LCD
  // ===================================================

  if (millis() - tiempoLCD >= 200)
  {
    tiempoLCD = millis();

    actualizarLCD();
  }
}

// =====================================================
// PARTE 3 - I2C
// NUCLEO SOLICITA ADC
// =====================================================

void onRequest()
{
  uint16_t valorADC = sensorADC;

  uint8_t datos[2];

  /*
   * STM32 espera:
   *
   * RX_Buffer[0] = LSB
   * RX_Buffer[1] = MSB
   *
   * adcValue =
   * RX_Buffer[0] |
   * (RX_Buffer[1] << 8)
   */

  datos[0] = valorADC & 0xFF;
  datos[1] = (valorADC >> 8) & 0xFF;

  Wire.write(datos, 2);

  Serial.print("ADC enviado por I2C: ");
  Serial.println(valorADC);
}

// =====================================================
// RECIBIR COMANDO I2C
// =====================================================

void onReceive(int len)
{
  while (Wire.available())
  {
    comandoI2C = Wire.read();

    Serial.print("Comando I2C recibido: ");

    if (comandoI2C >= 32 && comandoI2C <= 126)
    {
      Serial.write(comandoI2C);
      Serial.println();
    }

    else
    {
      Serial.println(comandoI2C);
    }
  }
}
