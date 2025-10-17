#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "RTClib.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP32Servo.h>
#include <EEPROM.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// ================= CONFIG =================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

RTC_DS1307 rtc;

// Single button for display wake
const int WAKE_BUTTON_PIN = 0;  // BOOT button on ESP32-S3
bool displayActive = true;
unsigned long lastDisplayActivity = 0;
const unsigned long DISPLAY_TIMEOUT = 30000;  // 30 seconds

#define ONE_WIRE_BUS 1
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
float currentTemp = NAN;

Servo servoMotor;
const int SERVO_PIN = 6;
int servoPosition = 0;
int lastServoPosition = -1;

const int LIGHT_PIN = 7;
const int PUMP_PIN = 5;
const int HEATER_PIN = 4;
const int FEEDER_PIN = 9;

const unsigned long FEEDER_PULSE_SEC = 5;
bool feederActive = false;
unsigned long feederStartMillis = 0;
long lastFeederTriggerDayMinute = -1;

#define EEPROM_MAGIC 0xA5
#define EEPROM_ADDR_MAGIC 0
#define EEPROM_ADDR_DATA 1

struct Schedule {
  int feederHour = 12;
  int feederMinute = 0;
  int lightOnHour = 8;
  int lightOnMinute = 0;
  int lightOffHour = 20;
  int lightOffMinute = 0;
  int pumpOnHour = 7;
  int pumpOnMinute = 0;
  int pumpOffHour = 22;
  int pumpOffMinute = 0;
  float targetTemp = 24.0;
} schedule;

struct ManualMode {
  bool lightOn = false;
  bool pumpOn = false;
  bool heaterOn = false;
  bool servoOn = false;
} manualMode;

struct QuietMode {
  bool active = false;
  int durationMinutes = 30;
  unsigned long startTime = 0;
} quietMode;

bool oledPresent = false;
bool rtcPresent = false;
bool dsPresent = false;
bool bleConnected = false;

// BLE Configuration
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHAR_UUID_TEMP "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHAR_UUID_LIGHT "beb5483e-36e1-4688-b7f5-ea07361b26a9"
#define CHAR_UUID_FILTER "beb5483e-36e1-4688-b7f5-ea07361b26aa"
#define CHAR_UUID_HEATER "beb5483e-36e1-4688-b7f5-ea07361b26ab"
#define CHAR_UUID_FEEDER "beb5483e-36e1-4688-b7f5-ea07361b26ac"
#define CHAR_UUID_SERVO "beb5483e-36e1-4688-b7f5-ea07361b26ad"
#define CHAR_UUID_QUIET "beb5483e-36e1-4688-b7f5-ea07361b26ae"
#define CHAR_UUID_TIME_SYNC "beb5483e-36e1-4688-b7f5-ea07361b26af"
#define CHAR_UUID_SCHEDULE_LIGHT "beb5483e-36e1-4688-b7f5-ea07361b26b0"
#define CHAR_UUID_SCHEDULE_FILTER "beb5483e-36e1-4688-b7f5-ea07361b26b1"
#define CHAR_UUID_SCHEDULE_FEEDER "beb5483e-36e1-4688-b7f5-ea07361b26b2"
#define CHAR_UUID_FEED_NOW "beb5483e-36e1-4688-b7f5-ea07361b26b3"

BLEServer* pServer = NULL;
BLECharacteristic* pTempChar = NULL;
BLECharacteristic* pLightChar = NULL;
BLECharacteristic* pFilterChar = NULL;
BLECharacteristic* pHeaterChar = NULL;
BLECharacteristic* pFeederChar = NULL;
BLECharacteristic* pServoChar = NULL;
BLECharacteristic* pQuietChar = NULL;
BLECharacteristic* pTimeSyncChar = NULL;
BLECharacteristic* pScheduleLightChar = NULL;
BLECharacteristic* pScheduleFilterChar = NULL;
BLECharacteristic* pScheduleFeederChar = NULL;
BLECharacteristic* pFeedNowChar = NULL;

