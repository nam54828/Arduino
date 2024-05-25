#include <Wire.h>
#include <DHT.h>
#include <Adafruit_MCP23X17.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>
#include <HTTPClient.h>


const char* serverName = "http://172.20.10.7:3000/device";

const char* serverSecurity = "http://172.20.10.7:3000/security";

#define MCP23017_ADDR 0x24  // Địa chỉ I2C của MCP23017

#define DHTPIN 0
#define DHTTYPE DHT22

#define MQ8_3V3_PIN_AOUT 35
#define IRFLAME_3V3_PIN_AOUT 34  // Báo cháy

// Khai báo các chân cho RFID
#define RST_PIN 2  // Chân RST của RC522
#define SS_PIN 5   // Chân SDA của RC522

// Khai báo các chân kết nối với L298N
#define IN1_PIN 0  // Chân IN0 cho động cơ A trên L298N
#define IN2_PIN 1  // Chân IN1 cho động cơ A trên L298N
#define IN3_PIN 2  // Chân IN2 cho động cơ B trên L298N
#define IN4_PIN 3  // Chân IN3 cho động cơ B trên L298N
#define PIR_LED 4
#define ENA_PIN 27
#define ENB_PIN 17

#define SERVO1_PIN 4
#define SERVO2_PIN 16
int Buzzer = 13;
#define rainSensor 26
#define SOIL_MOISTURE_PIN 33
#define Fan_RELAY2 14
#define LIGHT_PIN 32
#define LIGHT_LED 25
#define TOUCH_PIN 12
int PIR_PIN = 15;
int pirState = 0;
#define MQ8_THRESHOLD 500
#define soilMoistureThreshold 2500

// Mã cho các phím
char keys[4][4] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};

// Khởi tạo đối tượng MCP23017
Adafruit_MCP23X17 mcp;

// Khởi tạo đối tượng LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

const char* mqtt_server = "a173311e38b74a4f81fe5b6632087769.s1.eu.hivemq.cloud";
const char* mqtt_username = "nam54828";
const char* mqtt_password = "DoDucNam123";
static const char* root_ca = R"EOF(
  -----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
  -----END CERTIFICATE-----
)EOF";


char ssid[] = "Azure";
char pass[] = "abc.12345";

// Khởi tạo đối tượng MFRC522
MFRC522 mfrc522(SS_PIN, RST_PIN);

String password = "";
bool loopPaused = false;

unsigned long lastKeypadTime = 0;

WiFiClientSecure espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE);
Servo servo1, servo2;
HTTPClient http;

char mqtt_client_id[20];

void setup() {
  Serial.begin(115200);
  dht.begin();
  SPI.begin();
  servo1.attach(SERVO1_PIN);
  servo2.attach(SERVO2_PIN);
  lcd.init();
  pinMode(MQ8_3V3_PIN_AOUT, INPUT);
  pinMode(SOIL_MOISTURE_PIN, INPUT);
  pinMode(IRFLAME_3V3_PIN_AOUT, INPUT);
  pinMode(rainSensor, INPUT);
  servo1.write(180);
  servo2.write(0);
  pinMode(Buzzer, OUTPUT);
  digitalWrite(Buzzer, LOW);
  pinMode(Fan_RELAY2, OUTPUT);
  pinMode(LIGHT_LED, OUTPUT);
  pinMode(LIGHT_PIN, INPUT);
  pinMode(TOUCH_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);
  // Khởi tạo MCP23017
  mcp.begin_I2C(MCP23017_ADDR);

  // Thiết lập chân PA cho điều khiển động cơ A
  mcp.pinMode(IN1_PIN, OUTPUT);
  mcp.pinMode(IN2_PIN, OUTPUT);

  // Thiết lập chân PA cho điều khiển động cơ B
  mcp.pinMode(IN3_PIN, OUTPUT);
  mcp.pinMode(IN4_PIN, OUTPUT);

  mcp.pinMode(PIR_LED, OUTPUT);

  pinMode(ENA_PIN, OUTPUT);
  pinMode(ENB_PIN, OUTPUT);


  sprintf(mqtt_client_id, "arduino-client-%04X", random(0xFFFF));
  // Thiết lập chân PB cho hàng (ROW) keypad
  for (int i = 8; i < 12; i++) {
    mcp.pinMode(i, OUTPUT);
    mcp.digitalWrite(i, HIGH);  // Kích hoạt pullup
  }

  // Thiết lập chân PB cho cột (COL) keypad
  for (int i = 12; i < 16; i++) {
    mcp.pinMode(i, INPUT_PULLUP);
  }

  // Khởi tạo LCD
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Keypad/RFID:");

  // Khởi tạo RFID
  mfrc522.PCD_Init();
  http.begin(serverName);
  connectToWiFi();

  connectToMQTT();
}


