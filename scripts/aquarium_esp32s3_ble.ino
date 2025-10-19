/*
 * IMPORTANT: This code requires the ESP32 BLE library (built-in with ESP32 board package)
 * 
 * Arduino IDE Setup:
 * 1. Board: Tools → Board → ESP32 Arduino → ESP32S3 Dev Module
 * 2. Remove ArduinoBLE library if installed (it conflicts with ESP32 BLE)
 * 3. Make sure ESP32 board package is installed (v2.0.0 or higher)
 * 
 * Required Libraries (install via Library Manager):
 * - Adafruit GFX Library
 * - Adafruit SSD1306
 * - RTClib
 * - OneWire
 * - DallasTemperature
 * - ESP32Servo
 */

#include <EEPROM.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "RTClib.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP32Servo.h>
#include <Preferences.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoOTA.h>
#include <WiFi.h>

// ================= BLE CONFIG =================
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHAR_UUID_TEMP      "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHAR_UUID_LIGHT     "beb5483e-36e1-4688-b7f5-ea07361b26a9"
#define CHAR_UUID_FILTER    "beb5483e-36e1-4688-b7f5-ea07361b26aa"
#define CHAR_UUID_HEATER    "beb5483e-36e1-4688-b7f5-ea07361b26ab"
#define CHAR_UUID_FEEDER    "beb5483e-36e1-4688-b7f5-ea07361b26ac"
#define CHAR_UUID_SERVO     "beb5483e-36e1-4688-b7f5-ea07361b26ad"
#define CHAR_UUID_QUIET     "beb5483e-36e1-4688-b7f5-ea07361b26ae"
#define CHAR_UUID_TIME      "beb5483e-36e1-4688-b7f5-ea07361b26af"
#define CHAR_UUID_SCH_LIGHT "beb5483e-36e1-4688-b7f5-ea07361b26b0"
#define CHAR_UUID_SCH_FILTER "beb5483e-36e1-4688-b7f5-ea07361b26b1"
#define CHAR_UUID_SCH_FEEDER "beb5483e-36e1-4688-b7f5-ea07361b26b2"
#define CHAR_UUID_FEED_NOW  "beb5483e-36e1-4688-b7f5-ea07361b26b3"
#define CHAR_UUID_TARGET_TEMP "beb5483e-36e1-4688-b7f5-ea07361b26b4"

BLEServer* pServer = NULL;
BLECharacteristic* pCharTemp = NULL;
BLECharacteristic* pCharLight = NULL;
BLECharacteristic* pCharFilter = NULL;
BLECharacteristic* pCharHeater = NULL;
BLECharacteristic* pCharFeeder = NULL;
BLECharacteristic* pCharServo = NULL;
BLECharacteristic* pCharQuiet = NULL;
BLECharacteristic* pCharTime = NULL;
BLECharacteristic* pCharSchLight = NULL;
BLECharacteristic* pCharSchFilter = NULL;
BLECharacteristic* pCharSchFeeder = NULL;
BLECharacteristic* pCharFeedNow = NULL;
BLECharacteristic* pCharTargetTemp = NULL;

bool deviceConnected = false;
bool oldDeviceConnected = false;

unsigned long bleStartTime = 0;
const unsigned long BLE_TIMEOUT = 120000; // 2 minutes
bool bleActive = true;
bool bleTimeoutReached = false;
unsigned long buttonHoldStart = 0;
const unsigned long BUTTON_HOLD_TIME = 3000; // 3 seconds to reactivate BLE

// ================= HARDWARE CONFIG =================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define LEFT_MARGIN 8
#define RIGHT_MARGIN 120
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

RTC_DS1307 rtc;

const int BUTTON_PIN = 0;  // BOOT button on ESP32-S3
// const int BUTTON2_PIN = 1;  // Additional button - Removed

const unsigned long DEBOUNCE_DELAY = 20;
const unsigned long INACTIVITY_TIMEOUT = 30000;

const unsigned long HOLD_T1 = 1000;
const unsigned long HOLD_T2 = 2000;
const unsigned long HOLD_T3 = 3000;
const unsigned long BOTH_BUTTONS_HOLD = 4000;

#define ONE_WIRE_BUS 1
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
float currentTemp = NAN;

Servo servoMotor;
const int SERVO_PIN = 6;
int servoPosition = 0;
int lastServoPosition = -1;
bool servoMoving = false;
unsigned long servoMoveStartTime = 0;
const unsigned long SERVO_MOVE_TIME = 600;
bool useCustomServoPosition = false;

const int LIGHT_PIN = 5;
const int PUMP_PIN = 4;
const int HEATER_PIN = 3;
const int FEEDER_PIN = 2;

const unsigned long FEEDER_PULSE_SEC = 5;
bool feederActive = false;
unsigned long feederStartMillis = 0;
long lastFeederTriggerDayMinute = -1;

#define EEPROM_MAGIC 0xA5
#define EEPROM_ADDR_MAGIC 0
#define EEPROM_ADDR_DATA 1

// ================= UTILITY FUNCTIONS =================
unsigned long safeTimeDiff(unsigned long current, unsigned long previous) {
  if (current >= previous) {
    return current - previous;
  } else {
    return (ULONG_MAX - previous) + current + 1;
  }
}

int safeIncrement(int value, int increment, int minVal, int maxVal) {
  if (value > maxVal - increment) {
    return minVal;
  }
  int result = value + increment;
  if (result > maxVal) {
    return minVal;
  }
  return result;
}

int constrainValue(int value, int minVal, int maxVal) {
  if (value < minVal) return minVal;
  if (value > maxVal) return maxVal;
  return value;
}

// ================= STATE MANAGEMENT =================
enum MenuState {
  MAIN_SCREEN, MAIN_MENU, SCHEDULE_MENU, MANUAL_FEEDER,
  SETTINGS_MENU, QUIET_MODE_MENU, SCHEDULE_FEEDER,
  SCHEDULE_LIGHT_ON, SCHEDULE_LIGHT_OFF, SCHEDULE_PUMP_ON,
  SCHEDULE_PUMP_OFF, SET_TIME, MANUAL_SERVO_POSITION,
  QUIET_MODE_DURATION, DEVICE_STATUS, SET_TARGET_TEMP
};

MenuState currentState = MAIN_SCREEN;
int menuIndex = 0;
const int MAIN_MENU_COUNT = 4;
const int MAX_VISIBLE = 2;

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
  bool feederOn = false;
} manualMode;

struct QuietMode {
  bool active = false;
  int durationMinutes = 30;
  unsigned long startTime = 0;
} quietMode;

int editValue = 0;
bool editingValue = false;
bool editingHour = true;
int tempHour = 0;

unsigned long lastBlinkTime = 0;
bool blinkState = true;
unsigned long statusDisplayTime = 0;
unsigned long lastActivityTime = 0;

bool lastButton1Raw = LOW;
bool lastButton2Raw = LOW;
int button1State = LOW;
int button2State = LOW;
unsigned long lastDebounce1 = 0;
unsigned long lastDebounce2 = 0;
unsigned long press1Start = 0;
unsigned long press2Start = 0;
bool longPress1Handled = false;
bool longPress2Handled = false;

unsigned long bothButtonsStart = 0;
bool bothButtonsHandled = false;

unsigned long holdStart = 0;
int holdStage = 0;
bool holdActionDone1 = false;
bool holdActionDone2 = false;
bool holdActionDone3 = false;

bool oledPresent = false;
bool rtcPresent = false;
bool dsPresent = false;

char statusMsg[20] = "";
unsigned long msgTime = 0;
bool feedingMessageActive = false;

unsigned long lastTempReadTime = 0;
const unsigned long TEMP_READ_INTERVAL = 4000;

// ================= BLECALLBACKS =================
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      bleStartTime = millis(); // Reset BLE timer on connect
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class LightControlCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      uint8_t* data = pCharacteristic->getData();
      if (pCharacteristic->getLength() > 0) {
        manualMode.lightOn = (data[0] == 1);
        lastActivityTime = millis();
      }
    }
};

class FilterControlCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      uint8_t* data = pCharacteristic->getData();
      if (pCharacteristic->getLength() > 0) {
        manualMode.pumpOn = (data[0] == 1);
        lastActivityTime = millis();
      }
    }
};

class HeaterControlCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      uint8_t* data = pCharacteristic->getData();
      if (pCharacteristic->getLength() > 0) {
        manualMode.heaterOn = (data[0] == 1);
        lastActivityTime = millis();
      }
    }
};

class FeederControlCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      uint8_t* data = pCharacteristic->getData();
      if (pCharacteristic->getLength() > 0) {
        manualMode.feederOn = (data[0] == 1);
        lastActivityTime = millis();
      }
    }
};

class ServoControlCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      uint8_t* data = pCharacteristic->getData();
      if (pCharacteristic->getLength() > 0) {
        servoPosition = constrainValue(data[0], 0, 90);
        manualMode.servoOn = true;
        useCustomServoPosition = true;
        lastActivityTime = millis();
      }
    }
};

class QuietModeCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      String value = pCharacteristic->getValue().c_str();
      if (value.length() > 0) {
        int duration = value.toInt();
        if (duration > 0) {
          quietMode.active = true;
          quietMode.durationMinutes = duration;
          quietMode.startTime = millis();
          showQuietModeAnimation();
        } else {
          quietMode.active = false;
        }
        lastActivityTime = millis();
      }
    }
};

class TimeSyncCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      String value = pCharacteristic->getValue().c_str();
      if (value.length() >= 19 && rtcPresent) {
        // Format: "YYYY-MM-DD HH:MM:SS"
        int year = value.substring(0, 4).toInt();
        int month = value.substring(5, 7).toInt();
        int day = value.substring(8, 10).toInt();
        int hour = value.substring(11, 13).toInt();
        int minute = value.substring(14, 16).toInt();
        int second = value.substring(17, 19).toInt();
        
        rtc.adjust(DateTime(year, month, day, hour, minute, second));
        lastActivityTime = millis();
      }
    }
};

class ScheduleLightCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      String value = pCharacteristic->getValue().c_str();
      if (value.length() >= 11) {
        // Format: "HH:MM,HH:MM" (on time, off time)
        int onHour = value.substring(0, 2).toInt();
        int onMinute = value.substring(3, 5).toInt();
        int offHour = value.substring(6, 8).toInt();
        int offMinute = value.substring(9, 11).toInt();
        
        schedule.lightOnHour = constrainValue(onHour, 0, 23);
        schedule.lightOnMinute = constrainValue(onMinute, 0, 59);
        schedule.lightOffHour = constrainValue(offHour, 0, 23);
        schedule.lightOffMinute = constrainValue(offMinute, 0, 59);
        saveToEEPROM();
        lastActivityTime = millis();
      }
    }
};

class ScheduleFilterCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      String value = pCharacteristic->getValue().c_str();
      if (value.length() >= 11) {
        // Format: "HH:MM,HH:MM" (on time, off time)
        int onHour = value.substring(0, 2).toInt();
        int onMinute = value.substring(3, 5).toInt();
        int offHour = value.substring(6, 8).toInt();
        int offMinute = value.substring(9, 11).toInt();
        
        schedule.pumpOnHour = constrainValue(onHour, 0, 23);
        schedule.pumpOnMinute = constrainValue(onMinute, 0, 59);
        schedule.pumpOffHour = constrainValue(offHour, 0, 23);
        schedule.pumpOffMinute = constrainValue(offMinute, 0, 59);
        saveToEEPROM();
        lastActivityTime = millis();
      }
    }
};

class ScheduleFeederCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      String value = pCharacteristic->getValue().c_str();
      if (value.length() >= 5) {
        // Format: "HH:MM"
        int hour = value.substring(0, 2).toInt();
        int minute = value.substring(3, 5).toInt();
        
        schedule.feederHour = constrainValue(hour, 0, 23);
        schedule.feederMinute = constrainValue(minute, 0, 59);
        saveToEEPROM();
        lastActivityTime = millis();
      }
    }
};

class FeedNowCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      // Trigger feeding
      feederActive = true;
      feederStartMillis = millis();
      digitalWrite(FEEDER_PIN, HIGH);
      if (rtcPresent) {
        DateTime now = rtc.now();
        long dayMinutes = (long)now.day() * 1440L;
        long hourMinutes = (long)now.hour() * 60L;
        long totalMinutes = dayMinutes + hourMinutes + (long)now.minute();
        if (totalMinutes > 0 && totalMinutes < LONG_MAX) {
          lastFeederTriggerDayMinute = totalMinutes;
        }
      }
      feedingMessageActive = true;
      showMsg("Karmienie :D!");
      lastActivityTime = millis();
    }
};

class TargetTempCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      uint8_t* data = pCharacteristic->getData();
      if (pCharacteristic->getLength() == 4) {
        float temp;
        memcpy(&temp, data, 4);
        schedule.targetTemp = constrainValue((int)temp, 18, 30);
        saveToEEPROM();
        lastActivityTime = millis();
      }
    }
};

// ================= EEPROM FUNCTIONS =================
void validateSchedule() {
  bool needsSave = false;
  
  if (schedule.feederHour < 0 || schedule.feederHour > 23) {
    schedule.feederHour = constrainValue(schedule.feederHour, 0, 23);
    if (schedule.feederHour < 0 || schedule.feederHour > 23) schedule.feederHour = 12;
    needsSave = true;
  }
  if (schedule.feederMinute < 0 || schedule.feederMinute > 59) {
    schedule.feederMinute = constrainValue(schedule.feederMinute, 0, 59);
    if (schedule.feederMinute < 0 || schedule.feederMinute > 59) schedule.feederMinute = 0;
    needsSave = true;
  }
  
  if (schedule.lightOnHour < 0 || schedule.lightOnHour > 23) {
    schedule.lightOnHour = constrainValue(schedule.lightOnHour, 0, 23);
    if (schedule.lightOnHour < 0 || schedule.lightOnHour > 23) schedule.lightOnHour = 8;
    needsSave = true;
  }
  if (schedule.lightOnMinute < 0 || schedule.lightOnMinute > 59) {
    schedule.lightOnMinute = constrainValue(schedule.lightOnMinute, 0, 59);
    if (schedule.lightOnMinute < 0 || schedule.lightOnMinute > 59) schedule.lightOnMinute = 0;
    needsSave = true;
  }
  
  if (schedule.lightOffHour < 0 || schedule.lightOffHour > 23) {
    schedule.lightOffHour = constrainValue(schedule.lightOffHour, 0, 23);
    if (schedule.lightOffHour < 0 || schedule.lightOffHour > 23) schedule.lightOffHour = 20;
    needsSave = true;
  }
  if (schedule.lightOffMinute < 0 || schedule.lightOffMinute > 59) {
    schedule.lightOffMinute = constrainValue(schedule.lightOffMinute, 0, 59);
    if (schedule.lightOffMinute < 0 || schedule.lightOffMinute > 59) schedule.lightOffMinute = 0;
    needsSave = true;
  }
  
  if (schedule.pumpOnHour < 0 || schedule.pumpOnHour > 23) {
    schedule.pumpOnHour = constrainValue(schedule.pumpOnHour, 0, 23);
    if (schedule.pumpOnHour < 0 || schedule.pumpOnHour > 23) schedule.pumpOnHour = 7;
    needsSave = true;
  }
  if (schedule.pumpOnMinute < 0 || schedule.pumpOnMinute > 59) {
    schedule.pumpOnMinute = constrainValue(schedule.pumpOnMinute, 0, 59);
    if (schedule.pumpOnMinute < 0 || schedule.pumpOnMinute > 59) schedule.pumpOnMinute = 0;
    needsSave = true;
  }
  
  if (schedule.pumpOffHour < 0 || schedule.pumpOffHour > 23) {
    schedule.pumpOffHour = constrainValue(schedule.pumpOffHour, 0, 23);
    if (schedule.pumpOffHour < 0 || schedule.pumpOffHour > 23) schedule.pumpOffHour = 22;
    needsSave = true;
  }
  if (schedule.pumpOffMinute < 0 || schedule.pumpOffMinute > 59) {
    schedule.pumpOffMinute = constrainValue(schedule.pumpOffMinute, 0, 59);
    if (schedule.pumpOffMinute < 0 || schedule.pumpOffMinute > 59) schedule.pumpOffMinute = 0;
    needsSave = true;
  }
  
  if (schedule.targetTemp < 18.0 || schedule.targetTemp > 30.0) {
    schedule.targetTemp = 24.0;
    needsSave = true;
  }
  
  if (needsSave) {
    saveToEEPROM();
  }
}

