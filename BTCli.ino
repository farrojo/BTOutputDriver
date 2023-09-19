#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "EEPROM.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"

#define EEPROM_SIZE 64

String inCommand = "";         
bool commandComplete = false;  
int addr = 0;
byte bstatus;
bool start = false;
int OUT[]={LED_BUILTIN,16,5,4,2,14,13,15};//mod ESP8266
bool started = false;

int out_len = sizeof(OUT)/sizeof(OUT[0]);

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;

#define SERVICE_UUID        "d8121f6c-571f-11ee-8c99-0242ac120002"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      BLEDevice::startAdvertising();
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }

};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
          char inChar;
    for(int i=0;i<rxValue.length();i++){inCommand +=rxValue[i];}
      inCommand.toUpperCase();
      exec(inCommand);
      commandComplete = true;
      }
      if (commandComplete) {
    inCommand = "";
    commandComplete = false;
  }
    }
    
void exec(String command){
  inCommand = command;
  if(inCommand=="AT"){
        Serial.println("OK");
        }
      else if(inCommand=="HELP"){
        String Help = F(

        "\r\n"
        "******************************************************\r\n"
        "           BT OUTPUT Driver Usage           \r\n"
        "******************************************************\r\n"
        "   Box<Box Number>=<Status> : 0 or 1                  \r\n"
        "   Box<Box Number>          : Toggle Status           \r\n"
        "   all=0                    : Clear All               \r\n"
        "   all=1                    : Set All                 \r\n"
        "   stat                     : Check all outputs state \r\n"
        "******************************************************"
                     );
        Serial.println(Help);
        }
      else if(inCommand.substring(0,3)=="ALL"){
        String errorAll = F(

            "\r\n"
            "Error using ALL command!, usage:\r\n"
            "ALL=1 to set all outputs\r\n"
            "ALL=0 to unset all outputs\r\n"
                     );
        if(inCommand.substring(3,4)=="="){
          if(inCommand.substring(4,5)=="1"){
          for(int i=0;i<8;i++){digitalWrite(OUT[i],HIGH);EEPROM.write(i, 1);EEPROM.commit();}    
            Serial.println("OK");

          }
          else if(inCommand.substring(4,5)=="0"){

          for(int i=0;i<8;i++){digitalWrite(OUT[i],LOW);EEPROM.write(i, 0);EEPROM.commit();}       
            Serial.println("OK");
          }
          else{Serial.println(errorAll);}
          }
        
        else{Serial.println(errorAll);}
        }
      else if(inCommand=="STAT"){
        
        for(int i=0;i<out_len;i++)
        {
          int adj=digitalRead(OUT[i]);
          int outstat;
          Serial.print("Out");Serial.print(i);Serial.print("=");Serial.println(adj);
        }
        Serial.println("OK");    
  
        }
        else if(inCommand=="ETAT"){
        
        for(int i=0;i<out_len;i++)
        {
          Serial.print("EEPROM");Serial.print(i);Serial.print("=");Serial.println(EEPROM.read(i));
        }
        Serial.println("OK");      
        }
      
      //***********************************************************************************************************
      else if(inCommand.substring(0,3)=="OUT"){
        
        
        if (inCommand.length()==3){Serial.println("No specified channel!");}        
        
        else if(inCommand.indexOf("=")!=-1){
          int chan = inCommand.substring(3,inCommand.indexOf("=")).toInt();
          int val = inCommand.substring(inCommand.indexOf("=")+1).toInt();
          
          if (inCommand.indexOf("=")==3){Serial.println("No specified channel,2!");}
          else if(inCommand.indexOf("=")>3&&inCommand.indexOf("=")!=inCommand.length()-1){
            if(val<0||val>1){Serial.println("Channel value must be 0 or 1!");}
            else{
              if (val==0){digitalWrite(OUT[chan],LOW);EEPROM.write(chan, 0);EEPROM.commit();}
              else if(val==1){digitalWrite(OUT[chan],HIGH);EEPROM.write(chan, 1);EEPROM.commit();}
            }}
          else if(inCommand.indexOf("=")>3&&inCommand.indexOf("=")==inCommand.length()-1){Serial.println("No value specified for channel!");}
          }
        
        else if(inCommand.indexOf("=")==-1){
          int chan = inCommand.substring(3,inCommand.length()).toInt();
          int outstat=digitalRead(OUT[chan]);          
          if(outstat==1){digitalWrite(OUT[chan],LOW);EEPROM.write(chan, 0);EEPROM.commit();}
          if(outstat==0){digitalWrite(OUT[chan],HIGH);EEPROM.write(chan, 1);EEPROM.commit();}          
          }
        else{Serial.println("No specified channel to set");}
        }
    //***********************************************************************************************************
        
      else if(inCommand=="HELP"){
        Serial.println(inCommand + " OK");
        }

        else if(inCommand=="MAC?"){
            const uint8_t* point = esp_bt_dev_get_address(); 
            for (int i = 0; i < 6; i++) {           
              char str[3];           
              sprintf(str, "%02X", (int)point[i]);
              Serial.print(str);           
              if (i < 5){
                Serial.print(":");
              }           
            }
        }

      else{Serial.println("Unknown command " + inCommand + "!");}  
  
  }

    
};

void setup() {
  Serial.begin(115200);

  if (!EEPROM.begin(EEPROM_SIZE))
  {
    Serial.println("failed to initialise EEPROM"); delay(1000000);
  }
  for(int i=0;i<out_len;i++){pinMode(OUT[i],OUTPUT);}
  // Create the BLE Device
  BLEDevice::init("BacoBox BLE - 2 - V1");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

  pCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
                       CHARACTERISTIC_UUID_RX,
                      BLECharacteristic::PROPERTY_WRITE
                    );

  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {

  while(!started){for(int i=0;i<out_len;i++){
    int prevstat = EEPROM.read(i);
    if(prevstat==0){digitalWrite(OUT[i],LOW);}
    else if(prevstat==1){digitalWrite(OUT[i],HIGH);}
    }
    started=true;
  }
    // notify changed value
    if (deviceConnected) {
        pCharacteristic->setValue((uint8_t*)&value, 4);
        pCharacteristic->notify();
        value++;
        delay(10); // Delay to deal with BT congestion in High traffic
    }
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // waiting time to set all up
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        // additional stuff to do in connecting 
        oldDeviceConnected = deviceConnected;
    }
}
