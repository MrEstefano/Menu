#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <Encoder.h>
#include <Wire.h>
#include <RTClib.h>

// TFT display pins
#define TFT_CS     10
#define TFT_RST    9
#define TFT_DC     8

// Rotary encoder pins
#define ENCODER_CLK 2
#define ENCODER_DT  3
#define ENCODER_SW  4

// Initialize TFT display, encoder, and RTC
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
Encoder myEncoder(ENCODER_CLK, ENCODER_DT);
RTC_DS3231 rtc;

int soilMoisture = 0;
int item1Value = 0, item2Value = 0, item3Value = 0, item4Value = 0;
bool item1OnOff = false, item2OnOff = false;
int statusItem1 = 100, statusItem2 = 200;

int menuPosition = 0;
long oldEncoderPosition = -999;
bool buttonPressed = false;

void setup() {
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(3); // Vertical orientation
  tft.fillScreen(ST77XX_BLACK); 
  pinMode(ENCODER_SW, INPUT_PULLUP);

  if (!rtc.begin()) {
    tft.setCursor(0, 0);
    tft.setTextColor(ST77XX_RED);
    tft.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  drawScreen();
}

void loop() {
  updateTopHalf();

  long newEncoderPosition = myEncoder.read() / 4;
  if (newEncoderPosition != oldEncoderPosition) {
    oldEncoderPosition = newEncoderPosition;
    menuPosition = constrain(menuPosition + (newEncoderPosition > oldEncoderPosition ? 1 : -1), 0, 6);
    drawMenu();
  }

  if (digitalRead(ENCODER_SW) == LOW) {
    if (!buttonPressed) {
      buttonPressed = true;
      handleSelection(menuPosition);
      drawMenu();
    }
  } else {
    buttonPressed = false;
  }
}

void updateTopHalf() {
  soilMoisture = analogRead(A0); // Example for soil moisture sensor
  DateTime now = rtc.now();

  tft.fillRect(0, 0, 160, 64, ST77XX_BLACK); // Clear top half of the screen
  tft.setCursor(0, 0);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);

  tft.print("Moisture:");
  tft.println(soilMoisture);  // Display soil moisture
  tft.print(now.hour(), DEC); // Display current time
  tft.print(":");
  tft.print(now.minute(), DEC);
  delay(1000); // Update every second
}

void drawScreen() {
  updateTopHalf();
  drawMenu();
}

void drawMenu() {
  tft.fillRect(0, 64, 160, 64, ST77XX_BLACK); // Clear bottom half of the screen

  tft.setCursor(0, 64);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(2);
  tft.println("MAIN MENU");

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);

  int startIdx = max(0, min(menuPosition - 1, 4));  // Ensure the 3-item window moves properly
  for (int i = 0; i < 3; i++) {
    int idx = startIdx + i;
    tft.setCursor(0, 80 + i * 16);
    if (idx == menuPosition) {
      flashItem(idx); // Flash the selected item
    } else {
      drawMenuItem(idx);
    }
  }
}

void flashItem(int idx) {
  static bool flash = false;
  if (flash) {
    drawMenuItem(idx, ST77XX_WHITE); // Normal color
  } else {
    drawMenuItem(idx, ST77XX_RED);   // Flash color
  }
  flash = !flash;
  delay(300); // Flash rate
}

void drawMenuItem(int idx, uint16_t color = ST77XX_WHITE) {
  tft.setTextColor(color);
  switch (idx) {
    case 0: tft.print("> Item1 Value: " + String(item1Value)); break;
    case 1: tft.print("> Item2 Value: " + String(item2Value)); break;
    case 2: tft.print("> Item3 Value: " + String(item3Value)); break;
    case 3: tft.print("> Item4 Value: " + String(item4Value)); break;
    case 4: tft.print("> Item1 On/Off: " + String(item1OnOff ? "ON" : "OFF")); break;
    case 5: tft.print("> Item2 On/Off: " + String(item2OnOff ? "ON" : "OFF")); break;
    case 6: tft.print("> Status Item1: " + String(statusItem1)); break;
    case 7: tft.print("> Status Item2: " + String(statusItem2)); break;
  }
}

void handleSelection(int menuPos) {
  switch (menuPos) {
    case 0: item1Value = adjustValue(item1Value); break;
    case 1: item2Value = adjustValue(item2Value); break;
    case 2: item3Value = adjustValue(item3Value); break;
    case 3: item4Value = adjustValue(item4Value); break;
    case 4: item1OnOff = !item1OnOff; break;
    case 5: item2OnOff = !item2OnOff; break;
    case 6: showStatus(1); break;
    case 7: showStatus(2); break;
  }
}

int adjustValue(int currentValue) {
  long oldEncoderPosition = myEncoder.read() / 4;
  bool adjustMode = true;
  while (adjustMode) {
    long newEncoderPosition = myEncoder.read() / 4;
    if (newEncoderPosition != oldEncoderPosition) {
      oldEncoderPosition = newEncoderPosition;
      currentValue = constrain(newEncoderPosition, 0, 255);
    }

    // Flash the value while adjusting
    static bool flash = false;
    if (flash) {
      tft.setTextColor(ST77XX_WHITE);
    } else {
      tft.setTextColor(ST77XX_BLACK); // Makes the value "disappear"
    }
    flash = !flash;

    tft.fillRect(0, 20, 160, 20, ST77XX_BLACK); // Clear the previous value
    tft.setCursor(0, 20);
    tft.print("Adjusting Value: "); 
    tft.println(currentValue);

    delay(300); // Flash rate

    // Exit adjust mode when the button is pressed
    if (digitalRead(ENCODER_SW) == LOW) {
      adjustMode = false;
    }
  }
  // Display the final value without flashing
  tft.setTextColor(ST77XX_WHITE);
  tft.fillRect(0, 20, 160, 20, ST77XX_BLACK);
  tft.setCursor(0, 20);
  tft.print("Value Set: "); 
  tft.println(currentValue);
  delay(500);

  return currentValue;
}

void showStatus(int statusItem) {
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 0);
  tft.print("Status Item"); tft.print(statusItem); tft.print(": ");
  tft.println(statusItem == 1 ? statusItem1 : statusItem2);
  delay(2000);
  drawScreen();
}
