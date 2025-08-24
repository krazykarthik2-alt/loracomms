#include <SPI.h>
#include <LoRa.h>
#include "this_gps.h"
#include<map>
#include <vector>
int myDeviceId = random(1, 0xffff);
String myUserName = "kkrazy2";
// LoRa pins (adjust if using different module pins)
#define LORA_SS 5
#define LORA_RST 14
#define LORA_DIO0 26
#define LORA_BAND 433E6 // Or 868E6 / 915E6 depending on your module

// msgs.h


// In the global variables section

#define MAX_GROUP_CHAT_MESSAGES 20
struct ChatMessage
{
  String sender;
  String content;
};

ChatMessage groupMessages[MAX_GROUP_CHAT_MESSAGES];
int groupMessageCount = 0;
int groupMessageIndex = 0;

void addGroupMessage(const String &sender, const String &content)
{
  // Reference to the slot where the new message will be stored

  struct ChatMessage &msg = groupMessages[(groupMessageIndex + groupMessageCount) % MAX_GROUP_CHAT_MESSAGES];

  // Overwrite with new values
  msg.sender  = sender;   // This will overwrite old String data automatically
  msg.content = content;  // String destructor will free old content internally

  Serial.println("@fnc:");
  Serial.println(msg.sender);

  if (groupMessageCount < MAX_GROUP_CHAT_MESSAGES) {
    groupMessageCount++;
  } else
  {
    groupMessageIndex = (groupMessageIndex + 1) % MAX_GROUP_CHAT_MESSAGES;
  }
}


#define MAX_MESSAGES 20 // Max messages per user
#define MAX_USERS 5     // Max number of users in your system

struct Message
{
  bool sentByMe;
  String content;
};

// Define a structure for a user's entire chat history
struct User {
  String username;
  float lat;
  float lon;
  unsigned long lastUpdate;
  Message messages[MAX_MESSAGES];
  int messageCount ;
  int startIndex ;


  User()
    : username(""), lat(0.0), lon(0.0), lastUpdate(0),
      messageCount(0),  startIndex(0)
  {
    for (int i = 0; i < MAX_MESSAGES; i++) {
      messages[i].sentByMe = false;
      messages[i].content = "";
    }
  }
};

// Global array to hold all user chats
std::map <int, User> allUserChats;//userid, user

void addMessage(int userId, int senderId, String content)
{
  struct User &userChat = allUserChats[userId];

  // Ensure indices are within bounds
  userChat.startIndex %= MAX_MESSAGES;
  int nextIndex = (userChat.startIndex + userChat.messageCount) % MAX_MESSAGES;

  Message &msg = userChat.messages[nextIndex];
  msg.sentByMe = (myDeviceId == senderId);
  msg.content = content;


  if (userChat.messageCount < MAX_MESSAGES) {
    userChat.messageCount++;
  } else {
    // buffer is full so move startIndex forward
    userChat.startIndex = (userChat.startIndex + 1) % MAX_MESSAGES;
  }

  for (int i = 0; i < userChat.messageCount; i++) {
    int idx = (userChat.startIndex + i) % MAX_MESSAGES;
    Serial.println(userChat.messages[idx].content);
  }
}


