#include <U8g2lib.h>
#include <Wire.h>
#include <math.h>

U8G2_SH1106_128X64_NONAME_2_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define DEG60 1.0472
#define DEG90 1.5708
#define DEG2 0.0349066
#define NEGLIGIBLE_DEG 10
#define LINE_HEIGHT 9
#define GROUPCHAT_LINES 3
#define GROUPCHAT_TOP_PADDING 5
#define MORSE_TOP 2
#define LEFT_PADDING 4

// ---------- Constants & Data ----------
const char* mainMenuItems[] = {
  "/groupchat",
  "/chat",
  "/locations",
  "/SOS",
  "/compass"
};

const int mainMenuSize = sizeof(mainMenuItems) / sizeof(mainMenuItems[0]);  // Keep this here


const char* chatUsers[] = { "Alice", "Bob", "Charlie" };

const char* groupMessages[][2] = {
  {"Alice", "Hi everyone!"},
  {"Bob", "Hello Alice"},
  {"Charlie", "Where are you all?"}
};

const char* privateChat[][2] = {
  {"You", "Hey Bob"},
  {"Bob", "Yo, what's up?"},
  {"You", "Need location"},
  {"Bob", "Check now"}
};
struct Location {
  const char* name;
  float lat;
  float lon;
  unsigned long lastUpdated;
};

Location locationList[] = {
  {"Alice", 17.385, 78.486, 1690000000},
  {"Bob", 17.400, 78.480, 1690000500},
  {"Charlie", 17.390, 78.485, 1690001000}
};


const int userCount = sizeof(chatUsers) / sizeof(chatUsers[0]);

float myLat = 17.385;
float myLon = 78.486;

// ---------- State Management ----------
enum Screen {
  SCREEN_MAIN,
  SCREEN_GROUPCHAT,
  SCREEN_CHAT_LIST,
  SCREEN_CHAT_PERSON,
  SCREEN_LOCATION_LIST,
  SCREEN_LOCATION_PERSON,
  SCREEN_COMPASS
};

Screen screen = SCREEN_MAIN;

bool oledDimmed = false;
bool oledOn = true;
unsigned long lastInteractionTime = 0;

int selectedIndex = 0;
int chatUserIndex = 0;
int locationUserIndex = 0;

// ---------- Morse Code ----------
#define TOUCH_MORSE 15
String morseInput = "";
String currentText = "";
unsigned long morseStartTime = 0;
bool morseActive = false;

String decodeMorse(String morse) {
  if (morse == ".-") return "A";
  if (morse == "-...") return "B";
  if (morse == "-.-.") return "C";
  if (morse == "-..") return "D";
  if (morse == ".") return "E";
  if (morse == "..-.") return "F";
  if (morse == "--.") return "G";
  if (morse == "....") return "H";
  if (morse == "..") return "I";
  if (morse == ".---") return "J";
  if (morse == "-.-") return "K";
  if (morse == ".-..") return "L";
  if (morse == "--") return "M";
  if (morse == "-.") return "N";
  if (morse == "---") return "O";
  if (morse == ".--.") return "P";
  if (morse == "--.-") return "Q";
  if (morse == ".-.") return "R";
  if (morse == "...") return "S";
  if (morse == "-") return "T";
  if (morse == "..-") return "U";
  if (morse == "...-") return "V";
  if (morse == ".--") return "W";
  if (morse == "-..-") return "X";
  if (morse == "-.--") return "Y";
  if (morse == "--..") return "Z";
  return "";
}

// ---------- Helpers ----------
float calculateDistance(float lat1, float lon1, float lat2, float lon2) {
  float dx = (lat2 - lat1) * 111000;
  float dy = (lon2 - lon1) * 111000;
  return sqrt(dx * dx + dy * dy);
}

float calculateBearing(float lat1, float lon1, float lat2, float lon2) {
  float dx = lat2 - lat1;
  float dy = lon2 - lon1;
  return atan2(dy, dx);
}
#define COMPASS_PADDING_CIRCLE 8
#define COMPASS_ARROW_FRONT_LEN 5
#define COMPASS_ARROW_BACK_LEN 7
#define COMPASS_ARROW_HEAD_DIFF 10
#define COMPASS_ARROW_HEADLEN 7
#define COMPASS_HEADANGLE DEG90

