#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ================== НАСТРОЙКИ ПОЛЬЗОВАТЕЛЯ ==================
const int SOIL_TARGET_MIN = 60; 
const int SOIL_TARGET_MAX = 70; 

const int LIGHT_AUTO_THRESHOLD = 700; 

// Задержки (таймеры стабильности) в миллисекундах
const unsigned long LIGHT_DEBOUNCE_TIME = 10000; // 10 сек для света
const unsigned long PUMP_DEBOUNCE_TIME  = 5000;  // 5 сек для насоса

// Калибровка
const int SOIL_DRY  = 1023; 
const int SOIL_WET  = 400;  
const int WATER_EMPTY = 0;  
const int WATER_FULL  = 600; 
const int WATER_LOW_PERCENT = 10; 

const unsigned long PUMP_DURATION = 5000; // Ручной режим

// ================== ПИНЫ ==================
#define PIN_SOIL       A0  
#define PIN_SOIL_POWER 12  
#define PIN_WATER      A1  
#define PIN_LDR        A3  

#define PIN_RELAY_PUMP  8  
#define PIN_RELAY_LIGHT 10 

#define ENCODER_CLK 2
#define ENCODER_DT  3
#define ENCODER_SW  4

#define RELAY_ON  LOW
#define RELAY_OFF HIGH

// ================== ДИСПЛЕЙ ==================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR     0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ================== ПЕРЕМЕННЫЕ ==================
int soilPercent = 0;
int waterPercent = 0;
int lightValue  = 0;
bool waterIsLow = false;
bool soilIsDry = false;

bool autoMode = true;      
bool manualLight = false;  
bool autoLightState = false; 
bool pumpRunning = false;  
unsigned long pumpStartTime = 0;

// Таймеры СВЕТА
unsigned long lightTimerStart = 0;
bool pendingLightState = false; 
bool lightTimerActive = false;

// Таймеры НАСОСА
unsigned long pumpTimerStart = 0;
bool pendingPumpRunning = false;
bool pumpTimerActive = false;

enum Screen { SCREEN_MAIN, SCREEN_SENSORS, SCREEN_MODE };
Screen currentScreen = SCREEN_MAIN;
int mainMenuCursor = 0;
int modeMenuCursor = 0;

const int8_t enc_states[] = {0,-1,1,0, 1,0,0,-1, -1,0,0,1, 0,1,-1,0};
static uint8_t prevNextCode = 0;
static uint16_t store = 0;
unsigned long lastEncoderStepTime = 0;
const unsigned long ENCODER_STEP_INTERVAL = 10;

int lastBtnState = HIGH;
unsigned long lastBtnChange = 0;
const unsigned long BTN_DEBOUNCE_MS = 50;
unsigned long lastPressTime = 0;
unsigned long lastClickTime = 0;
uint8_t clickCount = 0;
const unsigned long DOUBLE_CLICK_MS = 400;
const unsigned long LONG_PRESS_MS  = 800;

bool screenOn = true;
unsigned long lastSensorReadTime = 0;

void setup() {
  Serial.begin(9600);
  pinMode(ENCODER_CLK, INPUT_PULLUP);
  pinMode(ENCODER_DT,  INPUT_PULLUP);
  pinMode(ENCODER_SW,  INPUT_PULLUP);
  pinMode(PIN_SOIL, INPUT);
  pinMode(PIN_WATER, INPUT);
  pinMode(PIN_SOIL_POWER, OUTPUT);
  digitalWrite(PIN_SOIL_POWER, LOW);
  pinMode(PIN_RELAY_PUMP, OUTPUT);
  digitalWrite(PIN_RELAY_PUMP, RELAY_OFF);
  pinMode(PIN_RELAY_LIGHT, OUTPUT);
  digitalWrite(PIN_RELAY_LIGHT, RELAY_OFF);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) { for(;;); }
  display.clearDisplay(); display.display();
  readSensors(); 
  drawScreen();
}

void loop() {
  handleEncoder();
  handleButtonClicks();
  
  if (millis() - lastSensorReadTime > 1000) {
    readSensors();
    runAutoLogic();
    lastSensorReadTime = millis();
    if (screenOn && (currentScreen == SCREEN_SENSORS || waterIsLow || soilIsDry)) drawScreen();
  }
  
  updateRelays();
  checkPumpTimer();
}

void readSensors() {
  digitalWrite(PIN_SOIL_POWER, HIGH); delay(10);
  long soilSum = 0;
  for(int i=0; i<10; i++) { soilSum += analogRead(PIN_SOIL); delay(2); }
  digitalWrite(PIN_SOIL_POWER, LOW);
  int rawSoil = soilSum / 10;
  soilPercent = map(rawSoil, SOIL_DRY, SOIL_WET, 0, 100);
  soilPercent = constrain(soilPercent, 0, 100);
  soilIsDry = (soilPercent < SOIL_TARGET_MIN);

  long waterSum = 0;
  for(int i=0; i<20; i++) { waterSum += analogRead(PIN_WATER); delay(2); }
  int rawWater = waterSum / 20;
  waterPercent = map(rawWater, WATER_EMPTY, WATER_FULL, 0, 100);
  waterPercent = constrain(waterPercent, 0, 100);
  waterIsLow = (waterPercent < WATER_LOW_PERCENT);

  lightValue = analogRead(PIN_LDR);
}