void saveToEEPROM() {
  EEPROM.write(EEPROM_ADDR_MAGIC, EEPROM_MAGIC);
  EEPROM.put(EEPROM_ADDR_DATA, schedule);
  EEPROM.commit();
}

void loadFromEEPROM() {
  if (EEPROM.read(EEPROM_ADDR_MAGIC) == EEPROM_MAGIC) {
    EEPROM.get(EEPROM_ADDR_DATA, schedule);
    validateSchedule();
  } else {
    saveToEEPROM();
  }
}

// ================= HOLD ACTION FUNCTIONS =================
void resetHoldFlags() {
  holdStage = 0;
  holdStart = 0;
  holdActionDone1 = holdActionDone2 = holdActionDone3 = false;
}

void performHoldActionStage1() {
  // Visual feedback only
}

void performHoldActionStage2() {
  if (editingValue) {
    editingValue = false;
    editingHour = true;
    if (currentState == SCHEDULE_FEEDER || currentState == SCHEDULE_LIGHT_ON ||
        currentState == SCHEDULE_LIGHT_OFF || currentState == SCHEDULE_PUMP_ON ||
        currentState == SCHEDULE_PUMP_OFF) {
      currentState = SCHEDULE_MENU;
    } else if (currentState == QUIET_MODE_DURATION) {
      currentState = QUIET_MODE_MENU;
    } else {
      currentState = SETTINGS_MENU;
    }
    showMsg("Anulowano");
  }
}

void performHoldActionStage3() {
  if (editingValue) {
    saveEditedValue();
    editingValue = false;
    editingHour = true;
    saveToEEPROM();
    showMsg("Zapisano");
  }
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  
  Wire.begin();
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  // pinMode(BUTTON2_PIN, INPUT_PULLUP); // Removed
  pinMode(LIGHT_PIN, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(HEATER_PIN, OUTPUT);
  pinMode(FEEDER_PIN, OUTPUT);
  digitalWrite(FEEDER_PIN, LOW);

  EEPROM.begin(512);

  oledPresent = display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  rtcPresent = rtc.begin();
  if (rtcPresent && !rtc.isrunning()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  
  if (rtcPresent) {
    DateTime now = rtc.now();
    if (now.hour() > 23 || now.minute() > 59 || now.second() > 59) {
      rtc.adjust(DateTime(2025, 1, 1, 12, 0, 0));
    }
  }

  sensors.begin();
  dsPresent = (sensors.getDeviceCount() > 0);
  if (dsPresent) {
    sensors.setWaitForConversion(true);
  }

  loadFromEEPROM();

  // Initialize BLE
  BLEDevice::init("AquariumController");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Temperature characteristic (READ, NOTIFY)
  pCharTemp = pService->createCharacteristic(
                CHAR_UUID_TEMP,
                BLECharacteristic::PROPERTY_READ |
                BLECharacteristic::PROPERTY_NOTIFY
              );
  pCharTemp->addDescriptor(new BLE2902());

  // Control characteristics (READ, WRITE, NOTIFY)
  pCharLight = pService->createCharacteristic(
                 CHAR_UUID_LIGHT,
                 BLECharacteristic::PROPERTY_READ |
                 BLECharacteristic::PROPERTY_WRITE |
                 BLECharacteristic::PROPERTY_NOTIFY
               );
  pCharLight->setCallbacks(new LightControlCallbacks());
  pCharLight->addDescriptor(new BLE2902());

  pCharFilter = pService->createCharacteristic(
                  CHAR_UUID_FILTER,
                  BLECharacteristic::PROPERTY_READ |
                  BLECharacteristic::PROPERTY_WRITE |
                  BLECharacteristic::PROPERTY_NOTIFY
                );
  pCharFilter->setCallbacks(new FilterControlCallbacks());
  pCharFilter->addDescriptor(new BLE2902());

  pCharHeater = pService->createCharacteristic(
                  CHAR_UUID_HEATER,
                  BLECharacteristic::PROPERTY_READ |
                  BLECharacteristic::PROPERTY_WRITE |
                  BLECharacteristic::PROPERTY_NOTIFY
                );
  pCharHeater->setCallbacks(new HeaterControlCallbacks());
  pCharHeater->addDescriptor(new BLE2902());

  pCharFeeder = pService->createCharacteristic(
                  CHAR_UUID_FEEDER,
                  BLECharacteristic::PROPERTY_READ |
                  BLECharacteristic::PROPERTY_WRITE |
                  BLECharacteristic::PROPERTY_NOTIFY
                );
  pCharFeeder->setCallbacks(new FeederControlCallbacks());
  pCharFeeder->addDescriptor(new BLE2902());

  pCharServo = pService->createCharacteristic(
                 CHAR_UUID_SERVO,
                 BLECharacteristic::PROPERTY_READ |
                 BLECharacteristic::PROPERTY_WRITE |
                 BLECharacteristic::PROPERTY_NOTIFY
               );
  pCharServo->setCallbacks(new ServoControlCallbacks());
  pCharServo->addDescriptor(new BLE2902());

  pCharQuiet = pService->createCharacteristic(
                 CHAR_UUID_QUIET,
                 BLECharacteristic::PROPERTY_READ |
                 BLECharacteristic::PROPERTY_WRITE |
                 BLECharacteristic::PROPERTY_NOTIFY
               );
  pCharQuiet->setCallbacks(new QuietModeCallbacks());
  pCharQuiet->addDescriptor(new BLE2902());

  pCharTime = pService->createCharacteristic(
                CHAR_UUID_TIME,
                BLECharacteristic::PROPERTY_WRITE
              );
  pCharTime->setCallbacks(new TimeSyncCallbacks());

  pCharSchLight = pService->createCharacteristic(
                    CHAR_UUID_SCH_LIGHT,
                    BLECharacteristic::PROPERTY_WRITE
                  );
  pCharSchLight->setCallbacks(new ScheduleLightCallbacks());

  pCharSchFilter = pService->createCharacteristic(
                     CHAR_UUID_SCH_FILTER,
                     BLECharacteristic::PROPERTY_WRITE
                   );
  pCharSchFilter->setCallbacks(new ScheduleFilterCallbacks());

  pCharSchFeeder = pService->createCharacteristic(
                     CHAR_UUID_SCH_FEEDER,
                     BLECharacteristic::PROPERTY_WRITE
                   );
  pCharSchFeeder->setCallbacks(new ScheduleFeederCallbacks());

  pCharFeedNow = pService->createCharacteristic(
                   CHAR_UUID_FEED_NOW,
                   BLECharacteristic::PROPERTY_WRITE
                 );
  pCharFeedNow->setCallbacks(new FeedNowCallbacks());

  pCharTargetTemp = pService->createCharacteristic(
                      CHAR_UUID_TARGET_TEMP,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  pCharTargetTemp->setCallbacks(new TargetTempCallbacks());
  pCharTargetTemp->addDescriptor(new BLE2902());

  pService->start();

  // WiFi.mode(WIFI_STA);
  // WiFi.begin("YOUR_WIFI_SSID", "YOUR_WIFI_PASSWORD");
  // ArduinoOTA.setHostname("AquariumController");
  // ArduinoOTA.begin();
  
  bleStartTime = millis();
  
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  if (oledPresent) {
    display.clearDisplay();
    display.setCursor(0, 0);
    if (rtcPresent && dsPresent) {
      display.println(F("Sterownik Akwarium"));
      display.setCursor(0, 12);
      display.println(F("Bartosz Wolny 2025"));
      display.setCursor(0, 24);
      display.println(F("BLE: Gotowy"));
    } else {
      display.println(F("BLAD!"));
      display.setCursor(0, 12);
      if (!rtcPresent) display.println(F("Brak RTC"));
      if (!dsPresent) display.println(F("Brak DS18B20"));
    }
    display.display();
    delay(2000);
  }

  servoMotor.detach();
  lastActivityTime = millis();
}

// ================= MAIN LOOP =================
void loop() {
  // ArduinoOTA.handle(); // Uncomment if WiFi/OTA is enabled
  
  if (bleActive && !deviceConnected && (millis() - bleStartTime > BLE_TIMEOUT)) {
    BLEDevice::deinit(true);
    bleActive = false;
    bleTimeoutReached = true;
  }
  
  if (!bleActive && digitalRead(BUTTON_PIN) == LOW) {
    if (buttonHoldStart == 0) {
      buttonHoldStart = millis();
    } else if (millis() - buttonHoldStart > BUTTON_HOLD_TIME) {
      // Reactivate BLE
      initBLE();
      bleActive = true;
      bleStartTime = millis();
      buttonHoldStart = 0;
      showBLEAnimation();
    }
  } else {
    buttonHoldStart = 0;
  }

  DateTime now = rtcPresent ? rtc.now() : DateTime(2000,1,1,0,0,0);
  
  static float lastNotifiedTemp = NAN;
  static float lastNotifiedTargetTemp = NAN; // Notify target temperature changes
  
  if (dsPresent && safeTimeDiff(millis(), lastTempReadTime) >= TEMP_READ_INTERVAL) {
    sensors.requestTemperatures();
    currentTemp = sensors.getTempCByIndex(0);
    lastTempReadTime = millis();
    
    // Update BLE temperature characteristic only if value changed
    if (deviceConnected && !isnan(currentTemp) && (isnan(lastNotifiedTemp) || abs(currentTemp - lastNotifiedTemp) > 0.1)) {
      pCharTemp->setValue(currentTemp);
      pCharTemp->notify();
      lastNotifiedTemp = currentTemp;
    }
  }

  static uint8_t lastLightState = 255;
  static uint8_t lastFilterState = 255;
  static uint8_t lastHeaterState = 255;
  static uint8_t lastFeederState = 255;
  static uint8_t lastServoState = 255;
  static bool lastQuietState = false;
  
  if (deviceConnected) {
    uint8_t lightState = manualMode.lightOn ? 1 : 0;
    uint8_t filterState = manualMode.pumpOn ? 1 : 0;
    uint8_t heaterState = manualMode.heaterOn ? 1 : 0;
    uint8_t feederState = manualMode.feederOn ? 1 : 0;
    uint8_t servoState = servoPosition;
    
    if (lightState != lastLightState) {
      pCharLight->setValue(&lightState, 1);
      pCharLight->notify();
      lastLightState = lightState;
    }
    
    if (filterState != lastFilterState) {
      pCharFilter->setValue(&filterState, 1);
      pCharFilter->notify();
      lastFilterState = filterState;
    }
    
    if (heaterState != lastHeaterState) {
      pCharHeater->setValue(&heaterState, 1);
      pCharHeater->notify();
      lastHeaterState = heaterState;
    }
    
    if (feederState != lastFeederState) {
      pCharFeeder->setValue(&feederState, 1);
      pCharFeeder->notify();
      lastFeederState = feederState;
    }
    
    if (servoState != lastServoState) {
      pCharServo->setValue(&servoState, 1);
      pCharServo->notify();
      lastServoState = servoState;
    }
    
    if (quietMode.active != lastQuietState) {
      String quietStatus = quietMode.active ? String(quietMode.durationMinutes) : "0";
      pCharQuiet->setValue(quietStatus.c_str());
      pCharQuiet->notify();
      lastQuietState = quietMode.active;
    }
    
    if (isnan(lastNotifiedTargetTemp) || abs(schedule.targetTemp - lastNotifiedTargetTemp) > 0.1) {
      pCharTargetTemp->setValue(schedule.targetTemp);
      pCharTargetTemp->notify();
      lastNotifiedTargetTemp = schedule.targetTemp;
    }
  }

  // Handle BLE connection changes
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);
    pServer->startAdvertising();
    oldDeviceConnected = deviceConnected;
  }
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }

  handleButtons();
  updateDevices(now);
  updateDisplay(now);

  if (currentState != MAIN_SCREEN && currentState != DEVICE_STATUS && !editingValue) {
    if (safeTimeDiff(millis(), lastActivityTime) > INACTIVITY_TIMEOUT) {
      currentState = MAIN_SCREEN;
      menuIndex = 0;
    }
  }

  if (currentState == DEVICE_STATUS) {
    if (safeTimeDiff(millis(), statusDisplayTime) > 3000) {
      currentState = MAIN_SCREEN;
      menuIndex = 0;
    }
  }

  if (!manualMode.lightOn && !manualMode.pumpOn && !manualMode.heaterOn && 
      !manualMode.servoOn && !feederActive && !quietMode.active) {
    // Enter deep sleep after 5 minutes of inactivity
    if (millis() - lastActivityTime > 300000) {
      esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0); // Wake on button press
      esp_deep_sleep_start();
    }
  }
  
  delay(10);
}