#define LOC_PADDING_CIRCLE 7
#define LOC_ARROW_FRONT_LEN 3
#define LOC_ARROW_BACK_LEN 6
#define LOC_ARROW_HEAD_DIFF 7
#define LOC_ARROW_HEADLEN 6
#define LOC_HEADANGLE DEG90
void drawArrow(int cx, int cy, float angle, float frontlen, float backlen, float arrow_head_diff, float headAngle, float headLen, float padding_circle) {

  auto diff = frontlen + arrow_head_diff;

  int x1 = cx - cos(angle) * backlen;
  int y1 = cy - sin(angle) * backlen;
  int x2 = cx + cos(angle) * frontlen;
  int y2 = cy + sin(angle) * frontlen;
  int hox = cx + cos(angle) * (diff);
  int hoy = cy + sin(angle) * (diff);

  // Main line (extends both ways from center)
  u8g2.drawLine(x1, y1, x2, y2);

  // Arrowhead at the end (forward direction only)

  int x3 = x2 - cos(angle - headAngle) * headLen;
  int y3 = y2 - sin(angle - headAngle) * headLen;
  int x4 = x2 - cos(angle + headAngle) * headLen;
  int y4 = y2 - sin(angle + headAngle) * headLen;

  u8g2.drawLine(hox, hoy, x3, y3);
  u8g2.drawLine(hox, hoy, x4, y4);

  // Circle around the arrow
  u8g2.drawCircle(cx, cy, diff + padding_circle);  // Slightly larger than arrow length
}

#define COMPASS_W 20
void drawCompass() {
  static int i = 0;
  render();
  if (((i++) % 10) != 0)return;

  float fakeHeading = (millis()/70 % 360 );  // Just rotates for demo
  int realHeading = ((int)fakeHeading + 90) % 360;
  float angle = radians(fakeHeading);  // Convert to radians
  u8g2.firstPage();
  do {
    u8g2.setCursor(LEFT_PADDING, 12);
    u8g2.print("COMPASS");
    u8g2.setCursor(LEFT_PADDING, 24);
    u8g2.print("MODE");
    if (realHeading < NEGLIGIBLE_DEG || realHeading > (360 - NEGLIGIBLE_DEG)) {
 
      u8g2.drawDisc((SCREEN_WIDTH) / 2, (SCREEN_HEIGHT) / 2, COMPASS_W);
      u8g2.setDrawColor(0);
      u8g2.setCursor(SCREEN_WIDTH / 2  , SCREEN_HEIGHT / 2 );
      u8g2.print("N");
      u8g2.setDrawColor(1);

    }
    else
      drawArrow(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, angle, COMPASS_ARROW_FRONT_LEN, COMPASS_ARROW_BACK_LEN, COMPASS_ARROW_HEAD_DIFF, COMPASS_HEADANGLE, COMPASS_ARROW_HEADLEN, COMPASS_PADDING_CIRCLE);
    //    drawArrow(int cx,int cy,float angle,int len,int arrow_head_diff,float headAngle,int headlen,int padding_circle)
    u8g2.setCursor(SCREEN_WIDTH - 40 - LEFT_PADDING, SCREEN_HEIGHT - LINE_HEIGHT / 2);
    u8g2.print(realHeading);
    u8g2.print(" deg");
  } while (u8g2.nextPage());

}

