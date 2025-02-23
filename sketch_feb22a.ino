#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <SPI.h>

// ✅ WiFi Credentials
const char* ssid = "Owxn";
const char* password = "Owxn2409";

// ✅ Telegram Bot Tokens (1 บอทต่อ 1 ชั้น)
const char* botTokens[] = {
  "7713083064:AAFNzaIMmlDjwM6nyl6z1eAwkKHY1Zcnu9Q",
  "7702438986:AAEeokB03nKz0Y9s7Vs4VWi-U7pzHHVO8v8",
  "8175471471:AAG3IpS62xQb_2pR-ZwfZnH_aVMy5ekjukw",
  "7731694722:AAGIyRqH4XgT-Bh48aQWDWks0IN9x7mzveo",
  ""
};

// ✅ Chat IDs ของแต่ละบอท
const char* chatIds[] = {
  "-4734652541",
  "-4767274518",
  "-4708772755",
  "-4729985406",
  ""
};

// ✅ จำนวนชั้นที่ใช้งาน
const int numDrawers = sizeof(botTokens) / sizeof(botTokens[0]);

// ✅ กำหนด GPIO สำหรับ IR Sensors
const int irPins[][2] = {
  {D0, D1}, {D2, D3}, {D4, D5}, {D6, D7}, {D8, D9}
};

// ✅ ตัวแปรสถานะ
bool previousState[numDrawers][2];
unsigned long entryStartTime[numDrawers] = {0};
unsigned long removeStartTime[numDrawers] = {0};
int documentCount[numDrawers] = {0};

// ✅ WiFiClientSecure และ UniversalTelegramBot
WiFiClientSecure client;
UniversalTelegramBot* bots[numDrawers];

void setup() {
  Serial.begin(115200);
  SPI.begin();

  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  client.setInsecure();

  // ตั้งค่า IR Sensors
  for (int i = 0; i < numDrawers; i++) {
    pinMode(irPins[i][0], INPUT);
    pinMode(irPins[i][1], INPUT);
    previousState[i][0] = digitalRead(irPins[i][0]);
    previousState[i][1] = digitalRead(irPins[i][1]);
  }

  // สร้างบอทแต่ละตัว
  for (int i = 0; i < numDrawers; i++) {
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
    if (sent) {
      Serial.println("✅ ส่งข้อความสำเร็จไปยัง Bot ชั้น " + String(drawer + 1));
    } else {
      Serial.println("❌ ส่งข้อความล้มเหลวที่ Bot ชั้น " + String(drawer + 1));
    }
  } else {
    Serial.println("⚠️ ไม่มีบอทสำหรับชั้น " + String(drawer + 1));
  }
}

void loop() {
  for (int i = 0; i < numDrawers; i++) {
    bool currentState1 = digitalRead(irPins[i][0]);
    bool currentState2 = digitalRead(irPins[i][1]);
    unsigned long currentTime = millis();

    if (currentState1 == LOW && entryStartTime[i] == 0) {
      entryStartTime[i] = currentTime;
    }
    if (currentState1 == HIGH && entryStartTime[i] != 0 && currentTime - entryStartTime[i] >= 1000 && currentState2 == LOW) {
      documentCount[i]++;
      sendNotification(i, "🔔 มีเอกสารส่งถึงคุณ! เช็กที่ลิ้นชักเลย");
      entryStartTime[i] = 0;
    }

    if (currentState2 == HIGH && removeStartTime[i] == 0) {
      removeStartTime[i] = currentTime;
    }
    if (currentState2 == HIGH && removeStartTime[i] != 0 && currentTime - removeStartTime[i] > 1000) {
      sendNotification(i, "📜 เอกสารถูกนำออก! หากไม่ใช่คุณ โปรดตรวจสอบ");
      removeStartTime[i] = 0;
    }
    if (currentState2 == LOW) {
      removeStartTime[i] = 0;
    }

    previousState[i][0] = currentState1;
    previousState[i][1] = currentState2;
  }
  delay(100);
}