// BLE Server Callbacks
class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    bleConnected = true;
  }
  void onDisconnect(BLEServer* pServer) {
    bleConnected = false;
    BLEDevice::startAdvertising();
  }
};

// Light Control Callbacks
class LightCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    String value = String(pCharacteristic->getValue().c_str());
    manualMode.lightOn = (value.toInt() == 1);
  }
};

// Filter Control Callbacks
class FilterCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    String value = String(pCharacteristic->getValue().c_str());
    manualMode.pumpOn = (value.toInt() == 1);
  }
};

// Heater Control Callbacks
class HeaterCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    String value = String(pCharacteristic->getValue().c_str());
    manualMode.heaterOn = (value.toInt() == 1);
  }
};

// Feeder Control Callbacks
class FeederCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    String value = String(pCharacteristic->getValue().c_str());
    if (value.toInt() == 1) {
      triggerFeeder();
    }
  }
};

// Servo Control Callbacks
class ServoCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    String value = String(pCharacteristic->getValue().c_str());
    int pos = value.toInt();
    if (pos >= 0 && pos <= 180) {
      servoPosition = pos;
      manualMode.servoOn = true;
    }
  }
};

// Quiet Mode Callbacks
class QuietModeCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    String value = String(pCharacteristic->getValue().c_str());
    int duration = value.toInt();
    if (duration > 0) {
      quietMode.active = true;
      quietMode.durationMinutes = duration;
      quietMode.startTime = millis();
    } else {
      quietMode.active = false;
    }
  }
};

// Time Sync Callbacks
class TimeSyncCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    String value = String(pCharacteristic->getValue().c_str());
    // Format: "YYYY-MM-DD HH:MM:SS"
    if (value.length() >= 19 && rtcPresent) {
      int year = value.substring(0, 4).toInt();
      int month = value.substring(5, 7).toInt();
      int day = value.substring(8, 10).toInt();
      int hour = value.substring(11, 13).toInt();
      int minute = value.substring(14, 16).toInt();
      int second = value.substring(17, 19).toInt();
      rtc.adjust(DateTime(year, month, day, hour, minute, second));
    }
  }
};

// Schedule Light Callbacks
class ScheduleLightCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    String value = String(pCharacteristic->getValue().c_str());
    // Format: "HH:MM,HH:MM" (on time, off time)
    int commaPos = value.indexOf(',');
    if (commaPos > 0) {
      String onTime = value.substring(0, commaPos);
      String offTime = value.substring(commaPos + 1);
      
      int colonPos1 = onTime.indexOf(':');
      if (colonPos1 > 0) {
        schedule.lightOnHour = onTime.substring(0, colonPos1).toInt();
        schedule.lightOnMinute = onTime.substring(colonPos1 + 1).toInt();
      }
      
      int colonPos2 = offTime.indexOf(':');
      if (colonPos2 > 0) {
        schedule.lightOffHour = offTime.substring(0, colonPos2).toInt();
        schedule.lightOffMinute = offTime.substring(colonPos2 + 1).toInt();
      }
      
      saveToEEPROM();
    }
  }
};

// Schedule Filter Callbacks
class ScheduleFilterCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    String value = String(pCharacteristic->getValue().c_str());
    // Format: "HH:MM,HH:MM" (on time, off time)
    int commaPos = value.indexOf(',');
    if (commaPos > 0) {
      String onTime = value.substring(0, commaPos);
      String offTime = value.substring(commaPos + 1);
      
      int colonPos1 = onTime.indexOf(':');
      if (colonPos1 > 0) {
        schedule.pumpOnHour = onTime.substring(0, colonPos1).toInt();
        schedule.pumpOnMinute = onTime.substring(colonPos1 + 1).toInt();
      }
      
      int colonPos2 = offTime.indexOf(':');
      if (colonPos2 > 0) {
        schedule.pumpOffHour = offTime.substring(0, colonPos2).toInt();
        schedule.pumpOffMinute = offTime.substring(colonPos2 + 1).toInt();
      }
      
      saveToEEPROM();
    }
  }
};

