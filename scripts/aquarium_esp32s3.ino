#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RTClib.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP32Servo.h>
#include <Preferences.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// ================= BLE CONFIG =================
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHAR_TEMP_UUID      "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHAR_CONTROL_UUID   "1c95d5e3-d8f7-413a-bf3d-7a2e5d7be87e"
#define CHAR_SCHEDULE_UUID  "d8de624e-140f-4a22-8594-e2216b84a188"
#define CHAR_STATUS_UUID    "f27b53ad-c63d-49a0-8c0f-9f297e6cd3a2"
#define CHAR_TIME_UUID      "a3c87500-8ed3-4bdf-8a39-a01bebede295"

BLEServer* pServer = NULL;
BLECharacteristic* pTempChar = NULL;
BLECharacteristic* pControlChar = NULL;
BLECharacteristic* pScheduleChar = NULL;
BLECharacteristic* pStatusChar = NULL;
BLECharacteristic* pTimeChar = NULL;
bool deviceConnected = false;

// ================= HARDWARE CONFIG =================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

RTC_DS1307 rtc;
Preferences preferences;

// ESP32-S3 Zero Pin Definitions
const int BUTTON_PIN = 0;        // Boot button for display wake
const int ONE_WIRE_BUS = 4;      // Temperature sensor
const int SERVO_PIN = 5;         // Servo for aeration
const int LIGHT_PIN = 6;         // Light relay
const int PUMP_PIN = 7;          // Pump/Filter relay
const int HEATER_PIN = 8;        // Heater relay
const int FEEDER_PIN = 9;        // Feeder relay

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
float currentTemp = NAN;

Servo servoMotor;
int servoPosition = 0;
int lastServoPosition = -1;

const unsigned long FEEDER_PULSE_SEC = 5;
bool feederActive = false;
unsigned long feederStartMillis = 0;
long lastFeederTriggerDayMinute = -1;

const unsigned long DISPLAY_TIMEOUT = 30000; // 30s display timeout
unsigned long lastDisplayActivity = 0;
bool displayActive = false;

// ================= DATA STRUCTURES =================
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

bool rtcPresent = false;
bool dsPresent = false;
char statusMessage[32] = "";
bool isFeedingNow = false;

// ================= BLE CALLBACKS =================
class ServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("BLE Connected");
  }
  
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("BLE Disconnected");
    BLEDevice::startAdvertising();
  }
};

class ControlCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0) {
      String cmd = String(value.c_str());
      handleBLECommand(cmd);
    }
  }
};

class ScheduleCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0) {
      String data = String(value.c_str());
      handleScheduleUpdate(data);
    }
  }
};

class TimeCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0) {
      String timeData = String(value.c_str());
      handleTimeSync(timeData);
    }
  }
};

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  Wire.begin();
  
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LIGHT_PIN, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(HEATER_PIN, OUTPUT);
  pinMode(FEEDER_PIN, OUTPUT);
  
  digitalWrite(LIGHT_PIN, LOW);
  digitalWrite(PUMP_PIN, LOW);
  digitalWrite(HEATER_PIN, LOW);
  digitalWrite(FEEDER_PIN, LOW);

  // Initialize display
  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println(F("Sterownik Akwarium"));
    display.setCursor(0, 16);
    display.println(F("Bartosz Wolny 2025"));
    display.display();
    delay(2000);
  }

  // Initialize RTC
  rtcPresent = rtc.begin();
  if (rtcPresent && !rtc.isrunning()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Initialize temperature sensor
  sensors.begin();
  dsPresent = (sensors.getDeviceCount() > 0);

  // Initialize preferences (EEPROM replacement)
  preferences.begin("aquarium", false);
  loadSchedule();

  // Initialize BLE
  initBLE();

  // Initialize servo
  servoMotor.attach(SERVO_PIN);
  servoMotor.write(90); // Off position
  delay(500);
  servoMotor.detach();

  displayActive = true;
  lastDisplayActivity = millis();
  
  Serial.println("Aquarium Controller Ready");
}

