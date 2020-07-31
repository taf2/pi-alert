// see: https://github.com/ArduCAM/ArduCAM_ESP8266_UNO/blob/master/libraries/ArduCAM/examples/ESP8266/ArduCAM_ESP8266_UNO_Capture/ArduCAM_ESP8266_UNO_Capture.ino
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h> 

#include <ESP8266mDNS.h>
#include <Wire.h>
#include <ArduCAM.h>
#include <SPI.h>
#include "memorysaver.h"

#define OV2640_MINI_2MP

const int CS = 16;
const int RED_LED = 2;

const char *ssid = "<%= @config[:ssid] %>"; // Put your SSID here
const char *password = "<%= @config[:pass] %>"; // Put your PASSWORD here

ArduCAM myCAM(OV2640, CS);

static const size_t bufferSize = 4096;
static uint8_t buffer[bufferSize] = {0xFF};

void camCapture(ArduCAM myCAM) {
  WiFiClient client;
  myCAM.clear_fifo_flag();
  unsigned int i = 0;
  bool is_header = false;
  uint8_t temp = 0, temp_last = 0;

  if (!client.connect("<%= @config[:events][:host] %>", <%= @config[:events][:port] %>)) {
    Serial.println(F("Connection error to <%= @config[:events][:host] %>:<%= @config[:events][:port] %>"));
    return;
  }
  Serial.print("connected starting read and upload");

  uint32_t len  = myCAM.read_fifo_length();
  if (len >= MAX_FIFO_SIZE) { //8M
    Serial.println(F("Over size."));
    return;
  }
  if (len == 0 ) { //0 kb
    Serial.println(F("Size is 0."));
    return;
  }
  Serial.print("doing the POST headers");
  client.setNoDelay(true);
  client.println("POST /event HTTP/1.1");
  client.println("Host: <%= @config[:events][:host] %>");
  client.println("Connection: close");
  client.println("Content-Type: multipart/form-data; boundary=----WebKitFormBoundaryjg2qVIUS8teOAbN3");
  client.print("Content-Length: ");
  client.println(len+196);  //+196 is owerhead form boundary tags
  client.println();
  client.println("------WebKitFormBoundaryjg2qVIUS8teOAbN3");
  client.println("Content-Disposition: form-data; name=\"FileGDF\"; filename=\"image19.jpg\"");
  client.println("Content-Type: application/octet-stream");
  client.println();
  Serial.print("post headers sent");

  myCAM.CS_LOW();
  myCAM.set_fifo_burst();
  if (!client.connected()) { return; }
  Serial.print("reading data into buffers to send ");
  while (len--) {
    temp_last = temp;
    temp =  SPI.transfer(0x00);
    //Read JPEG data from FIFO
    if ( (temp == 0xD9) && (temp_last == 0xFF) ) { //If find the end ,break while,
      buffer[i++] = temp;  //save the last  0XD9
      //Write the remain bytes in the buffer
      if (!client.connected()) break;
      client.write(&buffer[0], i);
      is_header = false;
      i = 0;
      myCAM.CS_HIGH();
      break;
    }

    if (is_header == true) {
      //Write image data to buffer if not full
      if (i < bufferSize) {
        buffer[i++] = temp;
      } else {
        //Write bufferSize bytes image data to file
        if (!client.connected()) { break; }
        client.write(&buffer[0], bufferSize);
        i = 0;
        buffer[i++] = temp;
      }
    } else if ((temp == 0xD8) & (temp_last == 0xFF)) {
      is_header = true;
      buffer[i++] = temp_last;
      buffer[i++] = temp;
    }
  }
  client.stop();
}

void postEvent() {
  WiFiClient client;
  HTTPClient http;
  String postData = "{\"hello\":\"world\"}";
  String url = "http://<%= @config[:events][:host] %>:<%= @config[:events][:port] %>/event";
  Serial.println("POST:");
  Serial.println(url.c_str());

  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");
  http.POST(postData);
  http.writeToStream(&Serial);
  http.end();

}

void connectWifi() {
  digitalWrite(RED_LED, HIGH);
  
  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print(F("Connecting to "));
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(RED_LED, LOW);
    delay(500);
    digitalWrite(RED_LED, HIGH);
    Serial.print(F("."));
  }
  digitalWrite(RED_LED, LOW);
  Serial.println(F("WiFi connected"));
  Serial.println("");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  delay(8000);
  pinMode(RED_LED, OUTPUT); // red led
  digitalWrite(RED_LED, HIGH);
  myCAM.set_format(JPEG);
  myCAM.InitCAM();
  myCAM.OV2640_set_JPEG_size(OV2640_1024x768);
  connectWifi();
}

void motionCapture() {
  myCAM.flush_fifo();
  myCAM.clear_fifo_flag();
  myCAM.start_capture();

  Serial.println(F("CAM Capturing"));
  int total_time = 0;
  total_time = millis();
//  while (!(myCAM.read_reg(ARDUCHIP_TRIG) & CAP_DONE_MASK));
  while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));

  total_time = millis() - total_time;
  Serial.print(F("capture total_time used (in miliseconds):"));
  Serial.println(total_time, DEC);
  total_time = 0;
  Serial.println(F("CAM Capture Done."));
  total_time = millis();
  camCapture(myCAM);

  total_time = millis() - total_time;
  Serial.print(F("send total_time used (in miliseconds):"));
  Serial.println(total_time, DEC);
  Serial.println(F("CAM send Done."));
}

void loop() {
  int motion = analogRead(A0);
  if (motion > 1000) {
    Serial.println("motion detected");
    digitalWrite(RED_LED, HIGH);
    Serial.println("signal event");
    //postEvent();
    motionCapture();

    Serial.println("event delivered waiting 10 seconds");
    delay(10000);
    Serial.println("going low");
    digitalWrite(RED_LED, LOW);
    delay(2000);
  } else {
    digitalWrite(RED_LED, LOW);
  }
}