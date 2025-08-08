
#include <SPI.h>
#include <LoRa.h>

int myDeviceId = random(1, 0xffff);
String myUserName = "kkrazy2" ;
// LoRa pins (adjust if using different module pins)
#define LORA_SS 5
#define LORA_RST 14
#define LORA_DIO0 26
#define LORA_BAND 433E6 // Or 868E6 / 915E6 depending on your module

//msgs.h


int userCount = 0;

// In the global variables section

#define MAX_GROUP_CHAT_MESSAGES 20
struct ChatMessage {
  String sender;
  String content;
};

ChatMessage groupMessages[MAX_GROUP_CHAT_MESSAGES];
int groupMessageCount = 0;
int groupMessageIndex = 0;

void addGroupMessage(String sender, String content) {
  // Determine the current index for the new message
  int index = groupMessageIndex;

  // Free the old memory if the array is full (circular buffer)
  if (groupMessageCount == MAX_GROUP_CHAT_MESSAGES) {
    // free the old memory when needed here
  }
  groupMessages[index].sender = sender;
  groupMessages[index].content = content;
  Serial.println("@fnc:");
  Serial.println( groupMessages[index].sender);

  // Update the message count and index
  if (groupMessageCount < MAX_GROUP_CHAT_MESSAGES) {
    groupMessageCount++;
  }
  groupMessageIndex = (index + 1) % MAX_GROUP_CHAT_MESSAGES;
}

#define MAX_MESSAGES 20 // Max messages per user
#define MAX_USERS 5    // Max number of users in your system



struct Message {
  int senderId;
  String content;
};

// Define a structure for a user's entire chat history
struct UserChat {
  String username;
  int userId = -1;
  Message messages[MAX_MESSAGES];
  int messageCount = 0;
  int nextIndex = 0;
};

// Global array to hold all user chats
UserChat allUserChats[MAX_USERS];

// Helper function to find a user's index by username
int findUserIndexById(int userId) {
  for (int i = 0; i < MAX_USERS; i++) {
    if (allUserChats[i].userId == userId) {
      return i;
    }
  }
  return -1; // User not found
}
void addMessage(int userIndex, int senderId, String content) {
  if (userIndex < 0 || userIndex >= MAX_USERS) {
    return;
  }

  UserChat& userChat = allUserChats[userIndex];
  int index = userChat.nextIndex;

  userChat.messages[index].senderId = senderId;
  userChat.messages[index].content = content;

  if (userChat.messageCount < MAX_MESSAGES) {
    userChat.messageCount++;
  }
  userChat.nextIndex = (index + 1) % MAX_MESSAGES;
}

void sendUsernameQuery(int targetId) {
  String query = "QUERY:";
  query += myDeviceId;
  LoRa.beginPacket();
  LoRa.print(query);
  LoRa.print(":");
  LoRa.print(targetId);
  LoRa.endPacket(true);
}
// Add a new function to respond to a username query
void sendUsernameResponse(int targetId) {
  String response = "REPLY:";
  response += targetId;
  response += ":";
  response += myDeviceId;
  response += ":";
  response += myUserName; // Your device's username
  LoRa.beginPacket();
  LoRa.print(response);
  LoRa.endPacket(true);
}
// New helper to add a user when a message is received from a new ID
int addNewUser(int userId, String username = "") {
  if (userCount < MAX_USERS) {
    allUserChats[userCount].userId = userId;
    allUserChats[userCount].username = username;
    userCount++;
    return userCount - 1;
  }
  return -1; // No space for new user
}
void init_users() {
  for (int i = 0; i < userCount; i++) {
    // Assign the index as the user ID for simplicity
    allUserChats[i].userId = i;
  }
}
//msgs.h end





