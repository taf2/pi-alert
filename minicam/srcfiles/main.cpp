/*
 * Based on tutorials found on the internets use deep sleep and an RCWL-0516 microwave motion detection board
 * to wake up the esp32cam, take a picture, and upload the picture if possible to our servers for image processing
 **/
#include "esp_camera.h"
#include "Arduino.h"
#include "FS.h"                // SD Card ESP32
#include "SD_MMC.h"            // SD Card ESP32
#include "soc/soc.h"           // Disable brownour problems
#include "soc/rtc_cntl_reg.h"  // Disable brownour problems
#include "driver/rtc_io.h"
#include "WiFi.h"

#include <EEPROM.h>            // read and write from flash memory
// define the number of bytes you want to access
#define EEPROM_SIZE 1
#define TIMEOUT      20000

const char* ssid     = "<%= @config[:ssid] %>";
const char* password = "<%= @config[:pass] %>";
const char* events_host = "<%= @config.dig(:events,:host) %>";
const int   events_port = <%= @config.dig(:events,:port) %>;

const char* BOUNDARY= "----------------v7MlwafBAXWMW4t7eECQ-----------------";

RTC_DATA_ATTR int bootCount = 0;

// Pin definition for CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

int pictureNumber = 0;

void errorBlink(int high) {
  for (int i = 0; i < 10; ++i) {
    digitalWrite(GPIO_NUM_12, LOW);
    delay(500);
    digitalWrite(GPIO_NUM_12, HIGH);
  }
  if (high) {
    digitalWrite(GPIO_NUM_12, HIGH);
  }
}

String saveImage(camera_fb_t *fb, String path) {
  if (!SD_MMC.begin()) {
    Serial.println("SD Card Mount Failed");
    return "SD Card Mount Failed";
  }

  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD Card attached");
    return "No SD Card attached";
  }

  fs::FS &fs = SD_MMC;
  Serial.printf("Picture file name: %s\n", path.c_str());

  File file = fs.open(path.c_str(), FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file in writing mode");
  } else {
    file.write(fb->buf, fb->len); // payload (image), payload length
    Serial.printf("Saved file to path: %s\n", path.c_str());
  }
  file.close();
  return "";
}

/*
 * uploadImage
 * make an HTTP POST multipart/form-data submission to events_port and events_host
 * e.g. POST http://#{events_host}:#{events_port}/events -d filename=motion-capture.jpg
 * see: https://stackoverflow.com/questions/53264373/esp32-try-to-send-image-file-to-php-with-httpclient
 **/
String header(String token,size_t length) {
  String  data;
  data =  F("POST /events HTTP/1.1\r\n");
  data += F("cache-control: no-cache\r\n");
  data += F("Content-Type: multipart/form-data; boundary=");
  data += BOUNDARY;
  data += "\r\n";
  data += F("User-Agent: <%= @config[:events][:name] %>\r\n");
  data += F("Accept: */*\r\n");
  data += F("Host: ");
  data += events_host;
  data += F("\r\n");
  data += F("accept-encoding: gzip, deflate\r\n");
  data += F("Connection: keep-alive\r\n");
  data += F("content-length: ");
  data += String(length);
  data += "\r\n";
  data += "\r\n";
  return data;
}

String body(String content , String message) {
  String data;
  data = "--";
  data += BOUNDARY;
  data += F("\r\n");
  if(content=="imageFile") {
    data += F("Content-Disposition: form-data; name=\"imageFile\"; filename=\"picture.jpg\"\r\n");
    data += F("Content-Type: image/jpeg\r\n");
    data += F("\r\n");
  } else {
    data += "Content-Disposition: form-data; name=\"" + content +"\"\r\n";
    data += "\r\n";
    data += message;
    data += "\r\n";
  }
  return data;
}

String uploadImage(camera_fb_t *fb) {
// String sendImage(String token,String message, uint8_t *data_pic,size_t size_pic)
  //         res = sendImage("asdw","A54S89EF5",cam.getfb(),cam.getSize());
  const uint8_t *data_pic = fb->buf;
  const size_t size_pic   = fb->len;
  String token   = "coolbeans";
  String message = "hello";
  String bodyTxt = body("message",message);
  String bodyPic = body("imageFile",message);
  String bodyEnd = String("--")+BOUNDARY+String("--\r\n");
  size_t allLen  = bodyTxt.length()+bodyPic.length()+size_pic+bodyEnd.length();
  String headerTxt =  header(token,allLen);

  //WiFiClientSecure client;
  WiFiClient client;
  if (!client.connect(events_host,events_port)) {
    errorBlink(1);
    return "connection failed";
  }

  client.print(headerTxt+bodyTxt+bodyPic);
  client.write(data_pic,size_pic);
  client.print("\r\n"+bodyEnd);

  delay(20);
  long tOut = millis() + TIMEOUT;
  while (client.connected() && tOut > millis()) {
    if (client.available()) {
      String serverRes = client.readStringUntil('\r');
      return serverRes;
    }
  }
  return "No Response?";
}

void snapshot(String (*handler)(camera_fb_t *)) {
  camera_config_t config;
  // configure the board to use the camera
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Init Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    errorBlink(1);
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // blink the red light
  digitalWrite(GPIO_NUM_12, LOW);
  delay(500);
  digitalWrite(GPIO_NUM_12, HIGH);

  camera_fb_t * fb = NULL;

  // Take Picture with Camera
  fb = esp_camera_fb_get();
  if (!fb) {
    errorBlink(1);
    Serial.println("Camera capture failed");
    return;
  }

  (*handler)(fb);

  esp_camera_fb_return(fb);
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // disable brownout detector... we're on battery hopefully this is ok...

  pinMode(GPIO_NUM_13, INPUT); // PIR/motion sensor radar
  pinMode(GPIO_NUM_12, OUTPUT); // led for showing we detected motion

  Serial.begin(115200);
  // blink once on boot
  digitalWrite(GPIO_NUM_12, HIGH);
  delay(1000);
  digitalWrite(GPIO_NUM_12, LOW);
  delay(1000);
}

void loop() {
  int motion = digitalRead(GPIO_NUM_13);
  if (motion == HIGH) {
    digitalWrite(GPIO_NUM_12, HIGH);
    errorBlink(1);
    snapshot(&uploadImage);
    delay(1000); // pause
  } else {
    digitalWrite(GPIO_NUM_12, LOW);
  }
}