void runAutoLogic() {
  if (autoMode) {
    // === НАСОС (с Таймером) ===
    bool desiredPumpState = pumpRunning; // По умолчанию оставляем как есть

    if (waterIsLow) {
      desiredPumpState = false; // Нет воды - выключаем сразу, без задержек
    } else {
      // Гистерезис
      if (soilPercent < SOIL_TARGET_MIN) desiredPumpState = true;
      else if (soilPercent > SOIL_TARGET_MAX) desiredPumpState = false;
    }

    // Логика таймера Насоса
    if (desiredPumpState != pumpRunning) {
      if (!pumpTimerActive) {
        pumpTimerActive = true;
        pumpTimerStart = millis();
        pendingPumpRunning = desiredPumpState;
      } else {
        if (pendingPumpRunning != desiredPumpState) {
          pumpTimerActive = false; // Передумали
        } else {
          if (millis() - pumpTimerStart >= PUMP_DEBOUNCE_TIME) {
            pumpRunning = desiredPumpState; // Применяем
            pumpTimerActive = false;
          }
        }
      }
    } else {
      pumpTimerActive = false;
    }
    
    // === СВЕТ (с Таймером) ===
    bool desiredLightState = (lightValue < LIGHT_AUTO_THRESHOLD);

    if (desiredLightState != autoLightState) {
      if (!lightTimerActive) {
        lightTimerActive = true;
        lightTimerStart = millis();
        pendingLightState = desiredLightState;
      } else {
        if (pendingLightState != desiredLightState) {
          lightTimerActive = false; // Передумали
        } else {
          if (millis() - lightTimerStart >= LIGHT_DEBOUNCE_TIME) {
             autoLightState = desiredLightState; // Применяем
             lightTimerActive = false;
          }
        }
      }
    } else {
      lightTimerActive = false;
    }
  }
}

void updateRelays() {
  digitalWrite(PIN_RELAY_PUMP, pumpRunning ? RELAY_ON : RELAY_OFF);
  if (autoMode) digitalWrite(PIN_RELAY_LIGHT, autoLightState ? RELAY_ON : RELAY_OFF);
  else digitalWrite(PIN_RELAY_LIGHT, manualLight ? RELAY_ON : RELAY_OFF);
}

void checkPumpTimer() {
  if (!autoMode && pumpRunning) {
    if (waterIsLow) { pumpRunning = false; if(screenOn)drawScreen(); return; }
    if (millis() - pumpStartTime >= PUMP_DURATION) {
      pumpRunning = false;
      if (screenOn && currentScreen == SCREEN_MODE) drawScreen();
    }
  }
}

// === ИНТЕРФЕЙС ===
void handleEncoder() {
  int8_t dir = readEncoderStep(); if(dir==0) return;
  if(!screenOn){ screenOn=true; currentScreen=SCREEN_MAIN; drawScreen(); return; }
  if(currentScreen==SCREEN_MAIN){ mainMenuCursor+=dir; if(mainMenuCursor<0)mainMenuCursor=1; else if(mainMenuCursor>1)mainMenuCursor=0; }
  else if(currentScreen==SCREEN_MODE){ modeMenuCursor+=dir; if(modeMenuCursor<0)modeMenuCursor=2; else if(modeMenuCursor>2)modeMenuCursor=0; }
  drawScreen();
}
int8_t readEncoderStep() { 
  if(millis()-lastEncoderStepTime<ENCODER_STEP_INTERVAL)return 0;
  uint8_t val=(digitalRead(ENCODER_DT)<<1)|digitalRead(ENCODER_CLK);
  prevNextCode=(prevNextCode<<2)|val; prevNextCode&=0x0F;
  if(enc_states[prevNextCode]){ store=(store<<4)|prevNextCode;
    if((store&0xFF)==0x2B){lastEncoderStepTime=millis();return 1;}
    if((store&0xFF)==0x17){lastEncoderStepTime=millis();return -1;}
  } return 0;
}
void handleButtonClicks() {
  int reading = digitalRead(ENCODER_SW);
  if(reading!=lastBtnState){lastBtnChange=millis();lastBtnState=reading;}
  if(!screenOn && reading==LOW){screenOn=true;currentScreen=SCREEN_MAIN;drawScreen();return;}
  if(screenOn && (millis()-lastBtnChange>=BTN_DEBOUNCE_MS)){
    static int stable=HIGH; if(reading!=stable){ stable=reading;
      if(stable==LOW) lastPressTime=millis();
      else { if(millis()-lastPressTime>=LONG_PRESS_MS)goToMainAndSleep();
             else { if(millis()-lastClickTime>DOUBLE_CLICK_MS)clickCount=0; clickCount++; lastClickTime=millis(); } }
    }
    if(clickCount>0 && (millis()-lastClickTime>DOUBLE_CLICK_MS)){
      if(clickCount==1)onSingleClick(); else if(clickCount>=2)onDoubleClick(); clickCount=0;
    }
  }
}
void onSingleClick() {
  if(!screenOn)return; animateSelect();
  if(currentScreen==SCREEN_MAIN){ if(mainMenuCursor==0)currentScreen=SCREEN_SENSORS; else currentScreen=SCREEN_MODE; }
  else if(currentScreen==SCREEN_MODE){
    if(modeMenuCursor==0){ autoMode=!autoMode; if(autoMode){manualLight=false;}}
    else if(modeMenuCursor==1){ if(!autoMode)manualLight=!manualLight; }
    else if(modeMenuCursor==2){ if(!autoMode){
      if(waterIsLow){ display.clearDisplay();display.setCursor(10,25);display.setTextSize(2);display.println(F("NO WATER!"));display.display();delay(1000);}
      else { if(!pumpRunning){pumpRunning=true;pumpStartTime=millis();} else pumpRunning=false; }
    }}
  } drawScreen();
}
void onDoubleClick(){ if(currentScreen!=SCREEN_MAIN){currentScreen=SCREEN_MAIN;drawScreen();} }
void goToMainAndSleep(){ currentScreen=SCREEN_MAIN; screenOn=false; display.clearDisplay(); display.display(); }
void animateSelect(){ if(!screenOn)return; display.drawRect(0,0,127,63,1);display.display();delay(50);display.drawRect(0,0,127,63,0);display.display();delay(50);}