void loop() {
  if (!client.connected()) {
    connectToMQTT();
  }
  http.begin(serverName);

  int httpResponseCode = http.GET();
  if (httpResponseCode > 0) {
    String response = http.getString();
    if (response.startsWith("{\"count\":")) {
      DynamicJsonDocument doc(8192);
      deserializeJson(doc, response);
      JsonArray data = doc["data"];
      for (JsonVariant device : data) {
        JsonObject category = device["category"];
        String name = category["name"];
        bool status = category["status"];

        Serial.print("Name: ");
        Serial.println(name);
        Serial.print("Status: ");
        Serial.println(status);

        if (name == "LED") {
          if (status) {
            digitalWrite(LIGHT_LED, HIGH);
            Serial.println("LED ON");
          } else {
            digitalWrite(LIGHT_LED, LOW);
            Serial.println("LED OFF");
          }
        }

        if (name == "DOOR") {
          if (status) {
            servo1.write(90);
            servo2.write(90);
            Serial.println("DOOR ON");
          } else {
            servo1.write(180);
            servo2.write(0);
            Serial.println("DOOR OFF");
          }
        }

        if (name == "LIGHT SENSOR") {
          if (status) {
            int light = analogRead(LIGHT_PIN);
            if (light > 700) {
              digitalWrite(LIGHT_LED, HIGH);
            } else {
              digitalWrite(LIGHT_LED, LOW);
            }
          } else {
            digitalWrite(LIGHT_LED, LOW);
          }
        }

        if (name == "FLOOD") {
          if (status) {
            int soilMoistureValue = analogRead(SOIL_MOISTURE_PIN);
            if (soilMoistureValue < soilMoistureThreshold) {
              digitalWrite(Buzzer, HIGH);
            }
          } else {
            digitalWrite(Buzzer, LOW);
          }
        }

        if (name == "RAIN") {
          if (status) {
            int rainValue = digitalRead(rainSensor);
            if (!rainValue) {
              digitalWrite(Buzzer, HIGH);
            }
          } else {
            digitalWrite(Buzzer, LOW);
          }
        }

        if (name == "FAN") {
          if (status) {
            digitalWrite(Fan_RELAY2, HIGH);
          } else {
            digitalWrite(Fan_RELAY2, LOW);
          }
        }

        if (name == "GAS") {
          if (status) {
            int mq8Value = analogRead(MQ8_3V3_PIN_AOUT);
            if (mq8Value > MQ8_THRESHOLD) {
              digitalWrite(Buzzer, HIGH);
              digitalWrite(Fan_RELAY2, HIGH);
            }
          } else {
            digitalWrite(Buzzer, LOW);
            digitalWrite(Fan_RELAY2, LOW);
          }
        }

        if (name == "PIR") {
          if (status) {
            pirState = digitalRead(PIR_PIN);
            if (pirState == HIGH) {
              mcp.digitalWrite(PIR_LED, HIGH);
            } else {
              mcp.digitalWrite(PIR_LED, LOW);
            }
          } else {
            mcp.digitalWrite(PIR_LED, LOW);
          }
        }
      }
    } else {
      Serial.println("Invalid JSON response");
    }
  } else {
    Serial.println("Error getting data");
  }

  bool touchValue = digitalRead(TOUCH_PIN);
  bool flameValue = digitalRead(IRFLAME_3V3_PIN_AOUT);
  if (flameValue) {
    digitalWrite(Buzzer, HIGH);
  } else {
    digitalWrite(Buzzer, LOW);
  }
  char key = readKeypad();

  handleKeypad(key);
  handleRFID();

  publishSensorData();
  client.loop();
  delay(1000);
}



void connectToWiFi() {
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}