void sendUsernameQuery(int targetId)
{
  LoRa.beginPacket();
  LoRa.print("QUERY:");
  LoRa.print(myDeviceId);
  LoRa.print(":");
  LoRa.print(myUserName);
  LoRa.print(":");
  LoRa.print(targetId);
  LoRa.endPacket(true);
}
// Add a new function to respond to a username query
void sendUsernameResponse(int targetId)
{
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
bool addNewUser(int userId, String username = "")
{
  Serial.printf("adding %d : %s\n", userId, username.c_str());

  if (allUserChats.size() < MAX_USERS)
  {
    struct User &u = allUserChats[userId];
    u.username = username;
    return true;
  }
  return false;
}


void disable_features_lora()
{
  LoRa.disableInvertIQ();
  LoRa.setSpreadingFactor(12);     // Lower = faster, less memory use
  LoRa.setSignalBandwidth(125E3); // Narrower BW reduces processing load
}
void init_lora()
{
  SPI.begin(); // Init SPI bus
  // Initialize LoRa
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(LORA_BAND))
  {
    Serial.println("LoRa init failed. Check connections.");
  }
  else
  {
    LoRa.setTxPower(20);
    LoRa.setCodingRate4(5);            // CR 4/5 (balanced reliability)
    LoRa.enableCrc();
    LoRa.setSyncWord(0xF3); // Prevents cross-talk with other LoRa networks
    Serial.println("LoRa init OK");
    LoRa.beginPacket();
    LoRa.print( "B:");
    LoRa.print( myDeviceId);
    LoRa.print( ":-1:-1");
    LoRa.endPacket(true);
  }
  disable_features_lora();
}
void sendSOS_lora(float myLat, float myLon)
{
  LoRa.beginPacket();
  LoRa.print("SOS:");
  LoRa.print(myDeviceId);
  LoRa.print(":");
  LoRa.print(myLat, 6);
  LoRa.print(":");
  LoRa.print(myLon, 6);
  LoRa.endPacket(true); // true = async send (non-blocking)
  Serial.println("[SOS] Emergency signal sent!");
}
// ---------- State Management ----------
String receivedMessage = "";                // To store incoming LoRa messages
unsigned long messageDisplayTime = 0;       // To control how long a message is displayed
const unsigned long MESSAGE_TIMEOUT = 5000; // Display message for 5 seconds

void printHexLora(int i)
{
  while (i > 0)
  {
    LoRa.print((i % 16 < 10 ? 0 : 87) + i % 16);
    i /= 16;
  }
}
void sendMessage_lora(String message, bool isGroup, int receiverDeviceId)
{
  LoRa.beginPacket();
  LoRa.print("MSG:");
  if (isGroup)
  {
    LoRa.print("G:");

    LoRa.print(myDeviceId);
    LoRa.print(":");

    LoRa.print(myUserName);
    LoRa.print(">");
  }
  else
  {
    LoRa.print(receiverDeviceId);
    LoRa.print(":");
    LoRa.print(myDeviceId);
    LoRa.print(">");
  }
  // No need to print "sender" here, as the device ID is implicitly the sender.
  LoRa.print(message);
  LoRa.endPacket(true);
  Serial.print("Message sent: ");
  Serial.println(message);
}


void handleIncomingBeacon(String msg)
{
  // Format: B:<userid>:<lat>:<lon>
  int first = msg.indexOf(':');
  int second = msg.indexOf(':', first + 1);
  int third = msg.indexOf(':', second + 1);

  int userid = msg.substring(first + 1, second).toInt();
  float lat = msg.substring(second + 1, third).toFloat();
  float lon = msg.substring(third + 1).toFloat();

  if ((allUserChats.find(userid) == allUserChats.end())) {
    if (!addNewUser(userid))
    {
      Serial.println("Max users reached");
      return;
    }
  }
  struct User *u = &allUserChats[userid];

  if (u->username == "")
  {
    sendUsernameQuery(userid);
    u->username = "unknown:" + String(userid); // Default username until we get a reply
  }
  if (lat == -1 && lon == -1) {} else {

    u->lat = lat;
    u->lon = lon;
    u->lastUpdate = millis();
  }
  Serial.print("beacon received from ");
  Serial.println(u->username);
}


/**
   Processes incoming LoRa messages and handles different types of communication.
   - Parses the incoming packet to determine its type.
   - If the message starts with "B:", it is treated as a beacon and handled by `handleIncomingBeacon()`.
   - If the message starts with "MSG:G:", it is a group chat message, and the sender's username and message body are extracted and stored.
   - If the message starts with "MSG:", it is a private chat message, and the receiver and sender IDs are parsed. The message is stored if intended for this device.
   - If the message starts with "QUERY:", it handles username queries, responding if the target ID matches this device.
   - If the message starts with "REPLY:", it processes username replies, updating the sender's username in the chat history if the reply is intended for this device.

   @return 1 if a beacon message was handled, 0 for other message types, and -1 if no packet was available or the message was not intended for this device.
*/

