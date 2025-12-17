#include <Arduino.h>

#include <U8g2lib.h>
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <DHT.h>

#define DHT11_PIN  
#define DHT_TYPE DHT11


DHT dht(DHT11_PIN, DHT_TYPE);

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

#define SOUND_PIN 35   
#define LIGHT_PIN 34   
#define PREV_BUTTON_PIN 19  
#define NEXT_BUTTON_PIN 23  


const int sampleWindow = 100;   
unsigned int soundLevel = 0;
unsigned int maxSoundLevel = 0;
unsigned int avgSoundLevel = 0;
int soundDB = 0;

float tempC = 0.0;
float humidity = 0.0;


int lightValue = 0;
int initLight = 0;         
int minLightValue = 4095;  
int maxLightValue = 0;     
int avgLightValue = 0;
int lightHistory[32];      
int lightHistoryIndex = 0;
String lightChangeStatus = "Initializing...";  


volatile int currentScreen = 0;  
const int totalScreens = 3;


unsigned long lastButtonPress = 0;
const unsigned long debounceDelay = 200;


unsigned int soundHistory[32]; 
int historyIndex = 0;


bool isValidTemperature(float temperature) {
    
    return (!isnan(temperature) && temperature >= 0 && temperature <= 50);
}

bool isValidHumidity(float hum) {
    return (!isnan(hum) && hum >= 0 && hum <= 100);
}


const char* getTemperatureCondition(float tempC) {
    if (tempC < 15.0) return "Cold";
    if (tempC < 25.0) return "Cool";
    if (tempC <= 30.0) return "Warm";
    return "Hot";
}

const char* getSoundLevelDescription(int db) {
    if (db < 40) return "Quiet";
    if (db < 60) return "Normal";
    if (db < 80) return "Loud";
    return "V.Loud";
}


const char* getLightLevelDescription(int lightVal) {
    if (lightVal < 500) return "Dark";
    if (lightVal < 1500) return "Dim";
    if (lightVal < 3000) return "Bright";
    return "V.Bright";
}


int getLightPercentage(int lightVal) {
    return map(lightVal, 0, 4095, 0, 100);
}


void readSensorTask(void * parameter) {
    while (true) {
        
        float newTemp = dht.readTemperature();
        float newHum = dht.readHumidity();
        
        
        if (isValidTemperature(newTemp)) {
            tempC = newTemp;
        }
        if (isValidHumidity(newHum)) {
            humidity = newHum;
        }
        
        
        Serial.print("Temperature: ");
        Serial.print(tempC);
        Serial.print("°C, Humidity: ");
        Serial.print(humidity);
        Serial.println("%");
        
        vTaskDelay(2000 / portTICK_PERIOD_MS);  
    }
}


void readSensorTask2(void * parameter) {
    while (true) {
        unsigned long startMillis = millis();  
        unsigned int signalMax = 0;    
        unsigned int signalMin = 4095;  
        
        while (millis() - startMillis < sampleWindow) {
            int sample = analogRead(SOUND_PIN);
            if (sample < 4095) {  
                if (sample > signalMax) {
                    signalMax = sample;
                }
                else if (sample < signalMin) {
                    signalMin = sample;
                }
            }
        }
        
        int peakToPeak = signalMax - signalMin;    
        soundLevel = peakToPeak;
        soundDB = map(peakToPeak, 0, 2000, 30, 90);  
        soundDB = constrain(soundDB, 30, 90);
        
        if (soundLevel > maxSoundLevel) {
            maxSoundLevel = soundLevel;
        }
        
        static unsigned long soundSum = 0;
        static int soundCount = 0;
        soundSum += soundLevel;
        soundCount++;
        if (soundCount > 10) {
            avgSoundLevel = soundSum / soundCount;
            soundSum = 0;
            soundCount = 0;
        }
        
        soundHistory[historyIndex] = map(soundLevel, 0, 1000, 0, 20);
        historyIndex = (historyIndex + 1) % 32;
        
        Serial.print("Sound Level: ");
        Serial.print(soundLevel);
        Serial.print(", dB: ");
        Serial.println(soundDB);
        
        vTaskDelay(100 / portTICK_PERIOD_MS);  
    }
}


void readSensorTask3(void * parameter) {
    while (true) {
        
        lightValue = analogRead(LIGHT_PIN);
        
        if (abs(lightValue - initLight) > 50) {
            int change = lightValue - initLight;
            if (change > 0) {
                lightChangeStatus = "Brighter: +" + String(change);
            } else {
                lightChangeStatus = "Darker: " + String(change);
            }
        } else {
            lightChangeStatus = "No change";
        }
        
        if (lightValue < minLightValue) {
            minLightValue = lightValue;
        }
        if (lightValue > maxLightValue) {
            maxLightValue = lightValue;
        }
        
        static unsigned long lightSum = 0;
        static int lightCount = 0;
        lightSum += lightValue;
        lightCount++;
        if (lightCount > 10) {
            avgLightValue = lightSum / lightCount;
            lightSum = 0;
            lightCount = 0;
        }
        
        lightHistory[lightHistoryIndex] = map(lightValue, 0, 4095, 0, 25);
        lightHistoryIndex = (lightHistoryIndex + 1) % 32;
        
        Serial.print("Light Value: ");
        Serial.print(lightValue);
        Serial.print(" (");
        Serial.print(getLightPercentage(lightValue));
        Serial.print("%) - ");
        Serial.println(lightChangeStatus);
        
        vTaskDelay(150 / portTICK_PERIOD_MS);  
    }
}