// ================= BUTTON HANDLING =================
void handleButtons() {
  int r1 = digitalRead(BUTTON_PIN);
  // int r2 = digitalRead(BUTTON2_PIN); // Removed
  unsigned long t = millis();

  // Invert button readings (INPUT_PULLUP means LOW when pressed)
  r1 = !r1;
  // r2 = !r2; // Removed

  // Simplified button handling for a single button
  if (r1 != lastButton1Raw) lastDebounce1 = t;
  if (safeTimeDiff(t, lastDebounce1) > DEBOUNCE_DELAY) {
    if (r1 != button1State) {
      int prevState = button1State;
      button1State = r1;
      if (button1State == HIGH) {
        press1Start = t;
        longPress1Handled = false;
        lastActivityTime = t;
        
        // If BLE is inactive, start the hold for reactivation
        if (!bleActive) {
          buttonHoldStart = t;
        }
      } else if (prevState == HIGH) {
        if (!longPress1Handled) {
          onNavShort();
        }
        // Reset hold start if button released
        buttonHoldStart = 0;
      }
    }
  }

  if (button1State == HIGH && currentState == MAIN_SCREEN) {
    if (!longPress1Handled && (safeTimeDiff(t, press1Start) >= 2000)) {
      manualMode.lightOn = false;
      manualMode.pumpOn = false;
      manualMode.heaterOn = false;
      manualMode.servoOn = false;
      manualMode.feederOn = false;
      useCustomServoPosition = false;
      currentState = DEVICE_STATUS;
      statusDisplayTime = t;
      longPress1Handled = true;
      lastActivityTime = t;
      showMsg("Reset");
    }
  }

  lastButton1Raw = r1;
}

// =================NAVIGATION FUNCTIONS =================
void onNavShort() {
  if (editingValue) {
    if (currentState == QUIET_MODE_DURATION) {
      editValue = safeIncrement(editValue, 15, 30, 60);
    } else if (currentState == MANUAL_SERVO_POSITION) {
      editValue = safeIncrement(editValue, 10, 0, 90);
    } else if (currentState == SET_TARGET_TEMP) {
      editValue = safeIncrement(editValue, 1, 18, 30);
    } else if (currentState == SCHEDULE_FEEDER || currentState == SCHEDULE_LIGHT_ON ||
               currentState == SCHEDULE_LIGHT_OFF || currentState == SCHEDULE_PUMP_ON ||
               currentState == SCHEDULE_PUMP_OFF) {
      if (editingHour) {
        editValue = constrainValue(editValue, 0, 23);
        editValue = (editValue + 1) % 24;
      } else {
        editValue = constrainValue(editValue, 0, 59);
        editValue = safeIncrement(editValue, 10, 0, 50);
      }
    } else if (currentState == SET_TIME) {
      if (editingHour) {
        editValue = constrainValue(editValue, 0, 23);
        editValue = (editValue + 1) % 24;
      } else {
        editValue = constrainValue(editValue, 0, 59);
        editValue = safeIncrement(editValue, 10, 0, 50);
      }
    } else {
      editValue++;
    }
    return;
  }

  switch (currentState) {
    case MAIN_MENU: menuIndex = (menuIndex + 1) % MAIN_MENU_COUNT; break;
    case SCHEDULE_MENU: menuIndex = (menuIndex + 1) % 5; break;
    case SETTINGS_MENU: menuIndex = (menuIndex + 1) % 5; break;
    case QUIET_MODE_MENU: menuIndex = (menuIndex + 1) % 2; break;
    default: break;
  }
}