// ---------- UI Drawing ----------
void drawMenu(const char* const* items, int size, int selected) {
  u8g2.firstPage();
  do {
    const int leftPadding = 4;
    const int menuWidth = SCREEN_WIDTH - 1 - leftPadding;

    for (int i = 0; i < size; ++i) {
      if (i == selected) u8g2.drawBox(leftPadding, i * 12, menuWidth - leftPadding, 12);
      u8g2.setDrawColor(i == selected ? 0 : 1);
      u8g2.setCursor(leftPadding + 2, 12 + i * 12);
      u8g2.print(items[i]);
      u8g2.setDrawColor(1);
    }

  } while (u8g2.nextPage());
}
void drawGroupChat() {
  u8g2.firstPage();
  do {
    for (int i = 0; i < GROUPCHAT_LINES; i++) {
      u8g2.setCursor(0, GROUPCHAT_TOP_PADDING + (1 + i) * LINE_HEIGHT);
      u8g2.print(groupMessages[i][0]);
      u8g2.print(": ");
      u8g2.print(groupMessages[i][1]);
    }

    u8g2.setCursor(0, MORSE_TOP);
    u8g2.print(morseInput.c_str());
    u8g2.drawBox(LEFT_PADDING, SCREEN_HEIGHT - LINE_HEIGHT, SCREEN_WIDTH - 1 - LEFT_PADDING, LINE_HEIGHT);
    u8g2.setDrawColor(0);
    u8g2.setCursor(LEFT_PADDING,  SCREEN_HEIGHT);
    u8g2.print('>');
    u8g2.print(currentText.c_str());
    u8g2.setDrawColor(1);
  } while (u8g2.nextPage());

  // Show morseInput on top line

}

void drawChatWithUser() {
  u8g2.firstPage();
  do {
    for (int i = 0; i < 4; i++) {
      const char* speaker = privateChat[i][0];
      const char* message = privateChat[i][1];
      if (strcmp(speaker, "You") == 0) {
        u8g2.setDrawColor(0);
        u8g2.drawBox(LEFT_PADDING, i * 16, SCREEN_WIDTH - 1 - LEFT_PADDING, 16);
        u8g2.setDrawColor(1);
        u8g2.setCursor(LEFT_PADDING, i * 16 + 12);
        u8g2.print("You: ");
        u8g2.print(message);
        u8g2.setDrawColor(1);
      } else {
        u8g2.setCursor(LEFT_PADDING, i * 16 + 12);
        u8g2.print(speaker);
        u8g2.print(": ");
        u8g2.print(message);
      }
    }
    // Show morseInput on top line
    u8g2.setCursor(0, MORSE_TOP);
    u8g2.print(morseInput.c_str());
    u8g2.drawBox(LEFT_PADDING, SCREEN_HEIGHT - LINE_HEIGHT, SCREEN_WIDTH - 1 - LEFT_PADDING, LINE_HEIGHT);
    u8g2.setDrawColor(0);
    u8g2.setCursor(LEFT_PADDING,  SCREEN_HEIGHT);
    u8g2.print('>');
    u8g2.print(currentText.c_str());
    u8g2.setDrawColor(1);

  } while (u8g2.nextPage());

}

void drawLocationPerson() {
  Location person = locationList[locationUserIndex];

  // Randomize target's location slightly each time
  locationList[locationUserIndex].lat += random(-10, 11) * 0.1;
  locationList[locationUserIndex].lon += random(-10, 11) * 0.1;

  float dist = calculateDistance(myLat, myLon, person.lat, person.lon);
  float angle = calculateBearing(myLat, myLon, person.lat, person.lon);

  u8g2.firstPage();
  do {
    u8g2.setCursor(0, 12);
    u8g2.print("User: "); u8g2.print(person.name);
    u8g2.setCursor(0, 24);
    u8g2.print("Lat: "); u8g2.print(person.lat, 3);
    u8g2.setCursor(0, 36);
    u8g2.print("Lon: "); u8g2.print(person.lon, 3);
    u8g2.setCursor(0, 48);
    u8g2.print("Dist: "); u8g2.print(dist, 0); u8g2.print("m");
    drawArrow(SCREEN_WIDTH - 30, SCREEN_HEIGHT / 2, angle, LOC_ARROW_FRONT_LEN, LOC_ARROW_BACK_LEN, LOC_ARROW_HEAD_DIFF, LOC_HEADANGLE, LOC_ARROW_HEADLEN, LOC_PADDING_CIRCLE);
    u8g2.setCursor(0, 60);
    u8g2.print("Updated: ");
    u8g2.print(person.lastUpdated);

  } while (u8g2.nextPage());
}
void sendSOS() {
  Serial.println("[SOS] Emergency signal sent!");
  // Add LoRa or other comms logic here later if needed

  u8g2.firstPage();
  do {
    u8g2.setCursor(LEFT_PADDING, SCREEN_HEIGHT / 2 - LINE_HEIGHT / 2);
    u8g2.print("SENDING SOS...");
  } while (u8g2.nextPage());
  delay(2000);
  screen = SCREEN_MAIN;
  render();

}


