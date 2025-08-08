#include "this_lora.h"
#include <U8g2lib.h>

#include <QMC5883LCompass.h>

QMC5883LCompass compass;

U8G2_SH1106_128X64_NONAME_2_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);



#define DEG60 1.0472
#define DEG90 1.5708
#define DEG2 0.0349066
#define NEGLIGIBLE_DEG 10

#define LEFT_PADDING 4
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define LINE_HEIGHT 9
#define GROUPCHAT_LINES 10
#define GROUPCHAT_TOP_PADDING 5
#define MORSE_TOP 2

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


// ---------- Constants & Data ----------
const char* mainMenuItems[] = {
  "/groupchat",
  "/chat",
  "/locations",
  "/SOS",
  "/compass"
};

const int mainMenuSize = sizeof(mainMenuItems) / sizeof(mainMenuItems[0]);  // Keep this here

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



float myLat = 17.385;
float myLon = 78.486;

// ---------- State Management ----------


bool oledDimmed = false;
bool oledOn = true;
unsigned long lastInteractionTime = 0;

int selectedIndex = 0;
int chatUserIndex = 0;
int locationUserIndex = 0;




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


void init_display() {
  u8g2.begin();

  u8g2.setPowerSave(0);
  u8g2.setContrast(255);  // Max brightness (try 180–255)
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.clearBuffer();
  Wire.beginTransmission(0x3c);
  if (Wire.endTransmission() == 0) {
    Serial.println("Display Found");
  } else {
    Serial.println("Display(0x3c) Not Found");
  }
}
int newhash = 482, oldhash = 2321;
void render() {
  do {
    newhash = random(1, 100000);
  } while (newhash == oldhash);
}





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
  compass.read();
  int heading = (360 + compass.getAzimuth()) % 360;
  int realHeading = (heading + 90) % 360;
  int angledeg = (360 - heading) % 360;
  float angle = radians(( 180 + angledeg ) % 360); // Invert to make North fixed

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
    u8g2.print(":");
    u8g2.print(angledeg);
  } while (u8g2.nextPage());

}

void draw_inited_screen() {
  u8g2.firstPage();
  do {
    u8g2.setCursor(LEFT_PADDING + 2, 0);
    u8g2.print("Inited");
  } while (u8g2.nextPage());
}
// ---------- UI Drawing ----------
void drawMenu(const char* const* items, int size, int selected) {
  
  u8g2.firstPage();
  do {
    if(items==NULL){
      
    const int leftPadding = 4;
    const int menuWidth = SCREEN_WIDTH - 1 - leftPadding;

    for (int i = 0; i < size; ++i) {
      if (i == selected) u8g2.drawBox(leftPadding, i * 12, menuWidth - leftPadding, 12);
      u8g2.setDrawColor(i == selected ? 0 : 1);
      u8g2.setCursor(leftPadding + 2, 12 + i * 12);
      u8g2.print(allUserChats[i].username);
      u8g2.setDrawColor(1);
    }
      }
    else{
    const int leftPadding = 4;
    const int menuWidth = SCREEN_WIDTH - 1 - leftPadding;

    for (int i = 0; i < size; ++i) {
      if (i == selected) u8g2.drawBox(leftPadding, i * 12, menuWidth - leftPadding, 12);
      u8g2.setDrawColor(i == selected ? 0 : 1);
      u8g2.setCursor(leftPadding + 2, 12 + i * 12);
      u8g2.print(items[i]);
      u8g2.setDrawColor(1);
    }}

  } while (u8g2.nextPage());
}



void drawGroupChat() {
  u8g2.firstPage();
  do {
    u8g2.setCursor(0, MORSE_TOP);
    u8g2.print(morseInput.c_str());

    int startOffset = 0;
    if (groupMessageCount > GROUPCHAT_LINES) {
      startOffset = groupMessageCount - GROUPCHAT_LINES;
    }

    for (int i = 0; i < GROUPCHAT_LINES && (i + startOffset) < groupMessageCount; i++) {
      int messageIndex = (groupMessageIndex + startOffset + i) % MAX_GROUP_CHAT_MESSAGES;
      u8g2.setCursor(LEFT_PADDING, GROUPCHAT_TOP_PADDING + (1 + i) * LINE_HEIGHT);
      Serial.print("@re:");
      Serial.print(groupMessages[messageIndex].sender);
      Serial.print(": ");
      Serial.print(groupMessages[messageIndex].content);
      u8g2.print(i);
      u8g2.print(groupMessages[messageIndex].sender.c_str());
      u8g2.print(": ");
      u8g2.print(groupMessages[messageIndex].content.c_str());
    }

    u8g2.drawBox(LEFT_PADDING, SCREEN_HEIGHT - LINE_HEIGHT, SCREEN_WIDTH - 1 - LEFT_PADDING, LINE_HEIGHT);
    u8g2.setDrawColor(0);
    u8g2.setCursor(LEFT_PADDING, SCREEN_HEIGHT);
    u8g2.print('>');
    u8g2.print(currentText.c_str());
    u8g2.setDrawColor(1);
  } while (u8g2.nextPage());
}