void drawScreen() {
  if(!screenOn)return; display.clearDisplay(); display.setTextSize(1); display.setTextColor(SSD1306_WHITE); display.setCursor(0,0);
  if(currentScreen==SCREEN_MAIN)drawMainMenu(); else if(currentScreen==SCREEN_SENSORS)drawSensorsScreen(); else if(currentScreen==SCREEN_MODE)drawModeScreen();
  if(waterIsLow) drawAlert(110, "!");
  // Индикаторы таймеров (L - light, P - pump)
  if(autoMode) {
     if(lightTimerActive) { display.setCursor(115, 54); display.print(F("L")); }
     if(pumpTimerActive)  { display.setCursor(105, 54); display.print(F("P")); }
  }
  display.display();
}
void drawAlert(int x, const char* str) { if((millis()/500)%2==0){ display.fillRect(x,0,18,12,1); display.setTextColor(0); display.setCursor(x+5,2); display.print(str); display.setTextColor(1); } }

void drawMainMenu() {
  display.println(F("MAIN MENU")); display.drawLine(0,8,127,8,1);
  display.setCursor(0,14); display.print(mainMenuCursor==0?"> ":"  "); display.println(F("SENSORS"));
  display.setCursor(0,26); display.print(mainMenuCursor==1?"> ":"  "); display.println(F("MODE SELECT"));
  display.setCursor(0,40); display.println(F("Current State:"));
  display.setCursor(0,52); if(autoMode)display.print(F("AUTO MODE (ON)")); else { display.print(F("MANUAL: ")); bool a=false; if(manualLight){display.print(F("L=ON "));a=true;} if(pumpRunning){display.print(F("P=ON"));a=true;} if(!a)display.print(F("IDLE")); }
}

void drawSensorsScreen() {
  display.println(F("SENSORS")); display.drawLine(0,8,127,8,1);
  display.setCursor(0,14); display.print(F("Soil:  ")); if(soilIsDry&&!autoMode){display.print(F("DRY! ("));display.print(soilPercent);display.print(F("%)"));}else{display.print(soilPercent);display.print(F("%"));if(autoMode&&pumpRunning)display.print(F(" (Watering)"));}
  display.setCursor(0,26); display.print(F("Water: ")); if(waterIsLow){display.print(F("LOW! ("));display.print(waterPercent);display.print(F("%)"));}else{display.print(waterPercent);display.print(F("% (OK)"));}
  display.setCursor(0,38); display.print(F("Light: ")); display.print(lightValue);
  display.setCursor(0,54); display.print(F("Double click: BACK"));
}

void drawModeScreen() {
  display.println(F("MODE SETTINGS")); display.drawLine(0,8,127,8,1);
  display.setCursor(0,16); display.print(modeMenuCursor==0?"> ":"  "); display.print(F("AUTO MODE: ")); display.println(autoMode?F("[ON]"):F("[OFF]"));
  display.setCursor(0,28); display.print(modeMenuCursor==1?"> ":"  "); display.print(F("LIGHT: ")); if(autoMode){display.print(F("AUTO"));if(autoLightState)display.print(F(" [ON]"));}else display.print(manualLight?F("[ON]"):F("[OFF]"));
  display.setCursor(0,40); display.print(modeMenuCursor==2?"> ":"  "); display.print(F("PUMP:  ")); if(autoMode)display.print(F("AUTO"));else{if(waterIsLow)display.print(F("[NO H2O]"));else display.print(pumpRunning?F("[RUN...]"):F("[OFF]"));}
  display.setCursor(0,54); if(autoMode&&modeMenuCursor>0)display.print(F("Turn AUTO OFF first!"));
}
