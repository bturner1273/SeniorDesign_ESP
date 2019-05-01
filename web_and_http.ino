#include <ArduinoJson.h>
#include "FirebaseESP8266.h"
#include <ESP8266WiFi.h>
//Include the SSL client
#include <WiFiClientSecure.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

#define FIREBASE_HOST "smart-air-810ae.firebaseio.com"
#define MUX_ADDR 0x70

LiquidCrystal_I2C lcd(0x27, 20, 4);
char ssid[] = "Brad";       //SSID AND PASSWORD WILL NOT BE USED
char password[] = "12731273"; //ONCE WIFIMANAGER IS IMPLEMENTED
int unique_id;

//Add a SSL client
WiFiClientSecure client;

//Add firebase client
FirebaseData firebaseData;

String path; //this is the path to our user in the database
//float hum, temp, co2, o2, h2;

void lcd_setup(){
  lcd.init();
  lcd.backlight();
  lcd.setCursor(1, 0);
  lcd.print("Smart Air");
  lcd.setCursor(1, 1);
  lcd.print("Brad and Jeff");
}

void wifi_setup(){
  // Set WiFi to station mode and disconnect from an AP if it was Previously
  // connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // Attempt to connect to Wifi network:
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);
}

void firebase_setup(){
  Firebase.begin(FIREBASE_HOST, get_ESP_service_key());
}

void firebase_initial_push(){
  path = "/unclaimed_esps/esp" + String(unique_id);
  while(!Firebase.setString(firebaseData, path + "/user_path", "undefined")){
    Serial.println("Err: " + firebaseData.errorReason());
  }
  path = "undefined";
}

void firebase_initial_user_push(){
  String init_value = "\"val\": 0";
  String temp_json = "\"temp\":{" + init_value + "}",
  hum_json = "\"hum\":{" + init_value + "}",
  o2_json = "\"o2\":{" + init_value + "}", 
  h2_json = "\"h2\":{" + init_value + "}",
  co2_json="\"co2\":{" + init_value + "}";
  String json = "{\"name\":\"master\"," + temp_json + "," + hum_json + "," + o2_json + "," + h2_json + "," + co2_json + "}";
  while (!Firebase.setJSON(firebaseData, path + "/esp" + String(unique_id), json)) {
    Serial.println("Err: " + firebaseData.errorReason());    
  }
}

void firebase_push_data(){
  int _size = 5;
  float temp[_size], hum[_size], h2[_size], o2[_size], co2[_size];
  for (int i = 0; i < _size; i++) {
    temp[i] = random(0, 1000) / 100.0;
    hum[i] = random(0, 1000) / 100.0;
    h2[i] = random(0, 1000) / 100.0;
    o2[i] = random(0, 1000) / 100.0;
    co2[i] = random(0, 1000) / 100.0;
  }
  for (int i = 0; i < _size; i++) {
    while (!Firebase.pushFloat(firebaseData, path + "/esp" + String(unique_id) + "/temp", temp[i])) {
      Serial.println("Err: " + firebaseData.errorReason()); 
    }
    while (!Firebase.pushFloat(firebaseData, path + "/esp" + unique_id + "/hum", hum[i])) {
      Serial.println("Err: " + firebaseData.errorReason()); 
    }
    while (!Firebase.pushFloat(firebaseData, path + "/esp" + unique_id + "/h2", h2[i])) {
      Serial.println("Err: " + firebaseData.errorReason()); 
    }
    while (!Firebase.pushFloat(firebaseData, path + "/esp" + unique_id + "/o2", o2[i])) {
      Serial.println("Err: " + firebaseData.errorReason()); 
    }
    while (!Firebase.pushFloat(firebaseData, path + "/esp" + unique_id + "/co2", co2[i])) {
      Serial.println("Err: " + firebaseData.errorReason()); 
    }
  }
}