// Schedule Feeder Callbacks
class ScheduleFeederCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    String value = String(pCharacteristic->getValue().c_str());
    // Format: "HH:MM"
    int colonPos = value.indexOf(':');
    if (colonPos > 0) {
      schedule.feederHour = value.substring(0, colonPos).toInt();
      schedule.feederMinute = value.substring(colonPos + 1).toInt();
      saveToEEPROM();
    }
  }
};

// Feed Now Callbacks
class FeedNowCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    triggerFeeder();
  }
};

void saveToEEPROM() {
  EEPROM.write(EEPROM_ADDR_MAGIC, EEPROM_MAGIC);
  EEPROM.put(EEPROM_ADDR_DATA, schedule);
  EEPROM.commit();
}

void loadFromEEPROM() {
  if (EEPROM.read(EEPROM_ADDR_MAGIC) == EEPROM_MAGIC) {
    EEPROM.get(EEPROM_ADDR_DATA, schedule);
  } else {
    saveToEEPROM();
  }
}

void triggerFeeder() {
  feederActive = true;
  feederStartMillis = millis();
  digitalWrite(FEEDER_PIN, HIGH);
}

void setupBLE() {
  BLEDevice::init("AquariumController");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  BLEService* pService = pServer->createService(SERVICE_UUID);

  // Temperature (READ, NOTIFY)
  pTempChar = pService->createCharacteristic(
    CHAR_UUID_TEMP,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );
  pTempChar->addDescriptor(new BLE2902());

  // Light Control (READ, WRITE)
  pLightChar = pService->createCharacteristic(
    CHAR_UUID_LIGHT,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
  );
  pLightChar->setCallbacks(new LightCallbacks());

  // Filter Control (READ, WRITE)
  pFilterChar = pService->createCharacteristic(
    CHAR_UUID_FILTER,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
  );
  pFilterChar->setCallbacks(new FilterCallbacks());

  // Heater Control (READ, WRITE)
  pHeaterChar = pService->createCharacteristic(
    CHAR_UUID_HEATER,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
  );
  pHeaterChar->setCallbacks(new HeaterCallbacks());

  // Feeder Control (READ, WRITE)
  pFeederChar = pService->createCharacteristic(
    CHAR_UUID_FEEDER,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
  );
  pFeederChar->setCallbacks(new FeederCallbacks());

  // Servo Control (READ, WRITE)
  pServoChar = pService->createCharacteristic(
    CHAR_UUID_SERVO,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
  );
  pServoChar->setCallbacks(new ServoCallbacks());

  // Quiet Mode (READ, WRITE)
  pQuietChar = pService->createCharacteristic(
    CHAR_UUID_QUIET,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
  );
  pQuietChar->setCallbacks(new QuietModeCallbacks());

  // Time Sync (WRITE)
  pTimeSyncChar = pService->createCharacteristic(
    CHAR_UUID_TIME_SYNC,
    BLECharacteristic::PROPERTY_WRITE
  );
  pTimeSyncChar->setCallbacks(new TimeSyncCallbacks());

  // Schedule Light (WRITE)
  pScheduleLightChar = pService->createCharacteristic(
    CHAR_UUID_SCHEDULE_LIGHT,
    BLECharacteristic::PROPERTY_WRITE
  );
  pScheduleLightChar->setCallbacks(new ScheduleLightCallbacks());

  // Schedule Filter (WRITE)
  pScheduleFilterChar = pService->createCharacteristic(
    CHAR_UUID_SCHEDULE_FILTER,
    BLECharacteristic::PROPERTY_WRITE
  );
  pScheduleFilterChar->setCallbacks(new ScheduleFilterCallbacks());

  // Schedule Feeder (WRITE)
  pScheduleFeederChar = pService->createCharacteristic(
    CHAR_UUID_SCHEDULE_FEEDER,
    BLECharacteristic::PROPERTY_WRITE
  );
  pScheduleFeederChar->setCallbacks(new ScheduleFeederCallbacks());

  // Feed Now (WRITE)
  pFeedNowChar = pService->createCharacteristic(
    CHAR_UUID_FEED_NOW,
    BLECharacteristic::PROPERTY_WRITE
  );
  pFeedNowChar->setCallbacks(new FeedNowCallbacks());

  pService->start();

  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
}

