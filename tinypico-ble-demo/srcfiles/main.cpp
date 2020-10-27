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

bool deviceConnected = false;
float txValue = 0;
bool ConfigMode = false; //  when pressed we'll enter configuration mode

// Initialise the TinyPICO library
TinyPICO tp = TinyPICO();
BLEService *pService;
BLEServer *pServer;
BLECharacteristic *pCharacteristic;
EEPROMSettings settings;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

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
        Serial.println( ("Connect to WiFi with: " + key + " and " + value).c_str() );
        memcpy(settings.ssid, value.c_str(), 32);
        settings.configured = true; 
        settings.save();
      } else if (key.length() > 0 && key == "pass" && value.length() > 0) {
        Serial.println( ("Connect to WiFi with: " + key + " and " + value).c_str() );
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

void setup() {
  Serial.begin(115200);
  pinMode(LED_CONFIGURED, OUTPUT);
  pinMode(LED_BLUE_CONFIG, OUTPUT);
  pinMode(ENABLE_CONFIG, INPUT);

	EEPROM.begin(EEPROM_SIZE);

  tp.DotStar_SetPower( false );

  settings.load();

  if (settings.configured && strlen(settings.ssid) > 2 && strlen(settings.pass) > 2 && isalnum(settings.ssid[0])) {
    Serial.println("wifi is configured");
    Serial.println(settings.ssid);
    Serial.println(settings.pass);
    initWiFi(settings.ssid, settings.pass);
  }
}

void enableBLE() {
  // Create the BLE Device
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

    // Create a BLE Characteristic
    pCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_TX,
                        BLECharacteristic::PROPERTY_NOTIFY
                      );

    pCharacteristic->addDescriptor(new BLE2902());

    BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                           CHARACTERISTIC_UUID_RX,
                                           BLECharacteristic::PROPERTY_WRITE
                                         );

    pCharacteristic->setCallbacks(new MyCallbacks());

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
  //  pService->stop();
  /*  delete pCharacteristic;
    delete pService;
    delete pServer;
    delete callbacks;
    callbacks = NULL;
    pCharacteristic = NULL;
    pServer = NULL;
    pService = NULL;
    BLEDevice::deinit(true);
    */
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
    // Fabricate some arbitrary junk for now...
    txValue = 3.456; // This could be an actual sensor reading!

    // Let's convert the value to a char array:
    char txString[8]; // make sure this is big enuffz
    dtostrf(txValue, 1, 2, txString); // float_val, min_width, digits_after_decimal, char_buffer

//    pCharacteristic->setValue(&txValue, 1); // To send the integer value
//    pCharacteristic->setValue("Hello!"); // Sending a test message
    pCharacteristic->setValue(txString);

    pCharacteristic->notify(); // Send the value to the app!
    Serial.print("*** Sent Value: ");
    Serial.print(txString);
    Serial.println(" ***");

  }
  delay(_delay);
}
