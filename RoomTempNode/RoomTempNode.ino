/*
 Name:		RoomTempNode.ino
 Created:	1/11/2022 5:54:28 PM
 Author:	Crowbar
*/

#define MY_BAUD_RATE 9600
#define MY_TRANSPORT_WAIT_READY_MS 6000
#define MY_RADIO_RF24
#define MY_NODE_ID 4
//#define MY_DEBUG
//#define MY_DEBUG_VERBOSE_SIGNING
//#define MY_SIGNING_SOFT
//#define MY_SIGNING_NODE_WHITELISTING {{.nodeId = 0,.serial = {0xAC,0x35,0xA4,0x00,0xD8,0x40,0x16,0x00,0xAA}}}
//#define MY_SIGNING_REQUEST_SIGNATURES
//#define MY_SIGNING_SOFT_RANDOMSEED_PIN 4
#include <MySensors.h>

//DHT Sensors setup
#include <DHT.h>
#define RoomSensPin 7
#define SensType DHT22

#define RoomTempId 0
#define RoomHumdId 1
#define VapeStateId 2
#define VapeAlarmId 3
#define MasterHumdEnableId 4
#define RiseAboveStateId 5
#define FallBelowStateId 6
DHT RoomSens(RoomSensPin, SensType);

//Misc defines
#define PwrLed 8
#define VapePwr 6

// SaveState Positions
const uint8_t HumidLevelPos = 0;
const uint8_t MasterHundEnablePos = 1;

// Initialize messages
MyMessage RoomTempMsg(RoomTempId, V_TEMP);
MyMessage RoomHumdMsg(RoomHumdId, V_HUM);
MyMessage RoomVapeStatus(VapeStateId, V_LOCK_STATUS);
MyMessage VapeAlarm(VapeAlarmId, V_TRIPPED);
MyMessage HumdEnableMsg(MasterHumdEnableId, V_LOCK_STATUS);

//Temp defaults
uint8_t TargetHumd = 50;
const int lowHysteresis = 3;
const int highHysteresis = 1;
const float tempCorrectionFactor = 0.95;
const float humdCorrectionFactor = 0.95;
bool VapePowered = false;
long VapePoweredTime = 0;
bool MasterVapeEnabled = false;

void setup()
{
    Serial.begin(9600);
    RoomSens.begin();
    pinMode(PwrLed, OUTPUT);
    pinMode(VapePwr, OUTPUT);
    digitalWrite(PwrLed, HIGH);
    wait(500);
    /*
    uint8_t humdVal = loadState(HumidLevelPos);
    MasterVapeEnabled = loadState(MasterHundEnablePos);
    if (humdVal == 0U) {
        saveState(HumidLevelPos, TargetHumd);
    }
    if (humdVal > 60U) {
        saveState(HumidLevelPos, TargetHumd);
    }
    TargetHumd = humdVal;
    Serial.println(humdVal);
    Serial.println(TargetHumd);
    */
    Serial.begin(9600);
}

void presentation()
{
    // Send the sketch version information to the gateway and Controller
    sendSketchInfo("Ielyta's Room", "1.0");

    // Register all sensors to gw (they will be created as child devices)
    present(RoomTempId, S_TEMP);
    present(RoomHumdId, S_HUM);
    present(VapeStateId, S_LOCK);
    present(VapeAlarmId, S_MOTION);
}

void loop()
{
    wait(500);
    float RoomHumd = RoomSens.readHumidity();
    wait(500);
    float RoomTemp = RoomSens.readTemperature(true);
    if (RoomTemp > 1) {
        RoomTemp = RoomTemp * tempCorrectionFactor;
    }
    Serial.println("temp reported: " + String(RoomTemp) + " *F");
    Serial.println("humidity reported: " + String(RoomHumd) + " %");
    float lowerTarget = static_cast<float>(TargetHumd - static_cast<uint8_t>(lowHysteresis));
    float upperTarget = static_cast<float>(static_cast<uint8_t>(highHysteresis) + TargetHumd);
    if (RoomHumd > 0.0F && RoomHumd >= upperTarget) {
        VapePowered = false;
        Serial.println("Vape powered off");
        Serial.println(RoomHumd);
        Serial.println(upperTarget);
    }
    if (RoomHumd > 0.0F && (int)RoomHumd < lowerTarget) {
        VapePowered = true;
        VapePoweredTime = millis();
        Serial.println("Vape powered on");
        Serial.println(RoomHumd);
        Serial.println(lowerTarget);
    }
    if (VapePowered == true && RoomHumd < ((TargetHumd - lowHysteresis) - 2) || (VapePoweredTime + 12000) > millis()) {
        send(VapeAlarm.set(true));
        wait(500);
    }

    send(RoomTempMsg.set(RoomTemp, 2));
    wait(500);
    send(RoomHumdMsg.set(RoomHumd, 2));
    wait(500);
    send(RoomVapeStatus.set(VapePowered));
    wait(500);
    send(HumdEnableMsg.set(MasterVapeEnabled));
    wait(500);
    digitalWrite(VapePwr, VapePowered);
}

void receive(const MyMessage& message) {

    if (message.isAck()) {
        Serial.println("This is an ack from node");
    }

    if (message.type == V_TEMP && message.sensor == RoomHumdId) {
        saveState(HumidLevelPos, message.getInt());
    }

    if (message.type == V_LOCK_STATUS && message.sensor == MasterHumdEnableId) {
        saveState(MasterHundEnablePos, message.getInt());
        MasterVapeEnabled = message.getInt();
    }
}
