#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <HTTPClient.h>
#define SSID "<%= @config[:ssid] %>"
#define PASSWORD "<%= @config[:pass] %>"
#include "I2SSampler.h"
#define LED 27

WiFiClient *wifiClient = NULL;
HTTPClient *httpClient = NULL;
I2SSampler *sampler = NULL;

// replace this with your machines IP Address
#define SERVER_URL "http://<%= @config[:sound_server][:host] %>:<%= @config[:sound_server][:port] %>/samples"

// Task to write samples to our server
void writerTask(void *param)
{
  sampler = (I2SSampler *)param;
  const TickType_t xMaxBlockTime = pdMS_TO_TICKS(100);
  while (true) {
    // wait for some samples to save
    uint32_t ulNotificationValue = ulTaskNotifyTake(pdTRUE, xMaxBlockTime);
    if (ulNotificationValue > 0) {
      // send them off to the server
      digitalWrite(LED, HIGH);
      Serial.println("Sending data");
      httpClient->begin(*wifiClient, SERVER_URL);
      httpClient->addHeader("Content-Type", "application/octet-stream");
      httpClient->POST((uint8_t *)sampler->sampleBuffer(), sampler->numSamples() * sizeof(uint16_t));
      httpClient->end();
      digitalWrite(LED, LOW);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  // launch WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);
  if (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  Serial.println("Started up");
  // indicator LED
  pinMode(27, OUTPUT);
  // setup the HTTP Client
  wifiClient = new WiFiClient();
  httpClient = new HTTPClient();

  // create our sampler
  sampler = new I2SSampler();
  // set up the sample writer task
  TaskHandle_t writerTaskHandle;
  xTaskCreatePinnedToCore(writerTask, "Writer Task", 8192, sampler, 1, &writerTaskHandle, 1);
  // start sampling
  sampler->start(writerTaskHandle);
}

void loop()
{
  // nothing to do here - everything is taken care of by tasks
}