// ================= MAIN LOOP =================
void loop() {
  DateTime now = rtcPresent ? rtc.now() : DateTime(2025,1,1,12,0,0);
  
  // Read temperature
  if (dsPresent) {
    sensors.requestTemperatures();
    currentTemp = sensors.getTempCByIndex(0);
  }

  // Handle button for display wake
  handleButton();
  
  // Update devices based on schedule and manual modes
  updateDevices(now);
  
  // Update display
  updateDisplay(now);
  
  // Send BLE updates
  if (deviceConnected) {
    sendBLEUpdates(now);
  }

  // Handle display timeout
  if (displayActive && (millis() - lastDisplayActivity > DISPLAY_TIMEOUT)) {
    displayActive = false;
    display.clearDisplay();
    display.display();
  }

  delay(100);
}

// ================= BLE FUNCTIONS =================
void initBLE() {
  BLEDevice::init("AquariumController");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Temperature characteristic (read/notify)
  pTempChar = pService->createCharacteristic(
    CHAR_TEMP_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );
  pTempChar->addDescriptor(new BLE2902());

  // Control characteristic (read/write)
  pControlChar = pService->createCharacteristic(
    CHAR_CONTROL_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
  );
  pControlChar->setCallbacks(new ControlCallbacks());

  // Schedule characteristic (read/write)
  pScheduleChar = pService->createCharacteristic(
    CHAR_SCHEDULE_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
  );
  pScheduleChar->setCallbacks(new ScheduleCallbacks());

  // Status characteristic (read/notify)
  pStatusChar = pService->createCharacteristic(
    CHAR_STATUS_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );
  pStatusChar->addDescriptor(new BLE2902());

  // Time sync characteristic (write)
  pTimeChar = pService->createCharacteristic(
    CHAR_TIME_UUID,
    BLECharacteristic::PROPERTY_WRITE
  );
  pTimeChar->setCallbacks(new TimeCallbacks());

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  
  Serial.println("BLE Advertising started");
}

void handleBLECommand(String cmd) {
  Serial.println("BLE Command: " + cmd);
  
  if (cmd.startsWith("LIGHT:")) {
    manualMode.lightOn = cmd.substring(6).toInt();
  }
  else if (cmd.startsWith("PUMP:")) {
    manualMode.pumpOn = cmd.substring(5).toInt();
  }
  else if (cmd.startsWith("HEATER:")) {
    manualMode.heaterOn = cmd.substring(7).toInt();
  }
  else if (cmd.startsWith("SERVO:")) {
    int pos = cmd.substring(6).toInt();
    servoPosition = constrain(pos, 0, 90);
    manualMode.servoOn = (pos == 0);
  }
  else if (cmd.startsWith("FEED")) {
    triggerFeeding();
  }
  else if (cmd.startsWith("QUIET:")) {
    String params = cmd.substring(6);
    int separatorIndex = params.indexOf(',');
    if (separatorIndex > 0) {
      quietMode.active = params.substring(0, separatorIndex).toInt();
      quietMode.durationMinutes = params.substring(separatorIndex + 1).toInt();
      if (quietMode.active) {
        quietMode.startTime = millis();
      }
    }
  }
  
  wakeDisplay();
}

void handleScheduleUpdate(String data) {
  Serial.println("Schedule Update: " + data);
  
  // Format: TYPE:ON_H:ON_M:OFF_H:OFF_M
  // Example: LIGHT:8:0:20:0
  
  int firstColon = data.indexOf(':');
  if (firstColon < 0) return;
  
  String type = data.substring(0, firstColon);
  String params = data.substring(firstColon + 1);
  
  int values[4];
  int valueIndex = 0;
  int lastIndex = 0;
  
  for (int i = 0; i < params.length() && valueIndex < 4; i++) {
    if (params.charAt(i) == ':' || i == params.length() - 1) {
      if (i == params.length() - 1) i++;
      values[valueIndex++] = params.substring(lastIndex, i).toInt();
      lastIndex = i + 1;
    }
  }
  
  if (type == "LIGHT") {
    schedule.lightOnHour = constrain(values[0], 0, 23);
    schedule.lightOnMinute = constrain(values[1], 0, 59);
    schedule.lightOffHour = constrain(values[2], 0, 23);
    schedule.lightOffMinute = constrain(values[3], 0, 59);
  }
  else if (type == "PUMP") {
    schedule.pumpOnHour = constrain(values[0], 0, 23);
    schedule.pumpOnMinute = constrain(values[1], 0, 59);
    schedule.pumpOffHour = constrain(values[2], 0, 23);
    schedule.pumpOffMinute = constrain(values[3], 0, 59);
  }
  else if (type == "FEEDER") {
    schedule.feederHour = constrain(values[0], 0, 23);
    schedule.feederMinute = constrain(values[1], 0, 59);
  }
  
  saveSchedule();
  wakeDisplay();
}

