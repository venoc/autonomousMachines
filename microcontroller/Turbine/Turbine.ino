#include "WiFi.h"
#include <Adafruit_NeoPixel.h>
#include <WebServer.h>
#include "index.h"
#include <Arduino_JSON.h>
#include <HTTPClient.h>

#define IN1  4
#define IN2  16
#define IN3  17
#define IN4  5
#define LED_PIN 18

#define LED_COUNT 20
#define BRIGHTNESS 120

#define period  1000

HTTPClient http;
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
uint32_t green = strip.Color(0, BRIGHTNESS, 0); //<= 255
uint32_t red = strip.Color(BRIGHTNESS, 0, 0);
uint32_t off = strip.Color(0, 0, 0);


const char* ssid     = "ssid";
const char* password = "password";
boolean motorStart = false;
WebServer server(80);
int ledStart = 3  ;
int Steps = 0;
int8_t nextLED = LED_COUNT - 1;
boolean Direction = false;
unsigned long last_time, last_time_led = 0;
unsigned long currentMillis ;
String seed = "SEEDC9999999999999999999999999999999999999999999999999999999999999999999999999999";
long time1;
String mamKeys[50];
String addresses[50];
String mamRoots[50];
int mamIndex = -1;
String mamState = "";


String node = "http://192.168.1.2:3000/api";
String address = "0x000000000000000000";
int iotas  = 0;
int last_balance =  0;
float power = 4.5;
unsigned long time_now = 0;
unsigned long time_now2 = 0;
boolean isSending = false;
void setup()
{
  Serial.begin(115200);
  Serial.println("Starting ESP32 - Wind Turbine...");
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  strip.begin();
  strip.show();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected");
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/toggleMotor", toggleMotor);
  server.on("/changeLED", handleLED);
  server.on("/charger", handleCharger);
  server.on("/getQRCodeData", handleQR);
  server.begin();
  for (int i = 0; i < 50; i++) {
    mamKeys[i] = "key" ;
    for (int j = 0; j < 6; j++) {
      char randomletter = 'A' + (random(0, 25));
      mamKeys[i] += randomletter;
    }
    mamRoots[i] = "";
  }

}
void changeLED() {
  if (millis() - last_time_led >= 100) {
    last_time_led = millis();
    switch (ledStart) {
      case 1: strip.setPixelColor(nextLED, green); break;
      case 2: strip.setPixelColor(nextLED, red); break;
      case 3: strip.setPixelColor(nextLED, off); break;
      default: strip.setPixelColor(nextLED, off); break;
    }
    nextLED -= 1;
    if (nextLED <  0) {
      nextLED = LED_COUNT - 1;
      ledStart = 0 ;
    }
    strip.show();
  }
}
void loop()
{
  server.handleClient();
  if (ledStart != 0) {
    changeLED();
  }
  if (motorStart) {
    currentMillis = micros();
    if (currentMillis - last_time >= 1000) {
      stepper(1);
      time1 = time1 + micros() - last_time;
      last_time = micros();
    }
  }
  service();
}

void stepper(int xw) {
  for (int x = 0; x < xw; x++) {
    switch (Steps) {
      case 0:
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, LOW);
        digitalWrite(IN3, LOW);
        digitalWrite(IN4, HIGH);
        break;
      case 1:
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, LOW);
        digitalWrite(IN3, HIGH);
        digitalWrite(IN4, HIGH);
        break;
      case 2:
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, LOW);
        digitalWrite(IN3, HIGH);
        digitalWrite(IN4, LOW);
        break;
      case 3:
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, HIGH);
        digitalWrite(IN3, HIGH);
        digitalWrite(IN4, LOW);
        break;
      case 4:
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, HIGH);
        digitalWrite(IN3, LOW);
        digitalWrite(IN4, LOW);
        break;
      case 5:
        digitalWrite(IN1, HIGH);
        digitalWrite(IN2, HIGH);
        digitalWrite(IN3, LOW);
        digitalWrite(IN4, LOW);
        break;
      case 6:
        digitalWrite(IN1, HIGH);
        digitalWrite(IN2, LOW);
        digitalWrite(IN3, LOW);
        digitalWrite(IN4, LOW);
        break;
      case 7:
        digitalWrite(IN1, HIGH);
        digitalWrite(IN2, LOW);
        digitalWrite(IN3, LOW);
        digitalWrite(IN4, HIGH);
        break;
      default:
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, LOW);
        digitalWrite(IN3, LOW);
        digitalWrite(IN4, LOW);
        break;
    }
    SetDirection();
  }
}

