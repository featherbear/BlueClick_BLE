/*
   BlueClick BLE
   - Send MIDI events over Bluetooth Low Energy
   - Used for scrolling and turning pages for OnSong (iOS)

   Background:
   The iPad 2 doesn't support BLE, so the idea was to emulate a Bluetooth Classic HID device to send keystrokes.
   Too bad I couldn't find any good "copy-paste-find-replace"-able code.
   So I ended up buying an iPad Air 2 which has BLE support.

   Forked: https://github.com/neilbags/arduino-esp32-BLE-MIDI
*/

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define SERVICE_UUID        "03b80e5a-ede8-4b33-a751-6ce34ec4c700"
#define CHARACTERISTIC_UUID "7772e5db-3868-4112-a1a9-f2669d106bf3"

#define NOTE_A 0x3c
#define NOTE_B 0x3d

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

uint8_t midiPacket[] = {
  0x80,  // header
  0x80,  // timestamp, not implemented
  0x00,  // status
  0x3c,  // 0x3c == 60 == middle c
  0x00   // velocity
};

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

void setupBLE() {
  Serial.begin(115200);

  BLEDevice::init("BlueClick BLE");

  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(BLEUUID(SERVICE_UUID));

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      BLEUUID(CHARACTERISTIC_UUID),
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_WRITE_NR
                    );

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  pCharacteristic->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->addServiceUUID(pService->getUUID());
  pAdvertising->start();
}

void setup() {
  Serial.begin(115200);
  setupBLE();
}

/*
   Send note at default velocity 127
*/
void sendNote(uint8_t note) {
  sendNote(note, 127);
}

/*
    Send note at a given velocity
*/
void sendNote(uint8_t note, uint8_t velocity) {
  midiPacket[3] = note; // note
  midiPacket[4] = velocity;  // velocity

  // note down
  midiPacket[2] = 0x90; // note down, channel 0
  pCharacteristic->setValue(midiPacket, 5); // packet, length (bytes)
  pCharacteristic->notify(); // emit
  delay(100);

  // note up
  midiPacket[2] = 0x80; // note up, channel 0
  pCharacteristic->setValue(midiPacket, 5); // packet, length (bytes)
  pCharacteristic->notify(); // emit
  delay(100);
}

bool state = true;
void loop() {
  // Pin interrupts
  if (deviceConnected) {
    if (state) {
      sendNote(NOTE_A);
      state = false;
    } else {
      sendNote(NOTE_B);
      state = true;
    }
  }

}