int newhash = 0, oldhash = 1;
void render() {
  do {
    newhash = random(1, 100000);
  } while (newhash == oldhash);
}

void updateScreen() {
  if ( newhash == oldhash) {
    return;
  }
  oldhash = newhash;
  switch (screen) {
    case SCREEN_MAIN:
      drawMenu(mainMenuItems, mainMenuSize, selectedIndex); break;
    case SCREEN_GROUPCHAT:
      drawGroupChat(); break;
    case SCREEN_CHAT_LIST:
      drawMenu(chatUsers, userCount, chatUserIndex); break;
    case SCREEN_CHAT_PERSON:
      drawChatWithUser(); break;
    case SCREEN_LOCATION_LIST:
      drawMenu(chatUsers, userCount, locationUserIndex); break;
    case SCREEN_LOCATION_PERSON:
      drawLocationPerson(); break;
    case SCREEN_COMPASS:
      drawCompass(); break;

  }
}

#define TOUCH_NEXT 13
#define TOUCH_BACK 4
#define TOUCH_THRESHOLD 50

#define SHORT_THRESHOLD 300

unsigned long touchStartTime = 0;
bool touchActive = false;

void markInteraction() {
  if (millis() - lastInteractionTime > 0) {
    lastInteractionTime = millis();  // update on any check
  }

}
void handleInput() {

  int valNext = touchRead(TOUCH_NEXT);
  int valBack = touchRead(TOUCH_BACK);
  int valMorse = touchRead(TOUCH_MORSE);
  if (valNext < TOUCH_THRESHOLD || valBack < TOUCH_THRESHOLD || valMorse < TOUCH_THRESHOLD)
    markInteraction();
  if (valBack < TOUCH_THRESHOLD && valNext < TOUCH_THRESHOLD && (screen == SCREEN_GROUPCHAT || screen == SCREEN_CHAT_PERSON) ) {
    screen = SCREEN_MAIN;
    morseInput = "";
    currentText = "";
    render();
    return;
  }

  if ((screen == SCREEN_GROUPCHAT || screen == SCREEN_CHAT_PERSON) && valMorse < TOUCH_THRESHOLD) {
    if (!morseActive) {
      morseStartTime = millis();
      morseActive = true;
    }
  } else if (morseActive) {
    unsigned long pressDuration = millis() - morseStartTime;
    morseActive = false;
    if (pressDuration < 300) morseInput += ".";
    else morseInput += "-";
    render();
  }

  if (valNext < TOUCH_THRESHOLD && (screen == SCREEN_GROUPCHAT || screen == SCREEN_CHAT_PERSON) && !touchActive) {
    currentText += decodeMorse(morseInput);
    morseInput = "";
    render();
  }

  if (valBack < TOUCH_THRESHOLD && (screen == SCREEN_GROUPCHAT || screen == SCREEN_CHAT_PERSON) && !touchActive) {
    if (currentText.length() > 0) currentText.remove(currentText.length() - 1);
    delay(200);
    render();
  }

  if (valBack < TOUCH_THRESHOLD && valNext >= TOUCH_THRESHOLD && (screen == SCREEN_MAIN ||
      screen == SCREEN_CHAT_LIST ||
      screen == SCREEN_LOCATION_LIST || screen == SCREEN_LOCATION_LIST || screen == SCREEN_LOCATION_PERSON || screen == SCREEN_COMPASS)) {
    while (touchRead(TOUCH_BACK) < TOUCH_THRESHOLD);
    if (screen == SCREEN_GROUPCHAT || screen == SCREEN_CHAT_LIST || screen == SCREEN_LOCATION_LIST  || screen == SCREEN_COMPASS ) {
      screen = SCREEN_MAIN;
    } else if (screen == SCREEN_CHAT_PERSON) {
      screen = SCREEN_CHAT_LIST;
    } else if (screen == SCREEN_LOCATION_PERSON) {
      screen = SCREEN_LOCATION_LIST;
    }
    render();
  }

  if (valNext < TOUCH_THRESHOLD &&  (
        screen == SCREEN_MAIN ||
        screen == SCREEN_CHAT_LIST ||
        screen == SCREEN_LOCATION_LIST)) {
    if (!touchActive) {
      touchStartTime = millis();
      touchActive = true;
    }
  } else {
    if (touchActive) {
      unsigned long duration = millis() - touchStartTime;
      if (duration < SHORT_THRESHOLD) {
        if (screen == SCREEN_MAIN) selectedIndex = (selectedIndex + 1) % mainMenuSize;
        else if (screen == SCREEN_CHAT_LIST) chatUserIndex = (chatUserIndex + 1) % userCount;
        else if (screen == SCREEN_LOCATION_LIST) locationUserIndex = (locationUserIndex + 1) % userCount;
      } else {
        if (screen == SCREEN_MAIN) {
          if (selectedIndex == 0) screen = SCREEN_GROUPCHAT;
          else if (selectedIndex == 1) screen = SCREEN_CHAT_LIST;
          else if (selectedIndex == 2) screen = SCREEN_LOCATION_LIST;
          else if (selectedIndex == 3) sendSOS();
          else if (selectedIndex == 4) screen = SCREEN_COMPASS;
        }


        else if (screen == SCREEN_CHAT_LIST) {
          screen = SCREEN_CHAT_PERSON;
        } else if (screen == SCREEN_LOCATION_LIST) {
          screen = SCREEN_LOCATION_PERSON;
        }
      }
      touchActive = false;
      render();
    }
  }
}

