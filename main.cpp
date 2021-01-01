#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET     5 // Reset pin # (or -1 if sharing Arduino reset pin)
float temp;
float hum;
int buttonOLED_status = 1; // 1: temperatur, 2: humidity, 3: co2-ppm
const int buttonOLED = 6;
bool last_buttonOLED;
bool current_buttonOLED;
int buttonLED_status = 1; // 1: co2, 2: manuell, 3: OFF
const int buttonLED = 4;
bool last_buttonLED;
bool current_buttonLED;
int ledmosfetrot = 9;
int ledmosfetgruen = 10;
int ledmosfetblau = 11;
float potirot_prozent = 0;
float potigruen_prozent = 0;
float potiblau_prozent = 0;
int brightnessrot = 0; //pwm-Wert anhand des Potis
int brightnessgruen = 0;
int brightnessblau = 0;
int pwmrot = 0; // errechneter Wert für die CO2-Visualisierung
int potirot = A1;
int potigruen = A2;
int potiblau = A3;
int co2Sensor = A6;
int co2ppm = 0; // nimmt einen Wert zwischen 0 und 5000 an, 400 - 450 sind Normalwerte
unsigned long prev; // zeitvariable für die CO2-Messung alle intervall
unsigned long intervall = 3000;

Adafruit_BME280 bme;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


void setup() {
  Serial.begin(9600);
  delay(3000); //wartet 3 Sekunden, damit alles hochfahren kann
  Serial.println("Initialisiere das Programm... ");
  
  pinMode(co2Sensor, INPUT);
  pinMode(buttonOLED, INPUT);
  pinMode(buttonLED, INPUT);
  
  co2ppm = (5000*(pulseIn(co2Sensor, HIGH, 2500000)/1000))/1004;// misst einmalig CO2 beim starten des Geräts, später dann mit Funktion alle 5 Minuten
  prev = millis(); // startet den Timer für die C02-Messung

   unsigned status;
   status = bme.begin(0x76, &Wire); //initiiert I2C für den BME ---- warum steht hier nicht auch der OLED bzw. muss das überhaupt gemacht werden???

   if (!status) {
        Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
        Serial.print("SensorID was: 0x"); Serial.println(bme.sensorID(),16);
        Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
        Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
        Serial.print("        ID of 0x60 represents a BME 280.\n");
        Serial.print("        ID of 0x61 represents a BME 680.\n");
        while (1) delay(10);
    
    }
    
    Serial.println("-- Default Test --");

    Serial.println();
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
}

void co2messung(){ // ermittelt alle 5 Minuten den CO2-Wert und speichert ihn in co2ppm
    if (millis() - prev > intervall){
    co2ppm = (5000*(pulseIn(co2Sensor, HIGH, 2500000)/1000))/1004; // Erklaerung im extra-file
    }
}

void pwmrotberechnen(){
    if (co2ppm < 450){
        pwmrot = 0;
    }
    else if (co2ppm > 2000) {
        pwmrot = 255;
    }
    else {
        pwmrot = (255*(co2ppm-450))/1550;
    }
}

void temperatur() { // Anzeige von temperatur auf Display
  temp = bme.readTemperature();
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setRotation(2); // drehen, weil ich ihn verkehrt herum eingeklebt habe
  display.setCursor(0,0);
  display.println(F("Temperatur"));
  display.print(temp);
  display.print(" ");
  display.print(char(247));
  display.println("C");
  display.display();
}

void humidity() { // Anzeige von humidity auf Display
   hum = bme.readHumidity();
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setRotation(2); // drehen, weil ich ihn verkehrt herum eingeklebt habe
  display.setCursor(0,0);
  display.println(F("Humidity"));
  display.setTextSize(2);
  display.print(hum);
  display.println(" %");
  display.display();
}

void co2() { // Anzeige von co2 ppm auf Display
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setRotation(2); // drehen, weil ich ihn verkehrt herum eingeklebt habe
  display.setCursor(0,0);
  display.println(F("CO2-Gehalt"));
  display.setTextSize(2);
  display.print(co2ppm);
  display.println(" ppm");
  display.display();
}

