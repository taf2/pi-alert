#include <TinyPICO.h>
/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini

   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
   Has a characteristic of: 6E400002-B5A3-F393-E0A9-E50E24DCCA9E - used for receiving data with "WRITE"
   Has a characteristic of: 6E400003-B5A3-F393-E0A9-E50E24DCCA9E - used to send data with  "NOTIFY"

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.

   In this example rxValue is the data received (only accessible inside that function).
   And txValue is the data to be sent, in this example just a byte incremented every second.
*/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define LED_CONFIGURED 25
#define LED_BLUE_CONFIG 26
#define ENABLE_CONFIG 33

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
float txValue = 0;
bool ConfigMode = false; //  when pressed we'll enter configuration mode

// Initialise the TinyPICO library
TinyPICO tp = TinyPICO();
BLEService *pService;
BLEServer *pServer;
std::string rxValue; // Could also make this a global var to access it in loop()

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
      rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        Serial.println("*********");
        Serial.print("Received Value: ");

        for (int i = 0; i < rxValue.length(); i++) {
          Serial.print(rxValue[i]);
        }

        Serial.println();

        // Do stuff based on the command received from the app
        if (rxValue == "A" || rxValue == "B") {
          Serial.println("Received config use it now to connect ot wifi");
          //digitalWrite(LED, HIGH);
          digitalWrite(LED_BLUE_CONFIG, LOW);
          digitalWrite(LED_CONFIGURED, HIGH);
          disableBLE();
        }

        Serial.println();
        Serial.println("*********");
      }
    }
};

MyServerCallbacks *callbacks = NULL;

void setup() {
  Serial.begin(115200);
  pinMode(LED_CONFIGURED, OUTPUT);
  pinMode(LED_BLUE_CONFIG, OUTPUT);
  pinMode(ENABLE_CONFIG, INPUT);

  tp.DotStar_SetPower( false );
}

void enableBLE() {
  // Create the BLE Device
  Serial.println("enableBLE");
  BLEDevice::init("Clock"); // Give it a name
  Serial.println("init completed");

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

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
  ConfigMode = true;
}

// NOTE: once ble is disabled without restarting the device i have not figured out how to get enable to work after a disable...
void disableBLE() {
  ConfigMode = false;
  if (pServer) {
    pServer->getAdvertising()->stop();
    pService->stop();
    delete pCharacteristic;
    delete pService;
    delete pServer;
    delete callbacks;
    callbacks = NULL;
    pCharacteristic = NULL;
    pServer = NULL;
    pService = NULL;
    BLEDevice::deinit(true);
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
      delay(200); // enable the button and turn on bluetooth
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

    // You can add the rxValue checks down here instead
    // if you set "rxValue" as a global var at the top!
    // Note you will have to delete "std::string" declaration
    // of "rxValue" in the callback function.
//    if (rxValue.find("A") != -1) {
//      Serial.println("Turning ON!");
//      digitalWrite(LED, HIGH);
//    }
//    else if (rxValue.find("B") != -1) {
//      Serial.println("Turning OFF!");
//      digitalWrite(LED, LOW);
//    }
  }
  delay(_delay);
}
