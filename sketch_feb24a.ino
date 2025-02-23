#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <SPI.h>

// ✅ WiFi Credentials
const char* ssid = "Owxn";
const char* password = "Owxn2409";

// ✅ Telegram Bot Tokens (1 บอทต่อ 1 ชั้น)
const char* botTokens[] = {
  "BOT_TOKEN_1", // ชั้น 1
  "BOT_TOKEN_2", // ชั้น 2
  "BOT_TOKEN_3", // ชั้น 3
  "",            // ชั้น 4 (ยังไม่มี)
  ""             // ชั้น 5 (ยังไม่มี)
};

// ✅ Chat IDs ของแต่ละบอท
const char* chatIds[] = {
  "CHAT_ID_1",  // ชั้น 1
  "CHAT_ID_2",  // ชั้น 2
  "CHAT_ID_3",  // ชั้น 3
  "",           // ชั้น 4 (ยังไม่มี)
  ""            // ชั้น 5 (ยังไม่มี)
};

const int numDrawers = sizeof(botTokens) / sizeof(botTokens[0]);
const int irPins[][2] = {
  {D0, D1}, {D2, D3}, {D4, D5}, {D6, D7}, {D8, D9}
};

bool previousState[numDrawers][2];
unsigned long lastTriggerTime[numDrawers];
const unsigned long debounceTime = 3000;
static unsigned long entryStartTime[numDrawers] = {0};
static int documentCount[numDrawers] = {0};

WiFiClientSecure client;
UniversalTelegramBot* bots[numDrawers];

void setup() {
  Serial.begin(115200);
  SPI.begin();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi Connected!");
  Serial.print(" IP Address: ");
  Serial.println(WiFi.localIP());

  client.setInsecure();

  for (int i = 0; i < numDrawers; i++) {
    pinMode(irPins[i][0], INPUT);
    pinMode(irPins[i][1], INPUT);
    previousState[i][0] = digitalRead(irPins[i][0]);
    previousState[i][1] = digitalRead(irPins[i][1]);
    lastTriggerTime[i] = 0;
    
    if (strlen(botTokens[i]) > 0) {
      bots[i] = new UniversalTelegramBot(botTokens[i], client);
    } else {
      bots[i] = nullptr;
    }
  }
}

void sendNotification(int drawer, String message) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ WiFi ไม่เชื่อมต่อ");
    return;
  }

  if (bots[drawer] != nullptr && strlen(chatIds[drawer]) > 0) {
    bool sent = bots[drawer]->sendMessage(chatIds[drawer], message, "Markdown");
    Serial.println(sent ? "✅ ส่งข้อความสำเร็จ" : "❌ ส่งข้อความล้มเหลว");
  }
}

void loop() {
  for (int i = 0; i < numDrawers; i++) {
    bool currentState1 = digitalRead(irPins[i][0]);
    bool currentState2 = digitalRead(irPins[i][1]);
    unsigned long currentTime = millis();

    // Debugging Sensor Readings
    Serial.print("🔍 ชั้น "); Serial.print(i + 1);
    Serial.print(" -> Sensor1: "); Serial.print(currentState1);
    Serial.print(" | Sensor2: "); Serial.println(currentState2);

    if (currentState1 == LOW) {
      if (entryStartTime[i] == 0) {
        entryStartTime[i] = currentTime;
      }
    } else {
      if (entryStartTime[i] != 0 && currentState2 == LOW) {
        unsigned long timeElapsed = currentTime - entryStartTime[i];
        if (timeElapsed >= 1000) {
          documentCount[i]++;
          sendNotification(i, "🔔 มีเอกสารส่งถึงคุณ!");
          entryStartTime[i] = 0;
        }
      }
    }

    if (previousState[i][1] == LOW && currentState2 == HIGH && previousState[i][0] == HIGH && currentState1 == LOW) {
      if (currentTime - lastTriggerTime[i] > debounceTime) {
        sendNotification(i, "📜 เอกสารถูกนำออก!");
        lastTriggerTime[i] = currentTime;
      }
    }

    previousState[i][0] = currentState1;
    previousState[i][1] = currentState2;
  }
  delay(100);
}