void buttonTask(void * parameter) {
    while (true) {
        unsigned long currentTime = millis();
        
        
        if (currentTime - lastButtonPress > debounceDelay) {
            
            if (digitalRead(PREV_BUTTON_PIN) == LOW) {
                currentScreen = (currentScreen - 1 + totalScreens) % totalScreens;
                lastButtonPress = currentTime;
                Serial.println("Previous button pressed, Screen: " + String(currentScreen));
            }
            
            
            if (digitalRead(NEXT_BUTTON_PIN) == LOW) {
                currentScreen = (currentScreen + 1) % totalScreens;
                lastButtonPress = currentTime;
                Serial.println("Next button pressed, Screen: " + String(currentScreen));
            }
        }
        
        vTaskDelay(50 / portTICK_PERIOD_MS);  
    }
}


void drawDHTScreen() {
    
    String tempDisplay;
    String humDisplay;
    String conditionDisplay;
    
    if (isValidTemperature(tempC)) {
        tempDisplay = "T: " + String(tempC, 1) + "C";
        const char* condition = getTemperatureCondition(tempC);
        conditionDisplay = "Stat: " + String(condition);
    } else {
        tempDisplay = "T: N/A";
        conditionDisplay = "Sts: N/A";
    }
    
    if (isValidHumidity(humidity)) {
        humDisplay = "H: " + String(humidity, 1) + "%";
    } else {
        humDisplay = "H: N/A";
    }
    
    
    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.drawStr(1, 30, "SENSOR: TMPwHMDT");
    
    
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(1, 42, tempDisplay.c_str());
    u8g2.drawStr(64, 42, humDisplay.c_str());
    u8g2.drawStr(1, 54, conditionDisplay.c_str());
    
   
    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.drawStr(1, 64, "<PREV");
    u8g2.drawStr(90, 64, "NEXT>");
}


void drawSoundScreen() {
    
    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.drawStr(1, 30, "SENSOR: SOUND");
    
    String levelDisplay = "Lvl: " + String(soundLevel);
    String dbDisplay = "dB: " + String(soundDB);
    String maxDisplay = "Max: " + String(maxSoundLevel);
    String statusDisplay = "Sts: " + String(getSoundLevelDescription(soundDB));
    
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(1, 42, levelDisplay.c_str());
    u8g2.drawStr(70, 42, dbDisplay.c_str());
    u8g2.drawStr(70, 54, maxDisplay.c_str());
    
    u8g2.drawStr(1, 54, statusDisplay.c_str());
    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.drawStr(1, 64, "<PREV");
    u8g2.drawStr(90, 64, "NEXT>");
}

void drawLightScreen() {
    
    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.drawStr(1, 30, "SENSOR: LIGHT");
    
    String percentDisplay = "Val: " + String(getLightPercentage(lightValue)) + "%";
    String minDisplay = "Min: " + String(minLightValue);
    String maxDisplay = "Max: " + String(maxLightValue);
    String statusDisplay = "Stat: " + String(getLightLevelDescription(lightValue));

    u8g2.setFont(u8g2_font_ncenB08_tr);
    
    u8g2.drawStr(75, 54, percentDisplay.c_str());
    u8g2.drawStr(1, 42, minDisplay.c_str());
    u8g2.drawStr(75, 42, maxDisplay.c_str());
    u8g2.drawStr(1, 54, statusDisplay.c_str());
    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.drawStr(1, 64, "<PREV");
    u8g2.drawStr(90, 64, "NEXT>");
}

void monitorTemperature(void * parameter) {
    
    u8g2.begin();
    
    for (int i = 0; i < 32; i++) {
        soundHistory[i] = 0;
        lightHistory[i] = 0;
    }
    
    while (true) {
        u8g2.clearBuffer();
        
        switch (currentScreen) {
            case 0:
                drawDHTScreen();
                break;
            case 1:
                drawSoundScreen();
                break;
            case 2:
                drawLightScreen();
                break;
        }
        
        u8g2.sendBuffer();
        vTaskDelay(200 / portTICK_PERIOD_MS);  
    }
}

void setup() {
    Serial.begin(115200);
    
    dht.begin();
    
    tempC = 0.0;
    humidity = 0.0;
    lightValue = 0;
    
    initLight = analogRead(LIGHT_PIN);
    Serial.println("Initial light value: " + String(initLight));

    analogReadResolution(12);
  
    analogSetAttenuation(ADC_11db);  
    
    pinMode(PREV_BUTTON_PIN, INPUT_PULLUP);
    pinMode(NEXT_BUTTON_PIN, INPUT_PULLUP);
    
    pinMode(SOUND_PIN, INPUT);
    pinMode(LIGHT_PIN, INPUT);
    
    Serial.println("Multi-Sensor Monitor Starting...");
    Serial.println("Screens: 0=DHT11, 1=Sound, 2=Light");
    
    xTaskCreatePinnedToCore(readSensorTask, "Read DHT11", 4096, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(readSensorTask2, "Read Sound", 4096, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(readSensorTask3, "Read Light", 4096, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(buttonTask, "Button Handler", 2048, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(monitorTemperature, "Display Manager", 4096, NULL, 1, NULL, 1);
}

void loop() {
    
}