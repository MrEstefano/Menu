#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <ClickEncoder.h>
#include <TimerOne.h>

enum MenuItem { CONTRAST, VOLUME, LANGUAGE, DIFFICULTY, BACKLIGHT, RESET, TOTAL_ITEMS };
enum Page { MAIN_MENU, SUB_MENU };

String menuItems[TOTAL_ITEMS] = {"Contrast", "Volume", "Language", "Difficulty", "Light: ON", "Reset"};

boolean backlight = true;
int contrast = 60;
int volume = 50;

String languages[] = { "EN", "ES", "EL" };
int selectedLanguage = 0;

String difficulties[] = { "EASY", "HARD" };
int selectedDifficulty = 0;

int menuitem = CONTRAST;
int frame = 1;
Page page = MAIN_MENU;

bool up = false;
bool down = false;
bool middle = false;
bool lastMiddleState = false;

ClickEncoder *encoder;
int16_t last, value;

Adafruit_PCD8544 display = Adafruit_PCD8544(5, 4, 3);

void setup() {
    pinMode(7, OUTPUT);
    turnBacklightOn();

    encoder = new ClickEncoder(A1, A0, A2);
    encoder->setAccelerationEnabled(true);

    display.begin();
    display.clearDisplay();
    setContrast();

    Timer1.initialize(1000);
    Timer1.attachInterrupt(timerIsr);

    last = encoder->getValue();
}

void loop() {
    handleNavigation();
    drawMenu();
    delay(10); // Small delay for stability, can be adjusted or removed
}

void handleNavigation() {
    if (up || down) {
        navigateMenu();
    }
    if (middle) {
        selectMenuItem();
    }
}

void navigateMenu() {
    int delta = (up) ? -1 : 1;
    up = down = false;

    if (page == MAIN_MENU) {
        menuitem = constrain(menuitem + delta, CONTRAST, RESET);
        adjustFrameForMenu();
    } else {
        adjustSubMenuSetting(delta);
    }
}

void selectMenuItem() {
    middle = false;

    if (page == MAIN_MENU) {
        if (menuitem == BACKLIGHT) {
            toggleBacklight();
        } else if (menuitem == RESET) {
            resetDefaults();
        } else {
            page = SUB_MENU;
        }
    } else {
        page = MAIN_MENU;
    }
}

void adjustFrameForMenu() {
    static const int frameMapping[TOTAL_ITEMS] = {1, 1, 1, 2, 3, 4};
    frame = frameMapping[menuitem];
}

void adjustSubMenuSetting(int delta) {
    switch (menuitem) {
        case CONTRAST:
            contrast = constrain(contrast + delta, 0, 100);
            setContrast();
            break;
        case VOLUME:
            volume = constrain(volume + delta, 0, 100);
            break;
        case LANGUAGE:
            selectedLanguage = (selectedLanguage + delta + 3) % 3;
            break;
        case DIFFICULTY:
            selectedDifficulty = (selectedDifficulty + delta + 2) % 2;
            break;
    }
}

void drawMenu() {
    display.setTextSize(1);
    display.clearDisplay();
    display.setTextColor(BLACK, WHITE);
    display.setCursor(15, 0);
    display.print(page == MAIN_MENU ? "MAIN MENU" : menuItems[menuitem]);
    display.drawFastHLine(0, 10, 83, BLACK);

    if (page == MAIN_MENU) {
        drawMainMenu();
    } else {
        drawSubMenu();
    }

    display.display();
}

void drawMainMenu() {
    int startItem = frame - 1;
    for (int i = 0; i < 3 && startItem + i < TOTAL_ITEMS; i++) {
        displayMenuItem(menuItems[startItem + i], 15 + i * 10, menuitem == startItem + i);
    }
}

void drawSubMenu() {
    if (menuitem == CONTRAST || menuitem == VOLUME) {
        displayIntMenuPage(menuItems[menuitem], menuitem == CONTRAST ? contrast : volume);
    } else if (menuitem == LANGUAGE || menuitem == DIFFICULTY) {
        displayStringMenuPage(menuItems[menuitem], menuitem == LANGUAGE ? languages[selectedLanguage] : difficulties[selectedDifficulty]);
    }
}

void toggleBacklight() {
    backlight = !backlight;
    menuItems[BACKLIGHT] = backlight ? "Light: ON" : "Light: OFF";
    backlight ? turnBacklightOn() : turnBacklightOff();
}

void resetDefaults() {
    contrast = 60;
    volume = 50;
    selectedLanguage = 0;
    selectedDifficulty = 0;
    setContrast();
    backlight = true;
    menuItems[BACKLIGHT] = "Light: ON";
    turnBacklightOn();
}

void setContrast() {
    display.setContrast(contrast);
}

void turnBacklightOn() {
    digitalWrite(7, LOW);
}

void turnBacklightOff() {
    digitalWrite(7, HIGH);
}

void timerIsr() {
    encoder->service();

    // Update the rotary encoder's state
    int16_t newValue = encoder->getValue();
    if (newValue / 2 > last) {
        last = newValue / 2;
        down = true;
    } else if (newValue / 2 < last) {
        last = newValue / 2;
        up = true;
    }

    // Debounce and check middle button state
    ClickEncoder::Button buttonState = encoder->getButton();
    if (buttonState == ClickEncoder::Clicked && !lastMiddleState) {
        middle = true;
    }
    lastMiddleState = (buttonState == ClickEncoder::Held || buttonState == ClickEncoder::Clicked);
}

void displayIntMenuPage(String menuItem, int value) {
    display.setTextSize(1);
    display.clearDisplay();
    display.setTextColor(BLACK, WHITE);
    display.setCursor(15, 0);
    display.print(menuItem);
    display.drawFastHLine(0, 10, 83, BLACK);
    display.setCursor(5, 15);
    display.print("Value");
    display.setTextSize(2);
    display.setCursor(5, 25);
    display.print(value);
}

void displayStringMenuPage(String menuItem, String value) {
    display.setTextSize(1);
    display.clearDisplay();
    display.setTextColor(BLACK, WHITE);
    display.setCursor(15, 0);
    display.print(menuItem);
    display.drawFastHLine(0, 10, 83, BLACK);
    display.setCursor(5, 15);
    display.print("Value");
    display.setTextSize(2);
    display.setCursor(5, 25);
    display.print(value);
}

void displayMenuItem(String item, int position, bool selected) {
    display.setTextColor(selected ? WHITE : BLACK, selected ? BLACK : WHITE);
    display.setCursor(0, position);
    display.print(">" + item);
}

void readRotaryEncoder() {
    // Functionality now handled within timerIsr()
}
