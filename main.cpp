

#include <Arduino.h>

#include <ESP32SPISlave.h>

#include "driver/gpio.h"
#include <stdint.h>
#include <LiquidCrystal.h>

#define ledrojo 32
#define ledverde 33
#define ledazul 25

#define POT 34

//Parte2
//#define QUEUE_SIZE 1

#define RS 15
#define E 4

#define LCD_D0 16
#define LCD_D1 17
#define LCD_D2 5
#define LCD_D3 18
#define LCD_D4 19
#define LCD_D5 21
#define LCD_D6 22
#define LCD_D7 23

LiquidCrystal lcd(RS, E, LCD_D0, LCD_D1, LCD_D2, LCD_D3, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

char ultimoLED = 'G';   // solo prueba por ahora

//Parte2
//uint8_t sensor;
//uint8_t data;
//ESP32SPISlave slave;


void setup() {
  Serial.begin(115200);

  pinMode(POT, INPUT);

  pinMode(ledrojo, OUTPUT);
  pinMode(ledverde, OUTPUT);
  pinMode(ledazul, OUTPUT);

  lcd.begin(16, 2);
  lcd.clear();

  //slave.setDataMode(SPI_MODE0);
  //slave.setQueueSize(QUEUE_SIZE);

  //slave.begin(VSPI);
}

void loop() {

    int valorADCraw = analogRead(POT);
    int valorADC = map(valorADCraw, 0, 4095, 0, 255);

    float voltaje = valorADCraw * (3.3 / 4095.0);


    // LCD - Titulos
    lcd.setCursor(0, 0);
    lcd.print("Volt");

    lcd.setCursor(7, 0);
    lcd.print("ADC");

    lcd.setCursor(13, 0);
    lcd.print("LED");


    // Valores

    lcd.setCursor(0, 1);
    lcd.print("     ");
    lcd.setCursor(0, 1);
    lcd.print(voltaje, 2);

    lcd.setCursor(7, 1);
    lcd.print("    ");
    lcd.setCursor(7, 1);
    lcd.print(valorADC);

    lcd.setCursor(14, 1);
    lcd.print(" ");
    lcd.setCursor(14, 1);
    lcd.print(ultimoLED);


    delay(200);
}