void onActionShort() {
  if (editingValue) {
    if (currentState == SCHEDULE_FEEDER || currentState == SCHEDULE_LIGHT_ON ||
        currentState == SCHEDULE_LIGHT_OFF || currentState == SCHEDULE_PUMP_ON ||
        currentState == SCHEDULE_PUMP_OFF) {
      if (editingHour) {
        tempHour = constrainValue(editValue, 0, 23);
        editingHour = false;
        editValue = 0;
      } else {
        saveEditedValue();
        editingValue = false;
        editingHour = true;
        saveToEEPROM();
        showMsg("Zapisano");
      }
    } else if (currentState == SET_TIME) {
      if (editingHour) {
        tempHour = constrainValue(editValue, 0, 23);
        editingHour = false;
        editValue = 0;
      } else {
        int validHour = constrainValue(tempHour, 0, 23);
        int validMinute = constrainValue(editValue, 0, 59);
        
        if (rtcPresent) {
          DateTime now = rtc.now();
          rtc.adjust(DateTime(now.year(), now.month(), now.day(), validHour, validMinute, 0));
        }
        currentState = SETTINGS_MENU;
        editingValue = false;
        editingHour = true;
        showMsg("Zapisano");
      }
    } else {
      saveEditedValue();
      editingValue = false;
      showMsg("Zapisano");
    }
    return;
  }

  switch (currentState) {
    case MAIN_MENU:
      if (menuIndex == 0) { currentState = MANUAL_FEEDER; menuIndex = 0; showMsg("Karmnik"); }
      else if (menuIndex == 1) { currentState = SCHEDULE_MENU; menuIndex = 0; showMsg("Harmonogram"); }
      else if (menuIndex == 2) { currentState = QUIET_MODE_MENU; menuIndex = 0; showMsg("Tryb Cichy"); }
      else if (menuIndex == 3) { currentState = SETTINGS_MENU; menuIndex = 0; showMsg("Ustawienia"); }
      break;
    case SCHEDULE_MENU:
      if (menuIndex == 0) { 
        currentState = SCHEDULE_FEEDER; 
        editValue = constrainValue(schedule.feederHour, 0, 23);
        editingValue = true; 
        editingHour = true;
        showMsg("Edycja");
      }
      else if (menuIndex == 1) { 
        currentState = SCHEDULE_LIGHT_ON; 
        editValue = constrainValue(schedule.lightOnHour, 0, 23);
        editingValue = true; 
        editingHour = true;
        showMsg("Edycja");
      }
      else if (menuIndex == 2) { 
        currentState = SCHEDULE_LIGHT_OFF; 
        editValue = constrainValue(schedule.lightOffHour, 0, 23);
        editingValue = true; 
        editingHour = true;
        showMsg("Edycja");
      }
      else if (menuIndex == 3) { 
        currentState = SCHEDULE_PUMP_ON; 
        editValue = constrainValue(schedule.pumpOnHour, 0, 23);
        editingValue = true; 
        editingHour = true;
        showMsg("Edycja");
      }
      else { 
        currentState = SCHEDULE_PUMP_OFF; 
        editValue = constrainValue(schedule.pumpOffHour, 0, 23);
        editingValue = true; 
        editingHour = true;
        showMsg("Edycja");
      }
      break;
    case MANUAL_FEEDER:
      feederActive = true;
      feederStartMillis = millis();
      digitalWrite(FEEDER_PIN, HIGH);
      if (rtcPresent) {
        DateTime now = rtc.now();
        long dayMinutes = (long)now.day() * 1440L;
        long hourMinutes = (long)now.hour() * 60L;
        long totalMinutes = dayMinutes + hourMinutes + (long)now.minute();
        if (totalMinutes > 0 && totalMinutes < LONG_MAX) {
          lastFeederTriggerDayMinute = totalMinutes;
        }
      }
      feedingMessageActive = true;
      showMsg("Karmienie :D!");
      break;
    case SETTINGS_MENU:
      if (menuIndex == 0) {
        currentState = SET_TIME;
        editingValue = true;
        editingHour = true;
        if (rtcPresent) {
          int currentHour = rtc.now().hour();
          editValue = constrainValue(currentHour, 0, 23);
        } else {
          editValue = 12;
        }
        tempHour = 0;
        showMsg("Czas");
      } else if (menuIndex == 1) {
        manualMode.lightOn = !manualMode.lightOn;
        showMsg(manualMode.lightOn ? "Sw ON" : "Sw OFF");
      } else if (menuIndex == 2) {
        manualMode.pumpOn = !manualMode.pumpOn;
        showMsg(manualMode.pumpOn ? "Pmp ON" : "Pmp OFF");
      } else if (menuIndex == 3) {
        manualMode.servoOn = !manualMode.servoOn;
        useCustomServoPosition = false;
        showMsg(manualMode.servoOn ? "Nap ON" : "Nap OFF");
      } else if (menuIndex == 4) {
        currentState = SET_TARGET_TEMP;
        editValue = (int)schedule.targetTemp;
        editingValue = true;
        showMsg("Temp");
      }
      break;
    case QUIET_MODE_MENU:
      if (menuIndex == 0) {
        quietMode.active = !quietMode.active;
        if (quietMode.active) {
          quietMode.startTime = millis();
          showQuietModeAnimation();
        }
        showMsg(quietMode.active ? "Cichy ON" : "Cichy OFF");
      } else if (menuIndex == 1) {
        currentState = QUIET_MODE_DURATION;
        editValue = constrainValue(quietMode.durationMinutes, 30, 60);
        editingValue = true;
        showMsg("Czas");
      }
      break;
    default: break;
  }
  lastActivityTime = millis();
}

// ================= DEVICE UPDATE =================
void updateDevices(DateTime now) {
  if (quietMode.active) {
    unsigned long elapsed = safeTimeDiff(millis(), quietMode.startTime) / 60000;
    if (elapsed >= (unsigned long)quietMode.durationMinutes) quietMode.active = false;
  }

  bool lightShouldBeOn = manualMode.lightOn;
  if (!manualMode.lightOn) {
    int cur = now.hour() * 60 + now.minute();
    int on = schedule.lightOnHour * 60 + schedule.lightOnMinute;
    int off = schedule.lightOffHour * 60 + schedule.lightOffMinute;
    if (on < off) lightShouldBeOn = (cur >= on && cur < off);
    else lightShouldBeOn = (cur >= on || cur < off);
  }
  digitalWrite(LIGHT_PIN, lightShouldBeOn ? HIGH : LOW);

  bool pumpShouldBeOn = manualMode.pumpOn;
  if (!manualMode.pumpOn) {
    int cur = now.hour() * 60 + now.minute();
    int on = schedule.pumpOnHour * 60 + schedule.pumpOnMinute;
    int off = schedule.pumpOffHour * 60 + schedule.pumpOffMinute;
    if (on < off) pumpShouldBeOn = (cur >= on && cur < off);
    else pumpShouldBeOn = (cur >= on || cur < off);
  }
  digitalWrite(PUMP_PIN, pumpShouldBeOn ? HIGH : LOW);

  static bool heaterState = false;
  bool heaterShouldBeOn = manualMode.heaterOn;
  if (!manualMode.heaterOn && !isnan(currentTemp)) {
    if (currentTemp < schedule.targetTemp - 1.0) heaterState = true;
    else if (currentTemp >= schedule.targetTemp + 1.0) heaterState = false;
    heaterShouldBeOn = heaterState;
  }
  digitalWrite(HEATER_PIN, heaterShouldBeOn ? HIGH : LOW);

  int targetPos;
  if (quietMode.active) {
    targetPos = 90;
  } else {
    int currentMinutes = now.hour() * 60 + now.minute();
    int lightOffMinutes = schedule.lightOffHour * 60 + schedule.lightOffMinute;
    
    int aerationOffMinutes = lightOffMinutes - 30;
    if (aerationOffMinutes < 0) {
      aerationOffMinutes += 1440;
    }
    
    bool shouldTurnOffAeration = false;
    if (aerationOffMinutes < lightOffMinutes) {
      shouldTurnOffAeration = (currentMinutes >= aerationOffMinutes && currentMinutes < lightOffMinutes);
    } else {
      shouldTurnOffAeration = (currentMinutes >= aerationOffMinutes || currentMinutes < lightOffMinutes);
    }
    
    if (shouldTurnOffAeration) {
      targetPos = 90;
    } else if (manualMode.servoOn) {
      targetPos = useCustomServoPosition ? servoPosition : 0;
    } else {
      targetPos = 90;
    }
  }
  
  if (targetPos != lastServoPosition && !servoMoving) {
    servoMotor.attach(SERVO_PIN);
    servoMotor.write(targetPos);
    lastServoPosition = targetPos;
    servoMoving = true;
    servoMoveStartTime = millis();
  }
  if (servoMoving && (safeTimeDiff(millis(), servoMoveStartTime) >= SERVO_MOVE_TIME)) {
    servoMotor.detach();
    servoMoving = false;
  }

  if (manualMode.feederOn) digitalWrite(FEEDER_PIN, HIGH);
  else {
    if (!feederActive) digitalWrite(FEEDER_PIN, LOW);
  }
  if (feederActive) {
    if (safeTimeDiff(millis(), feederStartMillis) >= FEEDER_PULSE_SEC * 1000UL) {
      feederActive = false;
      if (!manualMode.feederOn) digitalWrite(FEEDER_PIN, LOW);
    }
  }

  if (rtcPresent) {
    long curDayMinute = (long)now.day() * 1440L + (long)now.hour() * 60L + (long)now.minute();
    long schedDayMinute = (long)now.day() * 1440L + (long)schedule.feederHour * 60L + (long)schedule.feederMinute;
    
    if (curDayMinute > 0 && curDayMinute < LONG_MAX && schedDayMinute > 0 && schedDayMinute < LONG_MAX) {
      if (curDayMinute == schedDayMinute) {
        if (lastFeederTriggerDayMinute != curDayMinute) {
          feederActive = true;
          feederStartMillis = millis();
          digitalWrite(FEEDER_PIN, HIGH);
          lastFeederTriggerDayMinute = curDayMinute;
        }
      }
    }
  }
}

