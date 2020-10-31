#include <TinyPICO.h>
/*
   The goal of this is to get wifi ssid and password via bluetooth and then start up the bluetooth.
   we'll use eeprom to save the wifi password and ssid so reboots don't need to be configured again.
*/
#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include "eeprom_settings.h"

#define LED_CONFIGURED 25
#define LED_BLUE_CONFIG 26
#define ENABLE_CONFIG 33
#define BUZZER 32
#define MP3_PWR 21

int last_sent_key = 0;
const int BLE_KEY_CONFIG_SIZE = 2;
const char *BLE_SET_CONFIG_KEYS[] =  {"ssid", "pass"}; // no keys longer than 4

bool deviceConnected = false;
bool ConfigMode = false; //  when pressed we'll enter configuration mode

// Initialise the TinyPICO library
TinyPICO tp = TinyPICO();
BLEService *pService;
BLEServer *pServer;
BLECharacteristic *pCharacteristic;
EEPROMSettings settings;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
#define SERVICE_UUID           "e2f52832-2c23-4318-85cb-be11f7421999" //"6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "e5de2fab-bf9b-439d-81d1-24651d8201a7" //"6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "e6f61157-5e3a-4656-a412-de8610a63d76" //"6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

void disableBLE();

class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
  };

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
  }
};

class MyCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string rxValue = pCharacteristic->getValue();

    if (rxValue.length() > 0) {
      Serial.println("*********");
      Serial.print("Received Value: ");
      // expecting key:value pairs
      String key, value;
      bool isKey = true;

      for (int i = 0; i < rxValue.length(); i++) {
        Serial.print(rxValue[i]);
        if (rxValue[i] == ':') {
          isKey = false;
        } else {
          if (isKey) {
            key +=  rxValue[i];
          } else {
            value += rxValue[i];
          }
        }
      }

      Serial.println();
      if (key.length() > 0 && key == "ssid" && value.length() > 0) {
        Serial.println( (key + " = " + value).c_str() );
        memcpy(settings.ssid, value.c_str(), 32);
        settings.configured = true; 
        settings.save();
      } else if (key.length() > 0 && key == "pass" && value.length() > 0) {
        Serial.println( ( key + " = " + value).c_str() );
        memcpy(settings.pass, value.c_str(), 32);
        settings.configured = true; 
        settings.save();
      }


      // Do stuff based on the command received from the app
      /*if (rxValue == "A" || rxValue == "B") {
        Serial.println("Received config use it now to connect ot wifi");
        //digitalWrite(LED, HIGH);
        digitalWrite(LED_BLUE_CONFIG, LOW);
        digitalWrite(LED_CONFIGURED, HIGH);
        disableBLE();
      }*/

      Serial.println();
      Serial.println("*********");
    }
  }
};

MyServerCallbacks *callbacks = NULL;

void initWiFi(const char *ssid, const char *password) {
  // text display tests
  IPAddress ip;

  WiFi.begin(ssid, password);
  Serial.print("Connecting to");
  Serial.println(ssid);
  int count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    char c = '/';
    if (count % 2 == 0) {
      c = '/';
    } else {
      c = '\\';
    }
    Serial.print(c);
    Serial.print("Connecting to");
    Serial.print(ssid);
    Serial.print(c);
    yield();
    ++count;
  }
  ip = WiFi.localIP();
  Serial.println(ip);
  Serial.print("Connected to ");
  Serial.print(ssid);
  Serial.print(" with ");
  Serial.print(ip.toString().c_str());

  delay(2000);
}

void startSong() {
  digitalWrite(MP3_PWR, HIGH); // send power to the device
  // it takes sometime for the relay to warm up to give power to the device... we need to delay here to ensure it had time 
  delay(1000);
  digitalWrite(BUZZER, LOW); // play music
  delay(100);
  digitalWrite(BUZZER, HIGH); // play music
}
void stopSong() {
  digitalWrite(MP3_PWR, LOW); // CUT the power
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_CONFIGURED, OUTPUT);
  pinMode(LED_BLUE_CONFIG, OUTPUT);
  pinMode(ENABLE_CONFIG, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(MP3_PWR, OUTPUT);
  digitalWrite(BUZZER, HIGH);
  digitalWrite(MP3_PWR, LOW);

	EEPROM.begin(EEPROM_SIZE);

  tp.DotStar_SetPower( false );

  if (settings.load() && settings.good()) {
    Serial.println("wifi is configured");
    Serial.println(settings.ssid);
    Serial.println(settings.pass);
    initWiFi(settings.ssid, settings.pass);
  }

  startSong();
}