void handleTimeSync(String timeData) {
  // Format: YYYY:MM:DD:HH:MM:SS
  Serial.println("Time Sync: " + timeData);
  
  int values[6];
  int valueIndex = 0;
  int lastIndex = 0;
  
  for (int i = 0; i < timeData.length() && valueIndex < 6; i++) {
    if (timeData.charAt(i) == ':' || i == timeData.length() - 1) {
      if (i == timeData.length() - 1) i++;
      values[valueIndex++] = timeData.substring(lastIndex, i).toInt();
      lastIndex = i + 1;
    }
  }
  
  if (valueIndex == 6 && rtcPresent) {
    rtc.adjust(DateTime(values[0], values[1], values[2], values[3], values[4], values[5]));
    Serial.println("Time synchronized");
  }
  
  wakeDisplay();
}

void sendBLEUpdates(DateTime now) {
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate < 1000) return; // Update every second
  lastUpdate = millis();
  
  // Send temperature
  if (!isnan(currentTemp)) {
    String tempStr = String(currentTemp, 1);
    pTempChar->setValue(tempStr.c_str());
    pTempChar->notify();
  }
  
  // Send status
  String status = String(digitalRead(LIGHT_PIN)) + "," +
                  String(digitalRead(PUMP_PIN)) + "," +
                  String(digitalRead(HEATER_PIN)) + "," +
                  String(manualMode.servoOn ? 1 : 0) + "," +
                  String(isFeedingNow ? 1 : 0) + "," +
                  String(quietMode.active ? 1 : 0);
  pStatusChar->setValue(status.c_str());
  pStatusChar->notify();
}

// ================= DEVICE CONTROL =================
void updateDevices(DateTime now) {
  // Check quiet mode timeout
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
    if (currentTemp < schedule.targetTemp - 0.5) heaterState = true;
    else if (currentTemp >= schedule.targetTemp + 0.5) heaterState = false;
    heaterShouldBeOn = heaterState;
  }
  digitalWrite(HEATER_PIN, heaterShouldBeOn ? HIGH : LOW);

  // Servo/Aeration control
  int targetPos;
  if (quietMode.active) {
    targetPos = 90; // Off
  } else {
    // Turn off 30 minutes before lights off
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
    
    if (shouldTurnOffAeration) {
      targetPos = 90;
    } else if (manualMode.servoOn) {
      targetPos = servoPosition;
    } else {
      targetPos = 0; // Default on
    }
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
      isFeedingNow = false;
      digitalWrite(FEEDER_PIN, LOW);
      strcpy(statusMessage, "");
    }
  }

  // Scheduled feeding
  if (rtcPresent) {
    long curDayMinute = (long)now.day() * 1440L + (long)now.hour() * 60L + (long)now.minute();
    long schedDayMinute = (long)now.day() * 1440L + (long)schedule.feederHour * 60L + (long)schedule.feederMinute;
    
    if (curDayMinute == schedDayMinute && lastFeederTriggerDayMinute != curDayMinute) {
      triggerFeeding();
      lastFeederTriggerDayMinute = curDayMinute;
    }
  }
}

void triggerFeeding() {
  feederActive = true;
  isFeedingNow = true;
  feederStartMillis = millis();
  digitalWrite(FEEDER_PIN, HIGH);
  strcpy(statusMessage, "Karmienie...");
  wakeDisplay();
}

// ================= DISPLAY FUNCTIONS =================
void handleButton() {
  static bool lastButtonState = HIGH;
  static unsigned long lastDebounce = 0;
  
  bool reading = digitalRead(BUTTON_PIN);
  
  if (reading != lastButtonState) {
    lastDebounce = millis();
  }
  
  if ((millis() - lastDebounce) > 50) {
    if (reading == LOW && lastButtonState == HIGH) {
      wakeDisplay();
    }
  }
  
  lastButtonState = reading;
}