// ================= DISPLAY FUNCTIONS =================
void updateDisplay(DateTime now) {
  if (!oledPresent) return;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  unsigned long msgDuration = feedingMessageActive ? (FEEDER_PULSE_SEC * 1000UL) : 2000;
  
  if (feedingMessageActive && feederActive) {
    showFeedingAnimation();
    return;
  }
  
  if (statusMsg[0] != '\0' && (safeTimeDiff(millis(), msgTime) < msgDuration)) {
    display.setTextSize(1);
    int textWidth = strlen(statusMsg) * 6;
    int centerX = (SCREEN_WIDTH - textWidth) / 2;
    int centerY = 10;
    display.setCursor(centerX, centerY);
    display.print(statusMsg);
    display.display();
    return;
  } else if (safeTimeDiff(millis(), msgTime) >= msgDuration) {
    statusMsg[0] = '\0';
    feedingMessageActive = false;
  }

  switch (currentState) {
    case MAIN_SCREEN: displayMainScreen(now); break;
    case MAIN_MENU: displayMainMenu(); break;
    case SCHEDULE_MENU: displayScheduleMenu(); break;
    case MANUAL_FEEDER: displayManualFeeder(); break;
    case SETTINGS_MENU: displaySettingsMenu(); break;
    case QUIET_MODE_MENU: displayQuietModeMenu(); break;
    case SCHEDULE_FEEDER:
    case SCHEDULE_LIGHT_ON:
    case SCHEDULE_LIGHT_OFF:
    case SCHEDULE_PUMP_ON:
    case SCHEDULE_PUMP_OFF: displayEditSchedule(); break;
    case SET_TIME: displaySetTime(); break;
    case MANUAL_SERVO_POSITION: displayEditServoPosition(); break;
    case QUIET_MODE_DURATION: displayEditDuration(); break;
    case DEVICE_STATUS: displayDeviceStatus(); break;
    case SET_TARGET_TEMP: displaySetTargetTemp(); break;
    default: displayMainScreen(now); break;
  }

  display.display();
}

void showFeedingAnimation() {
  if (!oledPresent) return;
  
  unsigned long elapsed = safeTimeDiff(millis(), feederStartMillis);
  unsigned long totalDuration = FEEDER_PULSE_SEC * 1000UL;
  
  // Animation runs for feeding duration
  if (elapsed >= totalDuration) {
    feedingMessageActive = false;
    return;
  }
  
  display.clearDisplay();
  
  // Three fish at different positions
  struct Fish {
    int x, y;
    bool facingRight;
  };
  
  Fish fish[3] = {
    {20, 24, true},   // Bottom left
    {60, 20, false},  // Middle right
    {100, 26, false}  // Bottom right
  };
  
  // Draw fish
  for (int i = 0; i < 3; i++) {
    int fx = fish[i].x;
    int fy = fish[i].y;
    
    if (fish[i].facingRight) {
      // Fish facing right
      display.fillTriangle(fx, fy, fx - 6, fy - 3, fx - 6, fy + 3, SSD1306_WHITE); // tail
      display.fillCircle(fx + 3, fy, 3, SSD1306_WHITE); // body
      display.drawPixel(fx + 5, fy - 1, SSD1306_BLACK); // eye
    } else {
      // Fish facing left
      display.fillTriangle(fx, fy, fx + 6, fy - 3, fx + 6, fy + 3, SSD1306_WHITE); // tail
      display.fillCircle(fx - 3, fy, 3, SSD1306_WHITE); // body
      display.drawPixel(fx - 5, fy - 1, SSD1306_BLACK); // eye
    }
  }
  
  // Food pellets falling from different positions
  int numPellets = 8;
  for (int i = 0; i < numPellets; i++) {
    // Different starting X positions across the screen
    int pelletX = 15 + i * 14;
    // Fall at different speeds
    int fallSpeed = 20 + (i % 3) * 5;
    int pelletY = (elapsed / fallSpeed + i * 4) % 28;
    
    // Check if any fish "ate" the pellet
    bool eaten = false;
    for (int f = 0; f < 3; f++) {
      if (pelletY > fish[f].y - 4 && pelletY < fish[f].y + 4 &&
          pelletX > fish[f].x - 8 && pelletX < fish[f].x + 8) {
        eaten = true;
        break;
      }
    }
    
    if (!eaten) {
      // Draw food pellet
      display.fillCircle(pelletX, pelletY, 2, SSD1306_WHITE);
    }
  }
  
  display.display();
}


void displayMainScreen(DateTime now) {
  display.setCursor(LEFT_MARGIN, 0);
  display.print(F("Czas: "));
  if (rtcPresent) {
    int displayHour = constrainValue(now.hour(), 0, 23);
    int displayMinute = constrainValue(now.minute(), 0, 59);
    int displaySecond = constrainValue(now.second(), 0, 59);
    
    if (displayHour < 10) display.print("0");
    display.print(displayHour);
    display.print(":");
    if (displayMinute < 10) display.print("0");
    display.print(displayMinute);
    display.print(":");
    if (displaySecond < 10) display.print("0");
    display.print(displaySecond);
  } else display.print(F("N/A"));

  display.setCursor(LEFT_MARGIN, 10);
  display.print(F("Data: "));
  if (rtcPresent) {
    if (now.day() < 10) display.print("0");
    display.print(now.day());
    display.print("/");
    if (now.month() < 10) display.print("0");
    display.print(now.month());
    display.print("/");
    display.print(now.year());
  } else display.print(F("N/A"));

  display.setCursor(LEFT_MARGIN, 20);
  display.print(F("Temp: "));
  if (!isnan(currentTemp)) {
    display.print(currentTemp, 1);
    display.print((char)247);
    display.print("C");
  } else display.print(F("N/A"));

  if (quietMode.active) {
    display.setCursor(90, 20);
    display.print(F("[C]"));
  }
  
  if (deviceConnected) {
    display.setCursor(110, 0);
    display.print(F("BT"));
  }
}

void displayMainMenu() {
  display.setCursor(LEFT_MARGIN, 0);
  display.println(F("=== MENU ==="));
  const char* items[4] = {"Karmnik", "Harmonogram", "Cichy", "Ustaw"};
  int start = (menuIndex >= MAX_VISIBLE) ? (menuIndex - (MAX_VISIBLE - 1)) : 0;
  for (int i = start; i < start + MAX_VISIBLE && i < MAIN_MENU_COUNT; ++i) {
    display.setCursor(LEFT_MARGIN, 10 + (i - start) * 10);
    display.print(i == menuIndex ? ">" : " ");
    display.print(items[i]);
  }
}