void setup() {
  Serial.begin(115000);
  Wire.setClock(100000);
  u8g2.begin();
  u8g2.setPowerSave(0);
  u8g2.setContrast(255);  // Max brightness (try 180–255)


  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.clearBuffer();
  touchAttachInterrupt(TOUCH_NEXT, NULL, TOUCH_THRESHOLD);
  touchAttachInterrupt(TOUCH_BACK, NULL, TOUCH_THRESHOLD);
  touchAttachInterrupt(TOUCH_MORSE, NULL, TOUCH_THRESHOLD);
}
#define NORMAL_IDLE_TIME 15000
#define CHAT_IDLE_TIME 25000
#define NORMAL_DIM_TIME 5000
#define CHAT_DIM_TIME 15000

void dimOled() {

  Wire.beginTransmission(0x3C);
  Wire.write(0x00);
  Wire.write(0x81);
  Wire.write(10); // even dimmer
  Wire.endTransmission();
  oledDimmed = true;
}
void turnOffOled() {
  u8g2.setPowerSave(1);  // completely turn off
  oledOn = false;
  oledDimmed = false;
}
void turnOnOled() {
  u8g2.setPowerSave(0);  // turn back on
  Wire.beginTransmission(0x3C);
  Wire.write(0x00);
  Wire.write(0x81);
  Wire.write(255); // full brightness
  Wire.endTransmission();
  oledOn = true;
  oledDimmed = false;
  render();  // force refresh
}
void loop() {
  handleInput();
  updateScreen();

  unsigned long now = millis();
  unsigned long diff = now - lastInteractionTime;

  bool inChat = (screen == SCREEN_GROUPCHAT || screen == SCREEN_CHAT_PERSON || screen == SCREEN_LOCATION_PERSON);
  unsigned long idleTime = inChat ? CHAT_IDLE_TIME : NORMAL_IDLE_TIME;
  unsigned long dimTime = inChat ? CHAT_DIM_TIME : NORMAL_DIM_TIME;

  // Turn off display after full idle timeout
  if (oledOn && diff > idleTime) {
    turnOffOled();
  }
  // Dim after dimTime but before full timeout
  else if (!oledDimmed && diff > dimTime && diff <= idleTime) {
    dimOled();
  }
  // Wake back up
  else if ((!oledOn || oledDimmed) && diff <= dimTime) {
    turnOnOled();
  }
}
