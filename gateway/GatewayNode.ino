
//#define MY_DEBUG
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
#define MY_WIFI_SSID "ATL-2G"
#define MY_WIFI_PASSWORD "Logan182"
#define MY_HOSTNAME "ESP8266_GW"
#define MY_IP_ADDRESS 10,0,10,254
#define MY_PORT 5003
#define MY_GATEWAY_MAX_CLIENTS 2
#define MY_DEFAULT_TX_LED_PIN  16  // the PCB, on board LED
#include <dht.h>
#define TempSensPin 0
#define TempSensType DHT22

// Defined nodes
#define DoorNodeID 3
#define DoorLockStatusId 0

#define IelytNodeID 4
#define IelytaTempSensorId 0
#define IelytaHumdSensorId 1
#define IelytaHumdStatusId 2

#define LoganNodeID 5
#define LoganHeatSensorId 0
#define OreoTempSensorId 1
#define OreoHumdSensorId 2
#define LoganHeatStatusId 3

#include <MySensors.h>

MyMessage LoganHeatStatusMsg(LoganHeatSensorId, V_LOCK_STATUS);
const int CheckTime = 120000;
const int HouseLowTempAlarmThreshold = 60;
int curCheck = 0;
bool HeatSyncError = false;
int DoorUpdateTimer = millis() + 5000;
DHT TempSens(TempSensPin, TempSensType);

void setup()
{
    static bool runonce = true;
    if (runonce)
    {
        runonce = false;
        Serial.begin(MY_BAUD_RATE, SERIAL_8N1, MY_ESP8266_SERIAL_MODE, 1);
        _begin();
    }
    LoganHeatStatusMsg.setDestination(5);
    pinMode(A0, INPUT);
    TempSens.begin();
}

void loop()
{
    // Send locally attached sensors data here
    wait(3000);
   // tone(15, 2000.0, 1000);

    float temp = TempSens.readTemperature(true);
    if (temp != NULL && temp > 1) {
        temp = temp * 0.95;
    }
    Serial.println("Home Node temp reported: " + String(temp) + " *F");

    if (millis() >= DoorUpdateTimer) {
        request(DoorLockStatusId, V_VAR1, DoorNodeID);
        DoorUpdateTimer = millis() + 5000;
    }
}

void receive(const MyMessage& message) {
    // We only expect one type of message from controller. But we better check anyway.
    if (message.isAck()) {
        Serial.println("This is an ack from node");
    }

    switch (message.sender)
    {
    case IelytNodeID:
        if (message.sensor == IelytaHumdSensorId) {
            float value = message.getFloat();
            Serial.println("Ielyta's humidity reported: " + String(value) + " *F");
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
        break;
    case LoganNodeID:
        if (message.sensor == OreoHumdSensorId) {
            float humd = message.getInt();
            Serial.println("Oreo's tank humidity reported: " + String(humd) + " %");
        }
        if (message.sensor == LoganHeatStatusId) {
            bool value = message.getBool();
            Serial.print("Room heat reported:");
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
