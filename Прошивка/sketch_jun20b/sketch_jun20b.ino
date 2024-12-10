#include <EEPROM.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "OneWire.h"
#include "DallasTemperature.h"
/////////////////////////////////////////////////////
Adafruit_SSD1306 display(128, 64, &Wire, -1);
OneWire oneWire(2);
DallasTemperature ds(&oneWire);
/////////////////////////////////////////////////////
uint32_t myTimer;
volatile int val;
const float r1=100000.0;
const float r2=10000;
bool engineStart;
long timerWork;
/////////////////////////////////////////////////////
//Структура времени работы двигателя/////////////////
/////////////////////////////////////////////////////
typedef struct{
  int motoHours;
  int motoMins;
} timeWork;
timeWork tWork;
/////////////////////////////////////////////////////
void setup() {
  Serial.begin(9600);
  if(EEPROM.read(0) != 'w'){
    EEPROM.put(1, tWork);
    EEPROM.write(0, 'w');
    Serial.println("First");
  }else{
    EEPROM.get(1, tWork);
  }
  ds.begin();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); 
  pinMode(4, INPUT_PULLUP);            
  drawStartLogo(25);
  display.display();
  engineStart = false;
  attachInterrupt(1, isr, CHANGE);
  
  delay(2000); 
}
/////////////////////////////////////////////////////
void loop() {
  drawStartInfo(getTemp(), getSpeed(), getVoltage());
  if (digitalRead(4) == 0){
    drawMotoHours();
  }
  if (getVoltage() >= 13.2 && engineStart == false){
    engineStart = true;
    timerWork = millis();
  }
  if(getVoltage() < 13.2 && engineStart == true){
    engineStart = false;
    getTimeWork();
    EEPROM.put(1, tWork);
  }
  delay(10);
}

/////////////////////////////////////////////////////
//Расчет времени работы//////////////////////////////
/////////////////////////////////////////////////////
void getTimeWork(){
  if(engineStart){
    int mMin = ((millis() - timerWork) / 60000);
    if (mMin >= 60){
      int mH = mMin / 60;
      tWork.motoHours += mH;
      tWork.motoMins += mMin - (mH * 60);
    }
    else{
      tWork.motoMins += mMin;
    }
  }
}
/////////////////////////////////////////////////////
//Расчет скорости////////////////////////////////////
/////////////////////////////////////////////////////
int getSpeed(){
  val = 0;
  delay(500);
  return ((val*60) / 120);
}
/////////////////////////////////////////////////////
//Считывание температуры/////////////////////////////
/////////////////////////////////////////////////////
float getTemp(){
  ds.requestTemperatures();
  return ds.getTempCByIndex(0); 
}
/////////////////////////////////////////////////////
//Считывание напряжения//////////////////////////////
/////////////////////////////////////////////////////
float getVoltage(){
  int analogvalue = analogRead(A0);
  return ((analogvalue * 5.0) / 1024.0) / (r2/(r1+r2));
}
void startDiagnostic(){
  int codeError[] = {0, 0, 0, 0};
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(5, 0);
  display.println(String("DIAGNOSTIC"));
  display.setTextSize(2);
  display.setCursor(15, 20);
  display.println(String("START!"));
  display.display();

  readError(codeError);
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(5, 0);
  for(int i = 0; i < 4; i++){
    display.print(codeError[i]);
  }
}
int readError(int codeError[4]){
  int indexError = -1;
  bool dInit = false;
  bool dStart = false;
  bool signal = false;
  
  while(true){
    int analogValue = analogRead(A2);
    float value = ((analogValue * 5.0) / 1024.0) / (r2/(r1+r2));
    if(value >= 6 && dInit == false){ 
      dInit = true;
      delay(1000);
    } // Отсеивание сигнала замыкания реле на массу
    if(value >= 6 && dInit == true && dStart == false){
      dStart = true;
      delay(2000);
    }  // Отсеивание сигнала начала цикла диагностики
    if(value >= 6 && dInit == true && dStart == true && signal == true){
      signal = false;
      codeError[indexError] += 1;
    }
    if(value >= 6 && dInit == true && dStart == true && signal == false){
      signal = true;
    }
    if(value < 6 && dInit == true && dStart == true && signal == false){
      int startTime = millis();
      while(value < 6){
        int analogValue = analogRead(A2);
        float value = ((analogValue * 5.0) / 1024.0) / (r2/(r1+r2));
      }
      if(millis() - startTime >= 1900){
        indexError += 1;
      }
    }
    if(indexError >= 5){
        break;
    }
  }
  return codeError;
}
/////////////////////////////////////////////////////
void drawDiagnostic(){
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(5, 0);
  display.println(String("DIAGNOSTIC"));
  display.setTextSize(1);
  display.setCursor(15, 20);
  display.println(String("PRESS THE BUTTON"));
  display.setCursor(35, 30);
  display.println(String("TO START"));
  display.display();
  delay(1000);
  while(true){
    if (digitalRead(4) == 0){
      startDiagnostic();
      break;
    }
    if (millis() - myTimer >= 10000) {
      myTimer = millis();
      break;
    }
  }
}
/////////////////////////////////////////////////////
void drawMotoHours(){
  display.clearDisplay();
  display.setTextSize(1); 
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 5);
  display.println(String("MotoTimeWork"));
  display.setTextSize(2);
  getTimeWork();
  display.println(String("H: ") + tWork.motoHours);
  display.println(String("M: ") + tWork.motoMins);
  if(engineStart == false){
    display.println(String("Engine:OFF"));
  }
  else{
    display.println(String("Engine:ON"));
  }
  
  display.display();
  delay(1500);
  while(true){
    if (digitalRead(4) == 0){
      drawDiagnostic();
      break;
    }
    if (millis() - myTimer >= 10000) {
      myTimer = millis();
      break;
    }
  }
}
/////////////////////////////////////////////////////
void drawStartInfo(int temp, int speed, float voltage) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(30, 5);
  display.println(temp + String(" C"));
  display.setTextSize(1);
  display.print(String("Voltage:"));
  display.println(voltage);
  display.setTextSize(3);
  display.setCursor(50, 35);
  display.print(speed);
  display.setTextSize(1);
  display.println(String("KMH"));
  display.display();
}
/////////////////////////////////////////////////////
//AUDI логотип///////////////////////////////////////
/////////////////////////////////////////////////////
void drawStartLogo(int r){
  display.clearDisplay();
  display.setTextSize(2); // указываем размер шрифта
  display.setTextColor(SSD1306_WHITE);
  int x = 25;
  display.drawCircle(25, 25, 20, WHITE);
  display.drawCircle(x + r, 25, 20, WHITE);
  display.drawCircle(x + 2 * r, 25, 20, WHITE);
  display.drawCircle(x + 3 * r, 25, 20, WHITE);
  display.setCursor(128 / 3, 50);
  display.println("AUDI");
}
/////////////////////////////////////////////////////
//Вспомогательный метод для расчета скорости/////////
/////////////////////////////////////////////////////
void isr(){
  val++;
}