void drawChatWithUser() {
  u8g2.firstPage();
  do {
    u8g2.setCursor(0, MORSE_TOP);
    u8g2.print(morseInput.c_str());

    UserChat& currentChat = allUserChats[chatUserIndex];
    int startOffset = 0;
    const int maxDisplayMessages = 4;
    if (currentChat.messageCount > maxDisplayMessages) {
      startOffset = currentChat.messageCount - maxDisplayMessages;
    }

    for (int i = 0; i < maxDisplayMessages && (i + startOffset) < currentChat.messageCount; i++) {
      int messageIndex = (currentChat.nextIndex + startOffset + i) % MAX_MESSAGES;
      int senderId = currentChat.messages[messageIndex].senderId;
      String content = currentChat.messages[messageIndex].content;

      if (senderId == myDeviceId) {
        u8g2.setDrawColor(0);
        u8g2.drawBox(LEFT_PADDING, (i + 1) * 12 + 1, SCREEN_WIDTH - 1 - LEFT_PADDING, 12);
        u8g2.setDrawColor(1);
        u8g2.setCursor(LEFT_PADDING + 2, (i + 2) * 12);
        u8g2.print("You: ");
        u8g2.print(content.c_str());
        u8g2.setDrawColor(1);
      } else {
        u8g2.setCursor(LEFT_PADDING, (i + 2) * 12);
        u8g2.print(currentChat.username.c_str());
        u8g2.print(": ");
        u8g2.print(content.c_str());
      }
    }

    u8g2.drawBox(LEFT_PADDING, SCREEN_HEIGHT - LINE_HEIGHT, SCREEN_WIDTH - 1 - LEFT_PADDING, LINE_HEIGHT);
    u8g2.setDrawColor(0);
    u8g2.setCursor(LEFT_PADDING, SCREEN_HEIGHT);
    u8g2.print('>');
    u8g2.print(currentText.c_str());
    u8g2.setDrawColor(1);

  } while (u8g2.nextPage());
}










float Degrees(float x) {
  return x * 180.0 / PI;
}
void drawLocationPerson() {
  compass.read();
  int heading = compass.getAzimuth();

  int realHeading = ((int)heading + 90) % 360;

  Location person = locationList[locationUserIndex];

  // Randomize target's location slightly each time
  locationList[locationUserIndex].lat += random(-10, 11) * 0.1;
  locationList[locationUserIndex].lon += random(-10, 11) * 0.1;

  float dist = calculateDistance(myLat, myLon, person.lat, person.lon);
  float angle = Degrees(calculateBearing(myLat, myLon, person.lat, person.lon));
  int projectedAngle = (int)(realHeading + angle) % 360;

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
    drawArrow(SCREEN_WIDTH - 30, SCREEN_HEIGHT / 2, radians(projectedAngle), LOC_ARROW_FRONT_LEN, LOC_ARROW_BACK_LEN, LOC_ARROW_HEAD_DIFF, LOC_HEADANGLE, LOC_ARROW_HEADLEN, LOC_PADDING_CIRCLE);
    u8g2.setCursor(0, 60);
    u8g2.print("Updated: ");
    u8g2.print(person.lastUpdated);

  } while (u8g2.nextPage());
}


void sendSOS() {
  Serial.println("[SOS] Emergency signal sent!");
  sendSOS_lora(myLat,myLon);

  u8g2.firstPage();
  do {
    u8g2.setCursor(LEFT_PADDING, SCREEN_HEIGHT / 2 - LINE_HEIGHT / 2);
    u8g2.print("SENDING SOS...");
  } while (u8g2.nextPage());
  delay(2000);
  screen = SCREEN_MAIN;
  render();

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



void updateScreen() {
  if ( newhash == oldhash) {
    return;
  }
  oldhash = newhash;
  Serial.println("re");
  
  switch (screen) {
    case SCREEN_MAIN:
      drawMenu(mainMenuItems, mainMenuSize, selectedIndex); break;
    case SCREEN_GROUPCHAT:
      drawGroupChat(); break;
    case SCREEN_CHAT_LIST:
      drawMenu(NULL, userCount, chatUserIndex); break;
    case SCREEN_CHAT_PERSON:
      drawChatWithUser(); break;
    case SCREEN_LOCATION_LIST:
      drawMenu(NULL, userCount, locationUserIndex); break;
    case SCREEN_LOCATION_PERSON:
      drawLocationPerson(); break;
    case SCREEN_COMPASS:
      drawCompass(); break;

  }
}
