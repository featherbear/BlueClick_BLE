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

#define NOTE_A 0x3c
#define PIN_A 32

#define NOTE_B 0x3d
#define PIN_B 33

/////////////////////////////////////////////////////////////////

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define SERVICE_UUID        "03b80e5a-ede8-4b33-a751-6ce34ec4c700"
#define CHARACTERISTIC_UUID "7772e5db-3868-4112-a1a9-f2669d106bf3"

void IRAM_ATTR sendNote(uint8_t note);
void IRAM_ATTR sendNote(uint8_t note, uint8_t velocity);

BLECharacteristic *pCharacteristic;

volatile bool deviceConnected = false;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      // Serial.println("Connected to a client");
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      // Serial.println("Disconnected from a client");
      deviceConnected = false;
    }
};

void setupBLE() {
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
  // Serial.println("Ready to receive BLE connections");
}


void IRAM_ATTR SEND_A() {
  sendNote(NOTE_A);
}

void IRAM_ATTR SEND_B() {
  sendNote(NOTE_B);
}

/*
   Send note at default velocity 127
*/
void IRAM_ATTR sendNote(uint8_t note) {
  sendNote(note, 127);
}

/*
    Send note at a given velocity
*/
void IRAM_ATTR sendNote(uint8_t note, uint8_t velocity) {
  // Debounce
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time < 150) return;
  last_interrupt_time = interrupt_time;

  if (!deviceConnected) return;
  // Serial.println("Sending note");

  uint8_t midiPacket[] = {
    0x80,    // header
    0x80,    // timestamp (not implemented)
    0x00,    // status
    note,    // note
    velocity // velocity
  };

  // note down
  midiPacket[2] = 0x90;                     // note down, channel 0
  pCharacteristic->setValue(midiPacket, 5); // packet, length (bytes)
  pCharacteristic->notify();                // emit
  delay(100);

  // note up
  midiPacket[2] = 0x80;                     // note up, channel 0
  pCharacteristic->setValue(midiPacket, 5); // packet, length (bytes)
  pCharacteristic->notify();                // emit

}


void setup() {
  Serial.begin(115200);

  // Slow down the clock speed, to save power
  setCpuFrequencyMhz(80);

  setupBLE();
  
  pinMode (PIN_A, INPUT_PULLUP);
  pinMode (PIN_B, INPUT_PULLUP);
  attachInterrupt(PIN_A, SEND_A, FALLING);
  attachInterrupt(PIN_B, SEND_B, FALLING);
}

void loop() {}