void connect_to_user(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("ID: ");
  lcd.setCursor(4, 0);
  lcd.print(String(unique_id));
  lcd.setCursor(0, 1);
  lcd.print("Go to the SmartAir");
  lcd.setCursor(0, 2);
  lcd.print("website to connect");
  lcd.setCursor(0, 3);
  lcd.print("device.");

  while (path.equals("undefined")) {
    if (Firebase.getString(firebaseData, "/unclaimed_esps/esp" + String(unique_id) + "/user_path")) {
      if (!firebaseData.stringData().equals("undefined")) {
        path = firebaseData.stringData();
        Serial.print("new path: ");
        Serial.println(path);
        firebase_initial_user_push();
        while (!Firebase.deleteNode(firebaseData, "/unclaimed_esps/esp" + String(unique_id))) {
          Serial.println(firebaseData.errorReason());
        }
      }
    } 
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Device Connected");
}

void setup() {
  randomSeed(analogRead(A0));
  Serial.begin(115200);
  lcd_setup();
  wifi_setup();
  unique_id = get_ESP_unique_id();
  firebase_setup();
  firebase_initial_push();
  delay(1000);
  connect_to_user();
}

void tca_select (uint8_t bus) {
  if (bus > 7) return;
  Wire.beginTransmission(MUX_ADDR);
  Wire.write(1 << bus);
  Wire.endTransmission();
}

String get_ESP_service_key(){
  String headers = "";
  String body = "";
  bool finishedHeaders = false;
  bool currentLineIsBlank = true;
  bool gotResponse = false;
  long now;
  char host[] = "smart-air-810ae.firebaseapp.com";
  String service_key;
  if (client.connect(host, 443)) {
    String URL = "/esp/service_key";
    client.println("GET " + URL + " HTTP/1.1");
    client.print("Host: "); client.println(host);
    client.println("User-Agent: arduino/1.0");
    client.println("");
    now = millis();
    // checking the timeout
    while (millis() - now < 1500) {
      while (client.available()) {
        char c = client.read();
        if (finishedHeaders) {
          body=body+c;
        } else {
          if (currentLineIsBlank && c == '\n') {
            finishedHeaders = true;
          }
          else {
            headers = headers + c;
          }
        }
        if (c == '\n') {
          currentLineIsBlank = true;
        }else if (c != '\r') {
          currentLineIsBlank = false;
        }
        //marking we got a response
        gotResponse = true;
      }
      if (gotResponse) {
        DynamicJsonBuffer jsonBuffer;
        JsonObject& root = jsonBuffer.parseObject(body);
        if (root.success()) {
          if (root.containsKey("service_key")) {
            service_key = root["service_key"].as<String>();
          } 
        } else {
          Serial.println("failed to parse JSON");
        }
        break;
      }
    }
  }  
    return service_key;
}

int get_ESP_unique_id(){
  String headers = "";
  String body = "";
  bool finishedHeaders = false;
  bool currentLineIsBlank = true;
  bool gotResponse = false;
  long now;
  char host[] = "smart-air-810ae.firebaseapp.com";
  int id;
  if (client.connect(host, 443)) {
    String URL = "/esp/unique_id";
    client.println("GET " + URL + " HTTP/1.1");
    client.print("Host: "); client.println(host);
    client.println("User-Agent: arduino/1.0");
    client.println("");
    now = millis();
    // checking the timeout
    while (millis() - now < 1500) {
      while (client.available()) {
        char c = client.read();
        if (finishedHeaders) {
          body=body+c;
        } else {
          if (currentLineIsBlank && c == '\n') {
            finishedHeaders = true;
          }
          else {
            headers = headers + c;
          }
        }
        if (c == '\n') {
          currentLineIsBlank = true;
        }else if (c != '\r') {
          currentLineIsBlank = false;
        }
        //marking we got a response
        gotResponse = true;
      }
      if (gotResponse) {
        DynamicJsonBuffer jsonBuffer;
        JsonObject& root = jsonBuffer.parseObject(body);
        if (root.success()) {
          if (root.containsKey("unique_id")) {
            id = root["unique_id"].as<int>();
          } 
        } else {
          Serial.println("failed to parse JSON");
        }
        break;
      }
    }
  }  
    return id; 
}

void loop() {
  firebase_push_data();
  delay(60000);
}
