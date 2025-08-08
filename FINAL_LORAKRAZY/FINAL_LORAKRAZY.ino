#include "this_display.h"
#include <Wire.h>
#include <math.h>
#define CODE_DEPLOYMENT 0 // 0 for testing 1 for deployment 
// hey don't remove CODE_DEPLOYMENT

// ---------- Morse Code ----------
#define TOUCH_MORSE 15
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


#if CODE_DEPLOYMENT
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
        if (screen == SCREEN_MAIN) {selectedIndex = (selectedIndex + 1) % mainMenuSize; }
        else if (screen == SCREEN_CHAT_LIST) {chatUserIndex = (userCount == 0 ? 0 : (chatUserIndex + 1) % userCount);}
        else if (screen == SCREEN_LOCATION_LIST){ locationUserIndex = ( userCount == 0 ? 0 : (locationUserIndex + 1) % userCount);}
        
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
          
        } else if (screen == SCREEN_GROUPCHAT || screen == SCREEN_CHAT_PERSON) {
          if (currentText.length() > 0) {
            addGroupMessage("You", currentText);
            sendMessage_lora(currentText, screen == SCREEN_GROUPCHAT, allUserChats[chatUserIndex].userId); // Send the current text
            currentText = ""; // Clear the text field
          }
          
        }
      }
      touchActive = false;
      render();
    }
  }
}
#else
void handleInput() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n'); // Read a full line from serial
    command.trim(); // Remove any whitespace

    if (command == "n") { // Next
      markInteraction();
      if (screen == SCREEN_MAIN) selectedIndex = (selectedIndex + 1) % mainMenuSize;
      else if (screen == SCREEN_CHAT_LIST) chatUserIndex = (userCount == 0 ? 0 : (chatUserIndex + 1) % userCount);
      else if (screen == SCREEN_LOCATION_LIST) locationUserIndex = ( userCount == 0 ? 0 : (locationUserIndex + 1) % userCount);
      render();
    } else if (command == "b") { // Back
      markInteraction();
      if (screen == SCREEN_MAIN) {
        // No action for back on main screen
      } else if (screen == SCREEN_GROUPCHAT || screen == SCREEN_CHAT_LIST || screen == SCREEN_LOCATION_LIST || screen == SCREEN_COMPASS) {
        screen = SCREEN_MAIN;
      } else if (screen == SCREEN_CHAT_PERSON) {
        screen = SCREEN_CHAT_LIST;
      } else if (screen == SCREEN_LOCATION_PERSON) {
        screen = SCREEN_LOCATION_LIST;
      }
      render();
    } else if (command == "-") { // Select
      markInteraction();
      if (screen == SCREEN_MAIN) {
        if (selectedIndex == 0) screen = SCREEN_GROUPCHAT;
        else if (selectedIndex == 1) screen = SCREEN_CHAT_LIST;
        else if (selectedIndex == 2) screen = SCREEN_LOCATION_LIST;
        else if (selectedIndex == 3) sendSOS();
        else if (selectedIndex == 4) screen = SCREEN_COMPASS;
      } else if (screen == SCREEN_CHAT_LIST) {
        screen = SCREEN_CHAT_PERSON;
      } else if (screen == SCREEN_LOCATION_LIST) {
        screen = SCREEN_LOCATION_PERSON;
      } else if (screen == SCREEN_GROUPCHAT || screen == SCREEN_CHAT_PERSON) {
        if (currentText.length() > 0) {
          if (screen == SCREEN_GROUPCHAT)
            addGroupMessage("You", currentText.c_str());
          else {
            addMessage(selectedIndex, myDeviceId, currentText);
          }
          sendMessage_lora(currentText, screen == SCREEN_GROUPCHAT, allUserChats[chatUserIndex].userId); // Send the current text
          currentText = ""; // Clear the text field
        }
      }
      render();
    } else if (command.startsWith("m:")) { // Text input
      markInteraction();
      currentText = command.substring(2); // Get text after "m:"
      morseInput = ""; // Clear morse input
      if (screen == SCREEN_GROUPCHAT || screen == SCREEN_CHAT_PERSON) {
        // You would typically send the message here. For this example, we'll just display it.
        // For example: sendMessage(currentText);
      }
      render();
    }
  }
}
#endif

void init_touch() {

  touchAttachInterrupt(TOUCH_NEXT, NULL, TOUCH_THRESHOLD);
  touchAttachInterrupt(TOUCH_BACK, NULL, TOUCH_THRESHOLD);
  touchAttachInterrupt(TOUCH_MORSE, NULL, TOUCH_THRESHOLD);
}



void handleDisplayInLoop() {

  unsigned long now = millis();
  unsigned long diff = now - lastInteractionTime;

  bool inChat = (screen == SCREEN_GROUPCHAT || screen == SCREEN_CHAT_PERSON || screen == SCREEN_LOCATION_PERSON);
  unsigned long idleTime = inChat ? CHAT_IDLE_TIME : NORMAL_IDLE_TIME;
  unsigned long dimTime = inChat ? CHAT_DIM_TIME : NORMAL_DIM_TIME;

  // Turn off display after full idle timeout
  if (oledOn && diff > idleTime ) {
    if (screen == SCREEN_COMPASS || screen == SCREEN_LOCATION_PERSON) {
    } else {
      turnOffOled();
    }
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

void setup() {
  Serial.begin(115200);
  compass.init();
  Serial.println("inited compass");
  compass.read();
  Serial.println(compass.getAzimuth());
  init_display();
  draw_inited_screen();
  Serial.println("inited all, tryna init lora");
  init_lora();
  init_touch();
  init_users();
  Serial.println("proceeding to loop");
}
void loop() {
  updateScreen();
  handleInput();
  handleDisplayInLoop();
  if (receive_msg_lora() == 0)render();
}