void setup() {
  Wire.begin();
  pinMode(WAKE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(LIGHT_PIN, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(HEATER_PIN, OUTPUT);
  pinMode(FEEDER_PIN, OUTPUT);

  digitalWrite(LIGHT_PIN, LOW);
  digitalWrite(PUMP_PIN, LOW);
  digitalWrite(HEATER_PIN, LOW);
  digitalWrite(FEEDER_PIN, LOW);

  EEPROM.begin(512);

  oledPresent = display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  if (oledPresent) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
  }

  rtcPresent = rtc.begin();
  if (rtcPresent && !rtc.isrunning()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  sensors.begin();
  dsPresent = (sensors.getDeviceCount() > 0);

  loadFromEEPROM();
  setupBLE();

  if (oledPresent) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(F("Sterownik Akwarium"));
    display.setCursor(0, 12);
    display.println(F("Bartosz Wolny 2025"));
    display.setCursor(0, 24);
    display.print(F("BLE: "));
    display.print(bleConnected ? F("ON") : F("OFF"));
    display.display();
    delay(2000);
  }

  servoMotor.attach(SERVO_PIN);
  servoMotor.write(90);
  delay(600);
  servoMotor.detach();
  lastServoPosition = 90;

  lastDisplayActivity = millis();
}

void loop() {
  DateTime now = rtcPresent ? rtc.now() : DateTime(2000, 1, 1, 0, 0, 0);

  // Read temperature
  if (dsPresent) {
    sensors.requestTemperatures();
    currentTemp = sensors.getTempCByIndex(0);
    
    // Send temperature via BLE
    if (bleConnected && pTempChar != NULL) {
      pTempChar->setValue(currentTemp);
      pTempChar->notify();
    }
  }

  // Handle wake button
  if (digitalRead(WAKE_BUTTON_PIN) == LOW) {
    displayActive = true;
    lastDisplayActivity = millis();
    delay(200); // Debounce
  }

  // Auto-sleep display
  if (displayActive && (millis() - lastDisplayActivity > DISPLAY_TIMEOUT)) {
    displayActive = false;
  }

  updateDevices(now);
  updateDisplay(now);

  delay(100);
}

void updateDevices(DateTime now) {
  // Quiet mode timeout
  if (quietMode.active) {
    unsigned long elapsed = (millis() - quietMode.startTime) / 60000;
    if (elapsed >= (unsigned long)quietMode.durationMinutes) {
      quietMode.active = false;
    }
  }

  // Light control
  bool lightShouldBeOn = manualMode.lightOn;
  if (!manualMode.lightOn) {
    int cur = now.hour() * 60 + now.minute();
    int on = schedule.lightOnHour * 60 + schedule.lightOnMinute;
    int off = schedule.lightOffHour * 60 + schedule.lightOffMinute;
    if (on < off) lightShouldBeOn = (cur >= on && cur < off);
    else lightShouldBeOn = (cur >= on || cur < off);
  }
  digitalWrite(LIGHT_PIN, lightShouldBeOn ? HIGH : LOW);

  // Pump control
  bool pumpShouldBeOn = manualMode.pumpOn;
  if (!manualMode.pumpOn) {
    int cur = now.hour() * 60 + now.minute();
    int on = schedule.pumpOnHour * 60 + schedule.pumpOnMinute;
    int off = schedule.pumpOffHour * 60 + schedule.pumpOffMinute;
    if (on < off) pumpShouldBeOn = (cur >= on && cur < off);
    else pumpShouldBeOn = (cur >= on || cur < off);
  }
  digitalWrite(PUMP_PIN, pumpShouldBeOn ? HIGH : LOW);

  // Heater control with hysteresis
  static bool heaterState = false;
  bool heaterShouldBeOn = manualMode.heaterOn;
  if (!manualMode.heaterOn && !isnan(currentTemp)) {
    if (currentTemp < schedule.targetTemp - 1.0) heaterState = true;
    else if (currentTemp >= schedule.targetTemp + 1.0) heaterState = false;
    heaterShouldBeOn = heaterState;
  }
  digitalWrite(HEATER_PIN, heaterShouldBeOn ? HIGH : LOW);

  // Servo control (aeration)
  int targetPos;
  if (quietMode.active) {
    targetPos = 90; // Off position
  } else if (manualMode.servoOn) {
    targetPos = servoPosition;
  } else {
    // Auto control: turn off 30 minutes before lights off
    int currentMinutes = now.hour() * 60 + now.minute();
    int lightOffMinutes = schedule.lightOffHour * 60 + schedule.lightOffMinute;
    int aerationOffMinutes = lightOffMinutes - 30;
    if (aerationOffMinutes < 0) aerationOffMinutes += 1440;

    bool shouldTurnOffAeration = false;
    if (aerationOffMinutes < lightOffMinutes) {
      shouldTurnOffAeration = (currentMinutes >= aerationOffMinutes && currentMinutes < lightOffMinutes);
    } else {
      shouldTurnOffAeration = (currentMinutes >= aerationOffMinutes || currentMinutes < lightOffMinutes);
    }

    targetPos = shouldTurnOffAeration ? 90 : 0;
  }

  if (targetPos != lastServoPosition) {
    servoMotor.attach(SERVO_PIN);
    servoMotor.write(targetPos);
    delay(600);
    servoMotor.detach();
    lastServoPosition = targetPos;
  }

  // Feeder control
  if (feederActive) {
    if (millis() - feederStartMillis >= FEEDER_PULSE_SEC * 1000UL) {
      feederActive = false;
      digitalWrite(FEEDER_PIN, LOW);
    }
  }

  // Scheduled feeding
  if (rtcPresent) {
    long curDayMinute = (long)now.day() * 1440L + (long)now.hour() * 60L + (long)now.minute();
    long schedDayMinute = (long)now.day() * 1440L + (long)schedule.feederHour * 60L + (long)schedule.feederMinute;

    if (curDayMinute == schedDayMinute && lastFeederTriggerDayMinute != curDayMinute) {
      triggerFeeder();
      lastFeederTriggerDayMinute = curDayMinute;
    }
  }
}

void updateDisplay(DateTime now) {
  if (!oledPresent) return;

  if (!displayActive) {
    display.clearDisplay();
    display.display();
    return;
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Line 1: Time and Date
  display.setCursor(0, 0);
  if (rtcPresent) {
    if (now.hour() < 10) display.print("0");
    display.print(now.hour());
    display.print(":");
    if (now.minute() < 10) display.print("0");
    display.print(now.minute());
    display.print(":");
    if (now.second() < 10) display.print("0");
    display.print(now.second());
    
    display.print(" ");
    if (now.day() < 10) display.print("0");
    display.print(now.day());
    display.print("/");
    if (now.month() < 10) display.print("0");
    display.print(now.month());
  } else {
    display.print(F("Brak RTC"));
  }

  // Line 2: Temperature
  display.setCursor(0, 10);
  display.print(F("Temp: "));
  if (!isnan(currentTemp)) {
    display.print(currentTemp, 1);
    display.print((char)247);
    display.print("C");
  } else {
    display.print(F("N/A"));
  }

  // Line 3: Device Status
  display.setCursor(0, 20);
  display.print(F("S:"));
  display.print(digitalRead(LIGHT_PIN) ? "1" : "0");
  display.print(F(" F:"));
  display.print(digitalRead(PUMP_PIN) ? "1" : "0");
  display.print(F(" G:"));
  display.print(digitalRead(HEATER_PIN) ? "1" : "0");
  display.print(F(" N:"));
  display.print(lastServoPosition == 0 ? "1" : "0");

  // BLE Status
  display.setCursor(100, 20);
  display.print(bleConnected ? F("BLE") : F("---"));

  // Feeding indicator
  if (feederActive) {
    display.setCursor(0, 30);
    display.print(F(">>> KARMIENIE <<<"));
  }

  // Quiet mode indicator
  if (quietMode.active) {
    display.setCursor(90, 0);
    display.print(F("[C]"));
  }

  display.display();
}