void oledanzeige(){ // sorgt für die Anzeige des richtigen Wertes auf dem Display
    if (buttonOLED_status == 1){
  temperatur();
  Serial.println("oled würde jetzt temperatur anzeigen");
 }
 if (buttonOLED_status == 2){
  humidity();
  Serial.println("oled würde jetzt humidity anzeigen");
 }
  if (buttonOLED_status == 3){
  co2();
  Serial.println("oled würde jetzt co2 anzeigen");
 }
}

void LEDanzeige(){ // sorgte für eine Farbanzeige der LED-Kette und wechselt zwischen den Modi CO2/Manuell/OFF
    if (buttonLED_status == 1){ // co2 anzeigen, 450 - 1500 ppm gruen weniger und rot mehr werdend
        pwmrotberechnen(); // rechnet den pwm-Wert für die rote LED anhand des CO2 ppm aus
        analogWrite(ledmosfetrot, pwmrot);
        analogWrite(ledmosfetgruen, (255 - pwmrot));
        analogWrite(ledmosfetblau, 0);
        Serial.println("led würde jetzt co2 anzeigen");
    }
    if (buttonLED_status == 2){ // manuelle Einstellung über Potis
        potirot_prozent = (float)analogRead(potirot)/1023;
        brightnessrot = potirot_prozent*255;
        analogWrite(ledmosfetrot, brightnessrot);
        Serial.println("led würde jetzt potis anzeigen");

        potigruen_prozent = (float)analogRead(potigruen)/1023;
        brightnessgruen = potigruen_prozent*255;
        analogWrite(ledmosfetgruen, brightnessgruen);

        potiblau_prozent = (float)analogRead(potiblau)/1023;
        brightnessblau = potiblau_prozent*255;
        analogWrite(ledmosfetblau, brightnessblau);
    }
    if (buttonLED_status == 3){ // LEDs aus
        analogWrite(ledmosfetrot, 0);
        analogWrite(ledmosfetgruen, 0);
        analogWrite(ledmosfetblau, 0);
        Serial.println("led würde jetzt nichts anzeigen");
    }
}

void buttonOLEDwatch(){ //verändert die buttonOLED_status beim drücken des entspr. Knopfes
      current_buttonOLED = digitalRead(buttonOLED); // überträgt den aktuellen Status des Knopfes in eine Variable
      
  if (current_buttonOLED != last_buttonOLED){ // hat sich der Status zur letzten Erfassung verändert?
    last_buttonOLED = current_buttonOLED; // dann ändere die Vergleichsvariable und...
    if (current_buttonOLED == HIGH){ // falls er gedrückt wurde (also neu HIGH wurde,) ändere den Status += 1
      if (buttonOLED_status == 1){
          buttonOLED_status = 2;
          Serial.println("oled zu 2 gewechselt");
      }
      else if (buttonOLED_status == 2){
          buttonOLED_status = 3;
          Serial.println("oled zu 3 gewechselt");
      }
      else if (buttonOLED_status == 3) {
          buttonOLED_status = 1;
          Serial.println("oled zu 1 gewechselt");
      }
    }
  }
}

void buttonLEDwatch(){ //verändert die buttonLED_status beim drücken des entspr. Knopfes
      current_buttonLED = digitalRead(buttonLED); // checkt, ob der Knopf gedrück wird
      
  if (current_buttonLED != last_buttonLED){ // wurde der Knopf neu gedrückt, losgelassen oder wird er gehalten = hat sich der Status verändert?
    last_buttonLED = current_buttonLED; // dann speichere dies und
    if (current_buttonLED == HIGH){ // falls er gedrückt wurde ändere den Status auf 1, 2 oder 3
      if (buttonLED_status == 1){
          buttonLED_status = 2;
          Serial.println("led zu 2 gewechselt");
      }
      else if (buttonLED_status == 2){
          buttonLED_status = 3;
          Serial.println("led zu 3 gewechselt");
      }
      else if (buttonLED_status == 3) {
          buttonLED_status = 1;
          Serial.println("led zu 1 gewechselt");
      }
    }
  }
}

void loop() {
   // Serial.println("program wird gestartet");
    co2messung();
    oledanzeige();
    buttonOLEDwatch();
    buttonLEDwatch();
    LEDanzeige();
}