void disable_features_lora() {
  LoRa.disableInvertIQ();
  LoRa.setSpreadingFactor(7); // Lower = faster, less memory use
  LoRa.setSignalBandwidth(125E3); // Narrower BW reduces processing load
}
void init_lora() {
  SPI.begin(); // Init SPI bus
  // Initialize LoRa
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(LORA_BAND)) {
    Serial.println("LoRa init failed. Check connections.");
  } else {
    LoRa.setSyncWord(0xF3); // Prevents cross-talk with other LoRa networks
    LoRa.disableCrc();      // Saves a bit of processing if you don't need CRC
    Serial.println("LoRa init OK");
  }
  disable_features_lora();
}
void sendSOS_lora(float myLat, float myLon) {
  LoRa.beginPacket();
  LoRa.print("[SOS]: ID_YOURDEVICE Lat:");
  LoRa.print(myLat, 4); // Send latitude with 4 decimal places
  LoRa.print(" Lon:");
  LoRa.print(myLon, 4); // Send longitude with 4 decimal places
  LoRa.endPacket(true); // true = async send (non-blocking)
  Serial.println("[SOS] Emergency signal sent!");
}
// ---------- State Management ----------
String receivedMessage = ""; // To store incoming LoRa messages
unsigned long messageDisplayTime = 0; // To control how long a message is displayed
const unsigned long MESSAGE_TIMEOUT = 5000; // Display message for 5 seconds

void printHexLora(int i) {
  while (i > 0) {
    LoRa.print((i % 16 < 10 ? 0 : 87) + i % 16);
    i /= 16;
  }
}
void sendMessage_lora(String message, bool isGroup, int receiverDeviceId ) {
  LoRa.beginPacket();
  LoRa.print("MSG:");
  if (isGroup) {
    LoRa.print("G:");

    LoRa.print(myDeviceId); LoRa.print(":");

    LoRa.print(myUserName); LoRa.print(">");

  } else {
    LoRa.print(receiverDeviceId);
    LoRa.print(":");
    LoRa.print(myDeviceId); LoRa.print(">");

  }
  // No need to print "sender" here, as the device ID is implicitly the sender.
  LoRa.print(message);
  LoRa.endPacket(true);
  Serial.print("Message sent: ");
  Serial.println(message);
}
// Modify the receive_msg_lora function
int receive_msg_lora() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String incoming = "";
    while (LoRa.available()) {
      incoming += (char)LoRa.read();
    }

    if (incoming.startsWith("MSG:G:")) {
      // Group chat message parsing
      String messageContent = incoming.substring(6);
      int colonIndex = messageContent.indexOf(':');
      int senderId = messageContent.substring(0, colonIndex).toInt();
      int gtIndex = messageContent.indexOf('>');
      String senderUName = messageContent.substring(colonIndex + 1, gtIndex);
      String messageBody = messageContent.substring(gtIndex + 1);
      addGroupMessage(senderUName, messageBody); // Displaying ID for now
      Serial.println("@after fnc:");
      Serial.println( groupMessages[0].sender);
    } else if (incoming.startsWith("MSG:")) {
      // Private chat message parsing
      String messageContent = incoming.substring(4);
      int colon1 = messageContent.indexOf(':');
      int receiverId = messageContent.substring(0, colon1).toInt();

      if (receiverId != myDeviceId) return -1; // Not for us

      int gt = messageContent.indexOf('>');
      int senderId = messageContent.substring(colon1 + 1, gt).toInt();
      String messageBody = messageContent.substring(gt + 1);

      int userIndex = findUserIndexById(senderId);
      if (userIndex == -1) {
        userIndex = addNewUser(senderId);
        sendUsernameQuery(senderId);
      }
      addMessage(userIndex, senderId, messageBody);
    } else if (incoming.startsWith("QUERY:")) {
      // Handle username query
      String content = incoming.substring(6);
      int colonIndex = content.indexOf(':');
      int senderId = content.substring(0, colonIndex).toInt();
      int targetId = content.substring(colonIndex + 1).toInt();

      if (targetId == myDeviceId) {
        sendUsernameResponse(senderId);
      }
    } else if (incoming.startsWith("REPLY:")) {
      // Handle username reply
      String content = incoming.substring(6);
      int colon1 = content.indexOf(':');
      int targetId = content.substring(0, colon1).toInt();

      if (targetId == myDeviceId) {
        int colon2 = content.indexOf(':', colon1 + 1);
        int senderId = content.substring(colon1 + 1, colon2).toInt();
        String username = content.substring(colon2 + 1);

        int userIndex = findUserIndexById(senderId);
        if (userIndex != -1) {
          allUserChats[userIndex].username = username;
        }
      }
    }

    Serial.print("LoRa Received: ");
    Serial.println(incoming);
    return 0;
  }
  return -1;
}