void displayScheduleMenu() {
  display.setCursor(LEFT_MARGIN, 0);
  display.println(F("HARMONOGRAM"));
  
  int start = (menuIndex >= MAX_VISIBLE) ? (menuIndex - (MAX_VISIBLE - 1)) : 0;
  for (int i = start; i < start + MAX_VISIBLE && i < 5; ++i) {
    display.setCursor(LEFT_MARGIN, 10 + (i - start) * 10);
    display.print(i == menuIndex ? ">" : " ");
    
    int h, m;
    if (i == 0) { 
      display.print(F("Karma ")); 
      h = constrainValue(schedule.feederHour, 0, 23);
      m = constrainValue(schedule.feederMinute, 0, 59);
    }
    else if (i == 1) { 
      display.print(F("Swiatlo+ ")); 
      h = constrainValue(schedule.lightOnHour, 0, 23);
      m = constrainValue(schedule.lightOnMinute, 0, 59);
    }
    else if (i == 2) { 
      display.print(F("Swiatlo- ")); 
      h = constrainValue(schedule.lightOffHour, 0, 23);
      m = constrainValue(schedule.lightOffMinute, 0, 59);
    }
    else if (i == 3) { 
      display.print(F("Filtr+ ")); 
      h = constrainValue(schedule.pumpOnHour, 0, 23);
      m = constrainValue(schedule.pumpOnMinute, 0, 59);
    }
    else { 
      display.print(F("Filtr- ")); 
      h = constrainValue(schedule.pumpOffHour, 0, 23);
      m = constrainValue(schedule.pumpOffMinute, 0, 59);
    }
    
    if (h < 10) display.print("0");
    display.print(h);
    display.print(":");
    if (m < 10) display.print("0");
    display.print(m);
  }
}

void displayManualFeeder() {
  display.setCursor(LEFT_MARGIN, 0);
  display.println(F("KARMNIK"));
  display.setCursor(LEFT_MARGIN, 10);
  display.print(F("Manual: "));
  display.print(manualMode.feederOn ? F("ON") : F("OFF"));
  display.setCursor(LEFT_MARGIN, 20);
  display.print(F("Klik=karm"));
}

void displaySettingsMenu() {
  display.setCursor(LEFT_MARGIN, 0);
  display.println(F("USTAWIENIA"));
  const char* items[5] = {"Czas", "Swiatlo", "Filtr", "Napow", "Temp"};
  int start = (menuIndex >= MAX_VISIBLE) ? (menuIndex - (MAX_VISIBLE - 1)) : 0;
  for (int i = start; i < start + MAX_VISIBLE && i < 5; ++i) {
    display.setCursor(LEFT_MARGIN, 10 + (i - start) * 10);
    display.print(i == menuIndex ? ">" : " ");
    display.print(items[i]);
    if (i == 1) { display.print(":"); display.print(manualMode.lightOn ? F("ON") : F("OFF")); }
    if (i == 2) { display.print(":"); display.print(manualMode.pumpOn ? F("ON") : F("OFF")); }
    if (i == 3) { display.print(":"); display.print(manualMode.servoOn ? F("ON") : F("OFF")); }
    if (i == 4) { 
      display.print(":"); 
      display.print(schedule.targetTemp, 0); 
      display.print((char)247);
      display.print("C");
    }
  }
}

void displayQuietModeMenu() {
  display.setCursor(LEFT_MARGIN, 0);
  display.println(F("TRYB CICHY"));
  display.setCursor(LEFT_MARGIN, 10);
  display.print(menuIndex == 0 ? ">" : " ");
  display.print(F("Status: "));
  display.print(quietMode.active ? F("ON") : F("OFF"));
  display.setCursor(LEFT_MARGIN, 20);
  display.print(menuIndex == 1 ? ">" : " ");
  display.print(F("Czas: "));
  display.print(quietMode.durationMinutes);
  display.print(F("m"));
}

void displayEditSchedule() {
  if (millis() - lastBlinkTime > 300) {
    blinkState = !blinkState;
    lastBlinkTime = millis();
  }
  
  display.setCursor(LEFT_MARGIN, 0);
  if (currentState == SCHEDULE_FEEDER) display.println(F("Karmnik"));
  else if (currentState == SCHEDULE_LIGHT_ON) display.println(F("Swiatlo ON"));
  else if (currentState == SCHEDULE_LIGHT_OFF) display.println(F("Swiatlo OFF"));
  else if (currentState == SCHEDULE_PUMP_ON) display.println(F("Pompa ON"));
  else display.println(F("Pompa OFF"));

  display.setCursor(LEFT_MARGIN, 10);
  display.print(editingHour ? F("Godz:") : F("Min:"));

  display.setCursor(LEFT_MARGIN, 20);
  if (editingHour) {
    if (blinkState) {
      int displayValue = constrainValue(editValue, 0, 23);
      if (displayValue < 10) display.print("0");
      display.print(displayValue);
    } else display.print(F("__"));
    display.print(F(":00"));
  } else {
    int displayHour = constrainValue(tempHour, 0, 23);
    if (displayHour < 10) display.print("0");
    display.print(displayHour);
    display.print(":");
    if (blinkState) {
      int displayValue = constrainValue(editValue, 0, 59);
      if (displayValue < 10) display.print("0");
      display.print(displayValue);
    } else display.print(F("__"));
  }
}

void displaySetTime() {
  if (millis() - lastBlinkTime > 300) {
    blinkState = !blinkState;
    lastBlinkTime = millis();
  }
  
  display.setCursor(LEFT_MARGIN, 0);
  display.print(F("USTAW CZAS"));
  display.setCursor(LEFT_MARGIN, 12);
  display.print(F("Godz: "));
  
  if (editingHour) {
    if (blinkState) {
      int displayValue = constrainValue(editValue, 0, 23);
      if (displayValue < 10) display.print("0");
      display.print(displayValue);
    } else display.print(F("__"));
    display.print(F(":00"));
  } else {
    int displayHour = constrainValue(tempHour, 0, 23);
    if (displayHour < 10) display.print("0");
    display.print(displayHour);
    display.print(":");
    if (blinkState) {
      int displayValue = constrainValue(editValue, 0, 59);
      if (displayValue < 10) display.print("0");
      display.print(displayValue);
    } else display.print(F("__"));
  }
}

void displayEditDuration() {
  if (millis() - lastBlinkTime > 300) {
    blinkState = !blinkState;
    lastBlinkTime = millis();
  }
  
  display.setCursor(LEFT_MARGIN, 0);
  display.println(F("Czas trwania"));
  display.setCursor(LEFT_MARGIN, 15);
  display.print(F("Min: "));
  if (blinkState) {
    int displayValue = constrainValue(editValue, 30, 60);
    display.print(displayValue);
  } else display.print(F("__"));
}

void displayEditServoPosition() {
  if (millis() - lastBlinkTime > 300) {
    blinkState = !blinkState;
    lastBlinkTime = millis();
  }
  
  display.setCursor(LEFT_MARGIN, 0);
  display.println(F("Poz Napow"));
  display.setCursor(LEFT_MARGIN, 15);
  display.print(F("Kat: "));
  if (blinkState) {
    int displayValue = constrainValue(editValue, 0, 90);
    display.print(displayValue);
  } else display.print(F("__"));
  display.print((char)247);
}

void displaySetTargetTemp() {
  if (millis() - lastBlinkTime > 300) {
    blinkState = !blinkState;
    lastBlinkTime = millis();
  }
  
  display.setCursor(LEFT_MARGIN, 0);
  display.println(F("TEMP DOCELOWA"));
  display.setCursor(LEFT_MARGIN, 15);
  display.print(F("Temp: "));
  if (blinkState) {
    int displayValue = constrainValue(editValue, 18, 30);
    display.print(displayValue);
  } else display.print(F("__"));
  display.print((char)247);
  display.print("C");
}

void displayDeviceStatus() {
  display.setCursor(LEFT_MARGIN, 0);
  display.println(F("STATUS"));
  display.setCursor(LEFT_MARGIN, 10);
  display.print(F("Swiatlo: "));
  display.print(manualMode.lightOn ? F("ON") : F("OFF"));
  display.setCursor(LEFT_MARGIN, 20);
  display.print(F("Filtr: "));
  display.print(manualMode.pumpOn ? F("ON") : F("OFF"));
}