String lastMessage = "";
int receive_msg_lora()
{
  int packetSize = LoRa.parsePacket();
  if (packetSize)
  {
    String incoming = "";
    while (LoRa.available())
    {
      incoming += (char)LoRa.read();
    }
    Serial.print("rssi:");
    Serial.println(
      LoRa.packetRssi());
    Serial.println(incoming);
    lastMessage = incoming;
    if (incoming.startsWith("B:"))
    {
      handleIncomingBeacon(incoming);
      return 1; // handled
    }
    else if (incoming.startsWith("MSG:G:"))
    {
      String messageContent = incoming.substring(6);
      int colonIndex = messageContent.indexOf(':');
      int senderId = messageContent.substring(0, colonIndex).toInt();
      int gtIndex = messageContent.indexOf('>');
      String senderUName = messageContent.substring(colonIndex + 1, gtIndex);
      String messageBody = messageContent.substring(gtIndex + 1);
      addGroupMessage(senderUName, messageBody); // Displaying ID for now
      Serial.println("@after fnc:");
      Serial.println(groupMessages[0].sender);
      return 0;
    }
    else if (incoming.startsWith("MSG:"))
    {
      // Private chat message parsing
      String messageContent = incoming.substring(4);
      int colon1 = messageContent.indexOf(':');
      int receiverId = messageContent.substring(0, colon1).toInt();

      if (receiverId != myDeviceId) {
        Serial.println("msg not for us");
        return -1; // Not for us
      }

      int gt = messageContent.indexOf('>');
      int senderId = messageContent.substring(colon1 + 1, gt).toInt();
      String messageBody = messageContent.substring(gt + 1);

      // find user in allUserChats by userid
      if (allUserChats.find(senderId) == allUserChats.end())
      {
        // If user not found, add them
        addNewUser(senderId);
        sendUsernameQuery(senderId);
      }
      addMessage(senderId, senderId, messageBody);
      return 0;
    }
    else if (incoming.startsWith("QUERY:"))
    {
      // Handle username query
      String content = incoming.substring(6);
      int colonIndex1 = content.indexOf(':');
      int colonIndex2 = content.indexOf(':', colonIndex1 + 1);
      int senderId = content.substring(0, colonIndex1).toInt();
      String senderUserName = content.substring(colonIndex1, colonIndex2);
      int targetId = content.substring(colonIndex2 + 1).toInt();

      if (targetId == myDeviceId)
      {
        sendUsernameResponse(senderId);
      }
      if (allUserChats.find(senderId) == allUserChats.end())
      {

        Serial.println("notfound uid,creating");
        addNewUser(senderId);
      } else
      {
        Serial.println("found uid,changing uname");
      }

      struct User *u = &allUserChats[senderId];
      u->username = senderUserName;
    }
    else if (incoming.startsWith("REPLY:"))
    {
      // Handle username reply
      String content = incoming.substring(6);
      int colon1 = content.indexOf(':');
      int targetId = content.substring(0, colon1).toInt();

      if (targetId == myDeviceId)
      {
        int colon2 = content.indexOf(':', colon1 + 1);
        int senderId = content.substring(colon1 + 1, colon2).toInt();
        String username = content.substring(colon2 + 1);

        Serial.printf("parsed key:%d", senderId);
        // find user in allUserChats by userid
        if (allUserChats.find(senderId) == allUserChats.end())
        {

          Serial.println("notfound uid,creating");
          addNewUser(senderId);
        } else
        {
          Serial.println("found uid,changing uname");
        }

        struct User *u = &allUserChats[senderId];
        u->username = username;

      } else {
        Serial.println("targetId doesn't match.ignore");
      }
      return 0;
    }
    else if (incoming.startsWith("SOS:"))
    {
      Serial.println("SOS received from " + incoming.substring(4));
      return 0;
    }
  } else {

    //  int idleRssi = LoRa.rssi();  // returns RSSI in dBm
    //  Serial.println(idleRssi);
  }
  return -1;
}
void sendLocationAsBeacon()
{
  Serial.print("sending beacon->");
  LoRa.beginPacket();
  LoRa.print( "B:" );
  LoRa.print(myDeviceId);
  LoRa.print(":");
  LoRa.print(currentLat);
  LoRa.print(":");
  LoRa.print(currentLon);
  LoRa.endPacket(true);
}
bool sendLocationBeacon()
{
  listenToGPS();
  if (getLocation())
  { // Your existing GPS acquisition function
    sendLocationAsBeacon();
    stopListeningToGPS();
    clearLocation();
    return true;
  }
  return false;
}