void SetDirection() {
  if (Direction == 1) {
    Steps++;
  }
  if (Direction == 0) {
    Steps--;
  }
  if (Steps > 7) {
    Steps = 0;
  }
  if (Steps < 0) {
    Steps = 7;
  }
}
void handleRoot() {
  String s = MAIN_page; //Read HTML contents
  Serial.println("Client opens Website");
  server.send(200, "text/html", s); //Send web page
}
void handleLED() {
  Serial.println("Change LED");
  if ( server.argName(0) == "color") {
    colorToCode(server.arg(0));
  }
  server.send(200, "text/html", server.arg(0) );

}
void handleCharger() {
  Serial.println("Change LED");
  if ( server.argName(0) == "power") {
    if (server.arg(0) == "on" && isSending == false) {
      mamIndex++;
      isSending = true;
      String getMessage = String("/getAddress?seed=") + seed + "&index=" + mamIndex;
      addresses[mamIndex] = sendGet(node + getMessage);
      Serial.print("New Address: ");
      Serial.println(addresses[mamIndex]);
      getMessage = "/mamInit?key=" + mamKeys[mamIndex];
      mamState = sendGet(node + getMessage);
      server.send(200, "text/html",addresses[mamIndex]  );
    }
    if (server.arg(0) == "off" && isSending == true) {
      isSending = false;
      server.send(200, "text/html","true"  );
    }
  }

}
void handleQR() {
  String text = "";
  Serial.println("Make QRCode");
  if ( server.argName(0) == "index") {
     Serial.print("Index: ");
     Serial.println(server.argName(0));
    int index = server.arg(0).toInt();
    text += "{"; 
    text += "\"root\":";
    text += String(mamRoots[index]);
    text += ",\"key\":\"";
    text += String(mamKeys[index]);
    text += "\"}";
    Serial.println(text);
  }
  server.send(200, "text/html", text );

}
void colorToCode(String color) {
  if (color == "green") {
    ledStart = 1;
    nextLED = LED_COUNT  - 1;
  }
  else if (color == "red") {
    ledStart = 2;
    nextLED = LED_COUNT  - 1;
  }
  else if (color == "off") {
    ledStart = 3;
    nextLED = LED_COUNT  - 1;
  }
}
void toggleMotor() {
  Serial.println("Motor toggled");
  motorStart = !motorStart;
  if (motorStart)
    server.send(200, "text/plane", "ON");
  else
    server.send(200, "text/plane", "OFF");
}
String sendGet(String req) {
  http.begin(req);
  Serial.print("GET: ");
  Serial.println(req);
  int httpCode = http.GET();

  if (httpCode > 0) { //Check for the returning code

    String payload = http.getString();
    Serial.println(httpCode);
    Serial.println(payload);
    return (payload);
  }

  else {
    Serial.println("Error on HTTP request");
    http.end();
    return ("Error on HTTP request");
  }

  http.end();
}
void service() {

  if (millis() > time_now2 + 5000) {
    time_now2 = millis();
    //true solange Charger STrom benötigt  --> Muss über Post gesteuert werden toggle
    //Post mamSend mamState + JSON als String Sensordata
    if ((WiFi.status() == WL_CONNECTED) && isSending) { //Check the current connection status
      http.begin(node + "/mamSend");
      http.addHeader("Content-Type", "application/json");
      String req = "{ \"mamState\": " + mamState + ", \"msg\": \"{\\\"timestamp\\\": " + millis() + ", \\\"address\\\": \\\"" + addresses[mamIndex] +"\\\", \\\"power\\\": " + (power +random(0,99) /100.0) + "}\"}";
      Serial.println(req);
      int httpCode = http.POST(req);
      Serial.println(httpCode);
      if (httpCode == 200) { //Check for the returning code
        String output;
        output = http.getString();
        JSONVar myObject = JSON.parse(output);
        Serial.println(output);
        Serial.println(mamRoots[mamIndex]);
        if (JSON.typeof(myObject) != "undefined") {
          if (myObject.hasOwnProperty("root") && mamRoots[mamIndex].equals("")) {
            Serial.println("Get root");
            mamRoots[mamIndex] = (String) JSON.stringify(myObject["root"]);
          }
          if (myObject.hasOwnProperty("mamState")) {
            mamState = JSON.stringify(myObject["mamState"]);
          }
          else {
            Serial.println("no mamState");
          }
        }
        Serial.print("MAM:  ");
        Serial.println(mamState);
        Serial.println(httpCode);
      }

      else {
        Serial.println("Error on HTTP request");
      }

      http.end(); //Free the resources
    }
    Serial.print("Key: ");
    Serial.println(mamKeys[mamIndex]);
    Serial.print("Root: ");
    Serial.println(mamRoots[mamIndex]);
  }
  /*
  if (millis() > time_now + period) {
    String req = node + "/getBalance?address=" + addresses[mamIndex];
    int b = sendGet(req).toInt();
    if (b >  last_balance) {
      iotas += b - last_balance;
      last_balance = b;

      state = true;
        toggle = true;
    }
    time_now = millis();
    if (motorStart) {
      if (iotas - abs(power) >= 0) {
        iotas -= power;
        colorToCode("green");
        state = true;
          toggle = false;
          digitalWrite(TRANSISTOR, toggle);
      }/*
          else{
            state = false;
            toggle = true;
            digitalWrite(TRANSISTOR, toggle);
          }*/
    //}
  //}
}
