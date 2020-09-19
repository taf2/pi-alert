#include <WiFi.h>
#include <Wire.h>
#include <Arduino.h>
#include <Adafruit_VC0706.h>
#include <SPI.h>
#include <pthread.h>

#define LED_PIN A0

#define CameraConnection Serial1
#define PICBUF 54000

static void connectToWiFi(const char * ssid, const char * pwd);
static void cameraInit();
static int uploadPicture();
static char outbuffer[1024];
static uint8_t picbuf[PICBUF];
static uint16_t picOffset = 0;

const char *ssid    =  "<%= @config[:ssid] %>";
const char *pass    =  "<%= @config[:pass] %>";
Adafruit_VC0706 cam = Adafruit_VC0706(&CameraConnection);

void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial.println(F("Booting"));
  pinMode(LED_PIN, OUTPUT);
  connectToWiFi(ssid, pass);

  delay(1000);

  cameraInit();
  Serial.println("Snap in 3 secs...");
  delay(3000);
  uploadPicture();
  cam.setMotionDetect(true);           // turn it on
    // You can also verify whether motion detection is active!
  Serial.print("Motion detection is ");
  if (cam.getMotionDetect())
    Serial.println("ON");
  else
    Serial.println("OFF");

}

void loop() {

  if (cam.motionDetected()) {
    digitalWrite(LED_PIN, HIGH);
    Serial.println("Motion!");   
    cam.setMotionDetect(false);
    delay(8000);
    uploadPicture();
    cam.resumeVideo();
    cam.setMotionDetect(true);
    digitalWrite(LED_PIN, LOW);
  } else {
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    digitalWrite(LED_PIN, LOW);
    delay(500);
  }


}

void connectToWiFi(const char * ssid, const char * pwd) {
  int ledState = 0;

  Serial.println("Connecting to WiFi network: " + String(ssid));

  WiFi.begin(ssid, pwd);

  while (WiFi.status() != WL_CONNECTED) {
    // LED_PIN LED while we're connecting:
    digitalWrite(LED_PIN, ledState);
    ledState = (ledState + 1) % 2; // Flip ledState
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

static void cameraInit() {
  Serial.println("VC0706 Camera Init");

  // Try to locate the camera
  if (cam.begin()) { // 2400)) {//115200)) {
    Serial.println("Camera Found:");
  } else {
    digitalWrite(LED_PIN, HIGH);
    while(!cam.begin()) {
      Serial.println("No camera found?");
      delay(1000);
    }
    return;
  }

    // Print out the camera version information (optional)
  char *reply = cam.getVersion();
  if (reply == 0) {
    Serial.print("Failed to get version");
  } else {
    Serial.println("-----------------");
    Serial.print(reply);
    Serial.println("-----------------");
  }

  // Set the picture size - you can choose one of 640x480, 320x240 or 160x120
  // Remember that bigger pictures take longer to transmit!

//  cam.getCompression();
//  cam.setCompression(0x95);
//  cam.getCompression();
//
  // boolean status = cam.setImageSize(VC0706_640x480);        // biggest
  // boolean status = cam.setImageSize(VC0706_320x240);        // medium
  boolean status = cam.setImageSize(VC0706_160x120);          // small
  if (!status) {
    Serial.println("error setting image size");
  }

  // You can read the size back from the camera (optional, but maybe useful?)
  uint8_t imgsize = cam.getImageSize();
  if (imgsize == -1) {
    Serial.println("error getting image size");
  } else {
    Serial.print("Image size: ");
    if (imgsize == VC0706_640x480) Serial.println("640x480");
    if (imgsize == VC0706_320x240) Serial.println("320x240");
    if (imgsize == VC0706_160x120) Serial.println("160x120");
  }

}

// consumes data into picbuf returns jpglen remaining
// advances picOffset to the end of picbuf
uint16_t bufferCameraData(uint16_t jpglen) {
  // read 32 bytes at a time;
  const uint8_t bytesToRead = min((uint16_t)72, jpglen); // change 32 to 64 for a speedup but may not work with all setups!
  const uint8_t *imgbuf     = cam.readPicture(bytesToRead);
  const uint8_t bytesAvailable = cam.available();

  if (bytesAvailable == 0) {
    return jpglen;
  }
  const uint8_t bytesInBuffer = min(bytesAvailable, bytesToRead);
  memcpy(picbuf+picOffset, imgbuf, bytesInBuffer);
  picOffset += bytesInBuffer;
  return jpglen - bytesInBuffer;
}

int uploadPicture() {
  WiFiClient client;

  int32_t time = millis();
  picOffset = 0;

  if (!cam.takePicture()) {
    Serial.println("Failed to snap!");
    return -1; // error
  } else {
    Serial.println("Picture taken!");
  }

  if (!client.connect("<%= @config[:events][:host] %>", <%= @config[:events][:port] %>)) {
    Serial.println(F("Connection error to <%= @config[:events][:host] %>:<%= @config[:events][:port] %>"));
    return -2;
  }
  Serial.println("connected starting read and upload");

  uint16_t jpglen = cam.frameLength();
  if (jpglen == 0 ) { //0 kb
    Serial.println(F("Size is 0."));
    client.stop();
    return -3;
  }
  Serial.println("Upload picture");

  client.setNoDelay(true);
  client.setTimeout(64);

  snprintf(outbuffer, 1024, "POST /capture HTTP/1.1\r\nHost: <%= @config[:events][:host] %>\r\nConnection: close\r\nContent-Type: image/jpeg\r\nContent-Length: %d\r\n\r\n", jpglen);
  client.write(outbuffer);
  Serial.write(outbuffer);

  while (jpglen > 0 && client.connected()) {
    jpglen = bufferCameraData(jpglen);
    if (picOffset >= PICBUF) {
      const size_t sentBytes = client.write(picbuf, PICBUF);
      if (sentBytes != picOffset) {
        snprintf(outbuffer, 1024, "Error bytes sent: %d != %d\n", sentBytes, PICBUF);
        Serial.println(outbuffer);
      }
      picOffset = 0;
      Serial.print('.');
    }
  }

  if (picOffset > 0) {
    const size_t sentBytes = client.write(picbuf, picOffset);
    if (sentBytes != picOffset) {
      snprintf(outbuffer, 1024, "Error bytes sent: %d != %d\n", sentBytes, picOffset);
      Serial.println(outbuffer);
    }
  }

  time = millis() - time;
  Serial.println("done!");
  Serial.print(time); Serial.println(" ms elapsed");

  Serial.println();
  Serial.println("closing connection");
  client.stop();
  return 0;
}