void wakeDisplay() {
  displayActive = true;
  lastDisplayActivity = millis();
}

void updateDisplay(DateTime now) {
  if (!displayActive) return;
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // Line 1: Time
  display.setCursor(0, 0);
  display.print(F("Czas: "));
  if (rtcPresent) {
    if (now.hour() < 10) display.print("0");
    display.print(now.hour());
    display.print(":");
    if (now.minute() < 10) display.print("0");
    display.print(now.minute());
    display.print(":");
    if (now.second() < 10) display.print("0");
    display.print(now.second());
  } else {
    display.print(F("--:--:--"));
  }
  
  // Line 2: Date
  display.setCursor(0, 12);
  display.print(F("Data: "));
  if (rtcPresent) {
    if (now.day() < 10) display.print("0");
    display.print(now.day());
    display.print("/");
    if (now.month() < 10) display.print("0");
    display.print(now.month());
    display.print("/");
    display.print(now.year());
  } else {
    display.print(F("--/--/----"));
  }
  
  // Line 3: Temperature
  display.setCursor(0, 24);
  display.print(F("Temp: "));
  if (!isnan(currentTemp)) {
    display.print(currentTemp, 1);
    display.print((char)247);
    display.print("C");
  } else {
    display.print(F("--.-"));
    display.print((char)247);
    display.print("C");
  }
  
  // Line 4: Device Status
  display.setCursor(0, 36);
  display.print(F("Status: "));
  if (digitalRead(LIGHT_PIN)) display.print(F("S"));
  else display.print(F("-"));
  if (digitalRead(PUMP_PIN)) display.print(F("F"));
  else display.print(F("-"));
  if (digitalRead(HEATER_PIN)) display.print(F("G"));
  else display.print(F("-"));
  if (lastServoPosition == 0) display.print(F("N"));
  else display.print(F("-"));
  
  // Line 5: Special messages
  display.setCursor(0, 48);
  if (isFeedingNow) {
    display.print(F(">>> KARMIENIE <<<"));
  } else if (quietMode.active) {
    display.print(F("Tryb cichy: "));
    unsigned long remaining = quietMode.durationMinutes - ((millis() - quietMode.startTime) / 60000);
    display.print(remaining);
    display.print(F("min"));
  } else if (strlen(statusMessage) > 0) {
    display.print(statusMessage);
  } else if (deviceConnected) {
    display.print(F("BLE: Polaczony"));
  }
  
  display.display();
}

// ================= STORAGE FUNCTIONS =================
void saveSchedule() {
  preferences.putInt("feedH", schedule.feederHour);
  preferences.putInt("feedM", schedule.feederMinute);
  preferences.putInt("lightOnH", schedule.lightOnHour);
  preferences.putInt("lightOnM", schedule.lightOnMinute);
  preferences.putInt("lightOffH", schedule.lightOffHour);
  preferences.putInt("lightOffM", schedule.lightOffMinute);
  preferences.putInt("pumpOnH", schedule.pumpOnHour);
  preferences.putInt("pumpOnM", schedule.pumpOnMinute);
  preferences.putInt("pumpOffH", schedule.pumpOffHour);
  preferences.putInt("pumpOffM", schedule.pumpOffMinute);
  preferences.putFloat("targetT", schedule.targetTemp);
  Serial.println("Schedule saved");
}

void loadSchedule() {
  schedule.feederHour = preferences.getInt("feedH", 12);
  schedule.feederMinute = preferences.getInt("feedM", 0);
  schedule.lightOnHour = preferences.getInt("lightOnH", 8);
  schedule.lightOnMinute = preferences.getInt("lightOnM", 0);
  schedule.lightOffHour = preferences.getInt("lightOffH", 20);
  schedule.lightOffMinute = preferences.getInt("lightOffM", 0);
  schedule.pumpOnHour = preferences.getInt("pumpOnH", 7);
  schedule.pumpOnMinute = preferences.getInt("pumpOnM", 0);
  schedule.pumpOffHour = preferences.getInt("pumpOffH", 22);
  schedule.pumpOffMinute = preferences.getInt("pumpOffM", 0);
  schedule.targetTemp = preferences.getFloat("targetT", 24.0);
  Serial.println("Schedule loaded");
}