void connectToMQTT() {
  espClient.setCACert(root_ca);
  client.setServer(mqtt_server, 8883);

  while (!client.connected()) {
    if (client.connect(mqtt_client_id, mqtt_username, mqtt_password)) {
      Serial.println("Connected to MQTT");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void getSecurityData() {
  http.begin(serverSecurity);

  // Make the GET request
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    String response = http.getString();

    // Kiểm tra xem phản hồi có phải là chuỗi JSON hợp lệ hay không
    if (response.startsWith("{\"count\":")) {
      DynamicJsonDocument doc(2048);
      deserializeJson(doc, response);
      JsonArray data = doc["data"];

      for (JsonVariant security : data) {
        // Xử lý dữ liệu như trong vòng loop
        String name = security["name"];
        String primaryPassword = security["primaryPassword"];

        // KEYPAD cửa chính
        if (name == "KEYPAD") {
          if (password == primaryPassword) {
            Serial.println("KEYPAD ON");
            updateDeviceStatus("DOOR", true);
          } else {
            Serial.println("Error password - KEYPAD");
          }
        }
      }
    } else {
      Serial.println("Invalid JSON response from serverSecurity");
    }
  } else {
    Serial.println("Error getting data from serverSecurity");
  }
}

void updateDeviceStatus(const String& name, bool newStatus) {
  HTTPClient http;
  http.begin(serverName);  // Use the serverName for the initial GET request

  // Fetch the current data
  int httpResponseCode = http.GET();
  if (httpResponseCode > 0) {
    String response = http.getString();
    if (response.startsWith("{\"count\":")) {
      DynamicJsonDocument doc(4096);  // Increase the size if needed
      deserializeJson(doc, response);
      JsonArray data = doc["data"];

      // Find the device and update its status
      bool deviceFound = false;
      JsonObject deviceToUpdate;
      for (JsonVariant device : data) {
        String categoryName = device["category"]["name"];
        if (categoryName == name) {
          deviceFound = true;
          deviceToUpdate = device.as<JsonObject>();  // Store the device JSON
          break;
        }
      }

      if (deviceFound) {
        // Update the category fields
        JsonObject category = deviceToUpdate["category"];
        category["status"] = newStatus;

        // Convert the document back to a string
        String updateJson;
        serializeJson(deviceToUpdate, updateJson);

        // Construct the URL with the device ID
        String deviceId = deviceToUpdate["_id"].as<String>();
        String url = String(serverName) + "/" + deviceId;

        // Log the JSON data to be sent
        Serial.println("JSON data to be sent:");
        Serial.println(updateJson);

        // Send the PUT request with the updated data
        http.begin(url);  // Reinitialize HTTPClient for PUT request
        http.addHeader("Content-Type", "application/json");
        int putResponseCode = http.PUT(updateJson);
        if (putResponseCode > 0) {
          Serial.println("PUT request successful");
          String response = http.getString();
          Serial.println(response);
        } else {
          Serial.print("Error sending PUT request, response code: ");
          Serial.println(putResponseCode);
        }
      } else {
        Serial.println("Device not found in the current data");
      }
    } else {
      Serial.println("Invalid JSON response");
    }
  } else {
    Serial.print("Error getting data, response code: ");
    Serial.println(httpResponseCode);
  }

  http.end();  // Close the HTTP connection
}

void handleKeypad(char key) {
  if (key != 0) {
    Serial.println(key);
    lcd.setCursor(0, 1);

    if (key == 'D') {  // Nếu phím nhấn là 'D' (Xóa)
      password = "";   // Xóa inputString
      lcd.clear();
      lcd.setCursor(0, 1);
    } else if (key == '*') {
      getSecurityData();
      delay(2000);
      return;
    } else {
      password += key;  // Thêm ký tự vào inputString
      lcd.print(key);
    }
  }
}



void handleRFID() {
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    String cardID = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      cardID.concat(String(mfrc522.uid.uidByte[i], HEX));
    }
    Serial.print("Card UID: ");
    Serial.println(cardID);
    lcd.setCursor(0, 1);
    lcd.print(cardID);

    controlMotorBForDuration(false, 3000, 80);
    // controlMotorBForDuration(true, 2800, 80);

    cardID = "";
  }
}

void publishSensorData() {
  client.publish("SmartParking/temperature", String(dht.readTemperature()).c_str());
  client.publish("SmartParking/humidity", String(dht.readHumidity()).c_str());
  client.publish("SmartParking/MQ8", String(analogRead(MQ8_3V3_PIN_AOUT)).c_str());
  client.publish("SmartParking/flame", String(digitalRead(IRFLAME_3V3_PIN_AOUT)).c_str());
}

char readKeypad() {
  for (int row = 8; row < 12; row++) {
    mcp.digitalWrite(row, LOW);  // Kích hoạt hàng
    for (int col = 12; col < 16; col++) {
      if (mcp.digitalRead(col) == LOW) {  // Nếu cột đang được nhấn
        mcp.digitalWrite(row, HIGH);      // Disable hàng
        return keys[row - 8][col - 12];   // Trả về giá trị ký tự
      }
    }

    mcp.digitalWrite(row, HIGH);  // Disable hàng
  }

  return 0;  // Nếu không có phím nào được nhấn
}

void controlMotorAForDuration(bool forward, unsigned long duration, int speed) {
  // Thiết lập hướng quay của động cơ A
  bool in1 = forward ? HIGH : LOW;
  bool in2 = forward ? LOW : HIGH;
  mcp.digitalWrite(IN1_PIN, in1);
  mcp.digitalWrite(IN2_PIN, in2);
  analogWrite(ENA_PIN, speed);

  // Chờ trong một khoảng thời gian
  delay(duration);

  // Dừng động cơ A sau khi chạy xong
  analogWrite(ENA_PIN, 0);
}

void controlMotorBForDuration(bool forward, unsigned long duration, int speed) {
  // Thiết lập hướng quay của động cơ B
  bool in3 = forward ? HIGH : LOW;
  bool in4 = forward ? LOW : HIGH;
  mcp.digitalWrite(IN3_PIN, in3);
  mcp.digitalWrite(IN4_PIN, in4);
  // Điều chỉnh tốc độ động cơ B bằng PWM
  analogWrite(ENB_PIN, speed);

  // Chờ trong một khoảng thời gian
  delay(duration);

  // Dừng động cơ B sau khi chạy xong
  analogWrite(ENB_PIN, 0);
}