void saveEditedValue() {
  switch (currentState) {
    case SCHEDULE_FEEDER:
      schedule.feederHour = constrainValue(tempHour, 0, 23);
      schedule.feederMinute = constrainValue(editValue, 0, 59);
      currentState = SCHEDULE_MENU;
      break;
    case SCHEDULE_LIGHT_ON:
      schedule.lightOnHour = constrainValue(tempHour, 0, 23);
      schedule.lightOnMinute = constrainValue(editValue, 0, 59);
      currentState = SCHEDULE_MENU;
      break;
    case SCHEDULE_LIGHT_OFF:
      schedule.lightOffHour = constrainValue(tempHour, 0, 23);
      schedule.lightOffMinute = constrainValue(editValue, 0, 59);
      currentState = SCHEDULE_MENU;
      break;
    case SCHEDULE_PUMP_ON:
      schedule.pumpOnHour = constrainValue(tempHour, 0, 23);
      schedule.pumpOnMinute = constrainValue(editValue, 0, 59);
      currentState = SCHEDULE_MENU;
      break;
    case SCHEDULE_PUMP_OFF:
      schedule.pumpOffHour = constrainValue(tempHour, 0, 23);
      schedule.pumpOffMinute = constrainValue(editValue, 0, 59);
      currentState = SCHEDULE_MENU;
      break;
    case QUIET_MODE_DURATION:
      quietMode.durationMinutes = constrainValue(editValue, 30, 60);
      currentState = QUIET_MODE_MENU;
      break;
    case MANUAL_SERVO_POSITION:
      servoPosition = constrainValue(editValue, 0, 90);
      manualMode.servoOn = true;
      useCustomServoPosition = true;
      currentState = SETTINGS_MENU;
      break;
    case SET_TARGET_TEMP:
      schedule.targetTemp = (float)constrainValue(editValue, 18, 30);
      currentState = SETTINGS_MENU;
      saveToEEPROM();
      break;
    default: break;
  }
}

void showMsg(const char* msg) {
  unsigned long currentTime = millis();
  
  bool isFeedingMsg = (strcmp(msg, "Karmienie :D!") == 0);
  
  if (statusMsg[0] == '\0' || (safeTimeDiff(currentTime, msgTime) >= 2000)) {
    strncpy(statusMsg, msg, sizeof(statusMsg) - 1);
    statusMsg[sizeof(statusMsg) - 1] = '\0';
    msgTime = currentTime;
    if (isFeedingMsg) {
      feedingMessageActive = true;
    }
  } else if (strcmp(statusMsg, msg) == 0) {
    msgTime = currentTime;
  }
}

void goBackOneLevel() {
  switch (currentState) {
    case MAIN_MENU:
      currentState = MAIN_SCREEN;
      menuIndex = 0;
      showMsg("Glowny");
      break;
    case SCHEDULE_MENU:
      currentState = MAIN_MENU;
      menuIndex = 1;
      showMsg("Menu");
      break;
    case MANUAL_FEEDER:
      currentState = MAIN_MENU;
      menuIndex = 0;
      showMsg("Menu");
      break;
    case SETTINGS_MENU:
      currentState = MAIN_MENU;
      menuIndex = 3;
      showMsg("Menu");
      break;
    case QUIET_MODE_MENU:
      currentState = MAIN_MENU;
      menuIndex = 2;
      showMsg("Menu");
      break;
    case SCHEDULE_FEEDER:
    case SCHEDULE_LIGHT_ON:
    case SCHEDULE_LIGHT_OFF:
    case SCHEDULE_PUMP_ON:
    case SCHEDULE_PUMP_OFF:
      if (editingValue) {
        editingValue = false;
        editingHour = true;
        showMsg("Anulowano");
      } else {
        currentState = SCHEDULE_MENU;
        showMsg("Harmonogram");
      }
      break;
    case SET_TIME:
      if (editingValue) {
        editingValue = false;
        editingHour = true;
        currentState = SETTINGS_MENU;
        showMsg("Anulowano");
      } else {
        currentState = SETTINGS_MENU;
        showMsg("Ustaw");
      }
      break;
    case QUIET_MODE_DURATION:
      if (editingValue) {
        editingValue = false;
        currentState = QUIET_MODE_MENU;
        showMsg("Anulowano");
      } else {
        currentState = QUIET_MODE_MENU;
        showMsg("Cichy");
      }
      break;
    case MANUAL_SERVO_POSITION:
      if (editingValue) {
        editingValue = false;
        currentState = SETTINGS_MENU;
        showMsg("Anulowano");
      } else {
        currentState = SETTINGS_MENU;
        showMsg("Ustaw"); 
      }
      break;
    case SET_TARGET_TEMP:
      if (editingValue) {
        editingValue = false;
        currentState = SETTINGS_MENU;
        showMsg("Anulowano");
      } else {
        currentState = SETTINGS_MENU;
        showMsg("Ustaw");
      }
      break;
    default:
      break;
  }
  lastActivityTime = millis();
}

void showBLEAnimation() {
  display.clearDisplay();
  for (int i = 0; i < 3; i++) {
    display.fillCircle(64, 16, 10 - i * 3, SSD1306_WHITE);
    display.display();
    delay(200);
    display.clearDisplay();
  }
  display.setCursor(30, 12);
  display.print(F("BLE AKTYWNY"));
  display.display();
  delay(1000);
}

void showQuietModeAnimation() {
  display.clearDisplay();
  for (int wave = 0; wave < 3; wave++) {
    for (int x = 0; x < SCREEN_WIDTH; x += 4) {
      int y = 16 + sin((x + wave * 20) * 0.1) * 8;
      display.drawPixel(x, y, SSD1306_WHITE);
    }
    display.display();
    delay(300);
    display.clearDisplay();
  }
  display.setCursor(20, 12);
  display.print(F("TRYB CICHY"));
  display.display();
  delay(1000);
}

// Function to reinitialize BLE
void initBLE() {
  BLEDevice::init("AquariumController");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharTemp = pService->createCharacteristic(CHAR_UUID_TEMP, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pCharTemp->addDescriptor(new BLE2902());

  pCharLight = pService->createCharacteristic(CHAR_UUID_LIGHT, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
  pCharLight->setCallbacks(new LightControlCallbacks());
  pCharLight->addDescriptor(new BLE2902());

  pCharFilter = pService->createCharacteristic(CHAR_UUID_FILTER, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
  pCharFilter->setCallbacks(new FilterControlCallbacks());
  pCharFilter->addDescriptor(new BLE2902());

  pCharHeater = pService->createCharacteristic(CHAR_UUID_HEATER, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
  pCharHeater->setCallbacks(new HeaterControlCallbacks());
  pCharHeater->addDescriptor(new BLE2902());

  pCharFeeder = pService->createCharacteristic(CHAR_UUID_FEEDER, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
  pCharFeeder->setCallbacks(new FeederControlCallbacks());
  pCharFeeder->addDescriptor(new BLE2902());

  pCharServo = pService->createCharacteristic(CHAR_UUID_SERVO, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
  pCharServo->setCallbacks(new ServoControlCallbacks());
  pCharServo->addDescriptor(new BLE2902());

  pCharQuiet = pService->createCharacteristic(CHAR_UUID_QUIET, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
  pCharQuiet->setCallbacks(new QuietModeCallbacks());
  pCharQuiet->addDescriptor(new BLE2902());

  pCharTime = pService->createCharacteristic(CHAR_UUID_TIME, BLECharacteristic::PROPERTY_WRITE);
  pCharTime->setCallbacks(new TimeSyncCallbacks());

  pCharSchLight = pService->createCharacteristic(CHAR_UUID_SCH_LIGHT, BLECharacteristic::PROPERTY_WRITE);
  pCharSchLight->setCallbacks(new ScheduleLightCallbacks());

  pCharSchFilter = pService->createCharacteristic(CHAR_UUID_SCH_FILTER, BLECharacteristic::PROPERTY_WRITE);
  pCharSchFilter->setCallbacks(new ScheduleFilterCallbacks());

  pCharSchFeeder = pService->createCharacteristic(CHAR_UUID_SCH_FEEDER, BLECharacteristic::PROPERTY_WRITE);
  pCharSchFeeder->setCallbacks(new ScheduleFeederCallbacks());

  pCharFeedNow = pService->createCharacteristic(CHAR_UUID_FEED_NOW, BLECharacteristic::PROPERTY_WRITE);
  pCharFeedNow->setCallbacks(new FeedNowCallbacks());

  pCharTargetTemp = pService->createCharacteristic(CHAR_UUID_TARGET_TEMP, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
  pCharTargetTemp->setCallbacks(new TargetTempCallbacks());
  pCharTargetTemp->addDescriptor(new BLE2902());

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
}