void enableBLE() {
  // Create the BLE Device
  //  
  stopSong();
  Serial.println("enableBLE");
  if (!BLEDevice::getInitialized()) {
    Serial.println("init start");
    BLEDevice::init("Clock"); // Give it a name
    Serial.println("init complete");

    // Create the BLE Server
    pServer = BLEDevice::createServer();
    Serial.println("server created");
    callbacks = new MyServerCallbacks();
    pServer->setCallbacks(callbacks);
    Serial.println("created pServer");

    // Create the BLE Service
    pService = pServer->createService(SERVICE_UUID);

    Serial.println("created pService");
    BLEDescriptor client_characteristic_configuration(BLEUUID((uint16_t)0x2902));

    // Create a BLE Characteristic
    pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX,BLECharacteristic::PROPERTY_NOTIFY);

    pCharacteristic->addDescriptor(new BLE2902());
    //pCharacteristic->addDescriptor(&client_characteristic_configuration);

    BLECharacteristic *pRxC = pService->createCharacteristic(CHARACTERISTIC_UUID_RX,BLECharacteristic::PROPERTY_WRITE);

    // gatt.client_characteristic_configuration
    //pRxC->addDescriptor(new BLE2902());
    pRxC->addDescriptor(&client_characteristic_configuration);


    pRxC->setCallbacks(new MyCallbacks());

    Serial.println("start server");

    // Start the service
    pService->start();
  }

  Serial.println("start advertising");

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
  ConfigMode = true;
}

// NOTE: once ble is disabled without restarting the device i have not figured out how to get enable to work after a disable...
void disableBLE() {
  ConfigMode = false;
  if (pServer) {
    Serial.println("stop ble server");
    pServer->getAdvertising()->stop();
  }
}

int buttonPressedCount = 0;

void loop() {
  int button = digitalRead(ENABLE_CONFIG);
  int _delay = 1000;
  if (button) {
    buttonPressedCount++;
    Serial.println(buttonPressedCount);
    if (buttonPressedCount > 10) {
      buttonPressedCount = 0;
      Serial.println("configure button pressed");
      if (ConfigMode) {
        ConfigMode  = false;
      } else {
        ConfigMode  = true;
      }

      if (ConfigMode) {
        digitalWrite(LED_BLUE_CONFIG, HIGH);
        digitalWrite(LED_CONFIGURED, LOW);
        enableBLE();
      } else {
        digitalWrite(LED_BLUE_CONFIG, LOW);
        disableBLE();
      }
      delay(500); // enable the button and turn on bluetooth
    }
    _delay = 100;
  } else {
    buttonPressedCount  = 0;
  }
  if (ConfigMode && deviceConnected) {
    // send key:value pairs
    // for each key and value we support for writing to our ble device
    const char *key = BLE_SET_CONFIG_KEYS[last_sent_key];
    char txBuffer[4+1+32+1]; // 4 bytes for key name, 1 byte for :, and 32 bytes for the value and 1 byte for a null
    switch (last_sent_key) {
    case 0:
      snprintf(txBuffer, sizeof(txBuffer), "%s:%s", key, settings.ssid);
      break;
    case 1:
      snprintf(txBuffer, sizeof(txBuffer), "%s:%s", key, settings.pass);
      break;
    default:
      last_sent_key = 0;
      // overflow
      return;
      break;
    }

    last_sent_key++;
    if (last_sent_key >= BLE_KEY_CONFIG_SIZE) {
      last_sent_key = 0; // loop back around
    }

    // Let's convert the value to a char array:
    //dtostrf(txValue, 1, 2, txString); // float_val, min_width, digits_after_decimal, char_buffer

//    pCharacteristic->setValue(&txValue, 1); // To send the integer value
//    pCharacteristic->setValue("Hello!"); // Sending a test message
    pCharacteristic->setValue(txBuffer);

    pCharacteristic->notify(); // Send the value to the app!
    Serial.print("*** Sent Value: ");
    Serial.print(txBuffer);
    Serial.println(" ***");

  }
  delay(_delay);
}
