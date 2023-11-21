
//#define MY_DEBUG
#include "Secret.h"
#define MY_BAUD_RATE 115200
#define MY_RADIO_RF24
#define MY_RF24_CS_PIN 5
//#define MY_SIGNING_SOFT 
//#define MY_DEBUG_VERBOSE_SIGNING
//#define MY_SPECIAL_DEBUG
//#define MY_SIGNING_REQUEST_SIGNATURES
//#define MY_SIGNING_NODE_WHITELISTING {{.nodeId = 1,.serial = {0xD3,0x20,0x23,0xD6,0xC6,0x18,0xA4,0xFC,0x24}}} 
//#define MY_SIGNING_SOFT_RANDOMSEED_PIN 15
#define MY_GATEWAY_ESP8266
#define MY_NODE_ID 7
#define MY_GATEWAY_MAX_CLIENTS 2
#define MY_DEFAULT_TX_LED_PIN  16  // the PCB, on board LED
#include <dht.h>
#define TempSensPin 9
#define TempSensType DHT22
#define BuzzerPin 15

// Defined nodes
#define GatewayNodeId 0
#define BuzzerEnableId 1

#define DoorNodeID 3
#define DoorLockStatusId 0
#define UnlockTriggerId 1

#define IelytNodeID 4
#define IelytaTempSensorId 0
#define IelytaHumdSensorId 1
#define IelytaHumdStatusId 2

#define LoganNodeID 5
#define LoganHeatSensorId 0
#define OreoTempSensorId 1
#define OreoHumdSensorId 2
#define LoganHeatStatusId 3

// SaveState positions
const uint8_t buzzerEnablePos = 0;

#include <MySensors.h>

MyMessage LoganHeatStatusMsg(LoganHeatSensorId, V_LOCK_STATUS);
MyMessage HomeTempMessage(1, V_TEMP);
const int CheckTime = 120000;
const int HouseLowTempAlarmThreshold = 60;
int curCheck = 0;
bool HeatSyncError = false;
int DoorUpdateTimer = millis() + 5000;
DHT TempSens(TempSensPin, TempSensType);
bool buzzerEnabled = true;

void setup()
{
    static bool runonce = true;
    if (runonce)
    {
        runonce = false;
        Serial.begin(MY_BAUD_RATE, SERIAL_8N1, MY_ESP8266_SERIAL_MODE, 1);
        _begin();
    }
    Serial.println("Begin!");
    LoganHeatStatusMsg.setDestination(5);
    HomeTempMessage.setDestination(2);
    pinMode(A0, INPUT);
    TempSens.begin();
    wait(2000);
    Serial.println("Began.");
    buzzerEnabled = loadState(buzzerEnablePos);
}

void loop()
{
    // Send locally attached sensors data here
    wait(3000);
 
    float temp = TempSens.readTemperature(true);
    float humd = TempSens.readHumidity();
    if (temp > 1) {
        temp = temp * 0.95;
    }
    Serial.println("Home Node temp reported: " + String(temp) + " *F");
    Serial.println("Home Node humidity reported: " + String(humd) + " %");
    gatewayTransportSend(HomeTempMessage.set(temp, 2));

    if (temp < 64.0F) { 
        for (int i = 0; i < 3; i++) {
            tone(BuzzerPin, 2200, 500);
        }
    }

    if (millis() >= DoorUpdateTimer) {
        request(DoorLockStatusId, V_VAR1, DoorNodeID);
        DoorUpdateTimer = millis() + 5000;
    }

    if (digitalRead(A0) > 1000) {
        Serial.println("Button Pressed.");
    }
}

void receive(const MyMessage& message) {
    // We only expect one type of message from controller. But we better check anyway.
    if (message.isAck()) {
        Serial.println("This is an ack from node");
    }

    switch (message.sender)
    {
    case GatewayNodeId:
        if (message.sensor == BuzzerEnableId) {
            buzzerEnabled = message.getBool();
            Serial.print("Buzzer state chaged to ");
            Serial.println(buzzerEnabled ? "Enabled" : "Disabled");
        }
        break;
    case IelytNodeID:
        if (message.sensor == IelytaHumdSensorId) {
            float value = message.getFloat();
            Serial.println("Ielyta's humidity reported: " + String(value) + " %");
        }
        if (message.sensor == IelytaTempSensorId) {
            float value = message.getFloat();
            Serial.println("Ielyta's temp reported: " + String(value) + " *F");
        }
        if (message.sensor == IelytaHumdStatusId) {
            bool value = message.getBool();
            Serial.print("Ielyta's vaporizer powered: ");
            Serial.println(value ? "on" : "off");
        }
        break;
    case DoorNodeID:
        if (message.sensor == DoorLockStatusId) {
            bool value = message.getBool();
            Serial.print("Lock At-Home reported:");
            Serial.println(value ? "enabled" : "disabled");
        }
        if (message.sensor == UnlockTriggerId) {
            Serial.println("Door was triggered!!");
            tone(BuzzerPin, 1800, 1000);
            wait(500);
            tone(BuzzerPin, 1500, 1000);
            wait(500);
        }
        break;
    case LoganNodeID:
        if (message.sensor == OreoHumdSensorId) {
            float humd = message.getInt();
            Serial.println("Oreo's tank humidity reported: " + String(humd) + " %");
        }
        if (message.sensor == LoganHeatStatusId) {
            bool value = message.getBool();
            Serial.print("Logan's room heat reported:");
            Serial.println(value ? "enabled" : "disabled");
        }
        if (message.sensor == OreoTempSensorId) {
            float temp = message.getInt();
            Serial.println("Oreo's tank temp reported: " + String(temp) + " *F");
        }
        if (message.sensor == LoganHeatSensorId) {
            float temp = message.getInt();
            Serial.println("Logans's temp reported: " + String(temp) + " *F");
        }
        break;
    default:
        break;
    }
}
