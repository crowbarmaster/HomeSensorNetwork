#define MY_BAUD_RATE 9600
#define MY_TRANSPORT_WAIT_READY_MS 6000
//#define MY_DEBUG
//#define MY_DEBUG_VERBOSE_SIGNING
#define MY_RADIO_RF24
//#define MY_SIGNING_SOFT
#define MY_NODE_ID 5
//#define MY_SIGNING_NODE_WHITELISTING {{.nodeId = 0,.serial = {0xAC,0x35,0xA4,0x00,0xD8,0x40,0x16,0x00,0xAA}}}
//#define MY_SIGNING_REQUEST_SIGNATURES
//#define MY_SIGNING_SOFT_RANDOMSEED_PIN 4
#include <MySensors.h>

//DHT Sensors setup
#include <DHT.h>
#define TortSensPin A1
#define RoomSensPin A2
#define SensType DHT22
#define RoomTempId 0
#define TortTempId 1
#define TortHumdId 2
#define HeatStatusId 3
#define MasterHeatEnableId 4
#define MasterHumdEnableId 5
#define RiseAboveStateId 6
#define FallBelowStateId 7
#define HouseTempId 8
#define HeatFaultId 9
DHT TortSens(TortSensPin, SensType);
DHT RoomSens(RoomSensPin, SensType);

//IR Sensor setup
#include <IRremote.h>
#define IR_SEND_PIN A0

//Misc defines
#define PwrLed 8
#define TestHeatPin A3

// Initialize messages
MyMessage TortTempMsg(TortTempId, V_TEMP);
MyMessage TortHumdMsg(TortHumdId, V_HUM);
MyMessage RoomTempMsg(RoomTempId, V_TEMP);
MyMessage RoomStatus(HeatStatusId, V_LOCK_STATUS);
MyMessage HeatFaultMsg(HeatFaultId, V_TEMP);

//Temp defaults
const int TortTargetTemp = 85; 
uint8_t TortTargetHumd = 46;
uint8_t RoomTargetTemp = 75;
const int RiseAboveTarget = 1;
const int FallBelowTarget = 1;
long PowerOnTime;
bool HeatEnabled = false;
bool MasterHeatEnable = true;
bool MasterHumdEnable = true;
bool HeatFaulted = false;
float FaultedAtTemp = 0;
unsigned long NextFaultCheck = 0;
int HouseTemp = 0;

// SaveState positions
const uint8_t RoomTempPos = 0;
const uint8_t TortHumdPos = 1;
const uint8_t RiseAbovePos = 2;
const uint8_t FallBelowPos = 3;
const uint8_t MasterHeatEnablePos = 4;
const uint8_t MasterHumdEnablePos = 5;

void setup()
{
    Serial.begin(9600);
    TortSens.begin();
    RoomSens.begin();
    PowerOnTime = millis();
    pinMode(PwrLed, OUTPUT);
    IrSender.begin(IR_SEND_PIN, ENABLE_LED_FEEDBACK);
    wait(500);
    /*
    uint8_t humdVal = loadState(TortHumdPos);
    uint8_t tempVal = loadState(RoomTempPos);
    uint8_t raVal = loadState(RiseAbovePos);
    uint8_t fbVal = loadState(FallBelowPos);
    MasterHeatEnable = loadState(MasterHeatEnablePos);
    MasterHumdEnable = loadState(MasterHumdEnablePos);
    if (humdVal == 0) {
        saveState(TortHumdPos, TortTargetHumd);
    }
    if (tempVal == 0) {
        saveState(RoomTempPos, RoomTargetTemp);
    }
    if (raVal == 0) {
        saveState(RiseAbovePos, 2);
    }
    if (fbVal == 0) {
        saveState(FallBelowPos, 2);
    }
    TortTargetHumd = humdVal;
    RoomTargetTemp = tempVal;
    */
}

void presentation()
{
    // Send the sketch version information to the gateway and Controller
    sendSketchInfo("Logan's Room", "1.0");

    // Register all sensors to gw (they will be created as child devices)
    present(TortTempId, S_TEMP);
    present(TortHumdId, S_HUM);
    present(RoomTempId, S_TEMP);
    present(HeatStatusId, S_LOCK);
}

void loop()
{
    request(HouseTempId, V_TEMP);
    wait(10000);
    // Read values
    float TortTemp = TortSens.readTemperature(true);
    wait(500);
    float TortHumd = TortSens.readHumidity();
    wait(500);
    float RoomTemp = RoomSens.readTemperature(true);
    wait(500);

    send(TortTempMsg.set(TortTemp, 2));
    wait(500);
    send(TortHumdMsg.set(TortHumd, 2));
    wait(500);
    send(RoomTempMsg.set(RoomTemp, 2));
    wait(500);
    send(RoomStatus.set(HeatEnabled));
    wait(500);
    Serial.print("Room temp: ");
    Serial.print(RoomTemp);
    Serial.print(" Heat ");
    if (HeatEnabled) {
        Serial.println("On.");
    }
    else {
        Serial.println("Off.");
    }
    Serial.print("Oreo's Humidity: ");
    Serial.println(TortHumd);
    Serial.print("Oreo's Temp: ");
    Serial.println(TortTemp);
    if (RoomTemp <= (RoomTargetTemp - FallBelowTarget) && !HeatEnabled && MasterHeatEnable) {
        PowerUpHeat();
    }
    if (RoomTemp >= (RoomTargetTemp + RiseAboveTarget) && HeatEnabled) {
        ShutDownHeat();
    }
    if ((RoomTemp > (RoomTargetTemp + (RiseAboveTarget)) && !HeatEnabled && !HeatFaulted) || (RoomTemp < (RoomTargetTemp - (RiseAboveTarget)) && HeatEnabled && !HeatFaulted)) {
        HeatFaulted = true;
        FaultedAtTemp = RoomTemp;
    }
    if (TortHumd <= (TortTargetHumd - FallBelowTarget) && MasterHumdEnable)
    {
        Serial.println("Relay HIGH!");
        digitalWrite(PwrLed, HIGH);
    }
    if (TortHumd >= (TortTargetHumd + RiseAboveTarget))
    {
        digitalWrite(PwrLed, LOW);
    }
    if (HeatFaulted) {
        Serial.println("Heat has faulted!");
        send(HeatFaultMsg.set(FaultedAtTemp, 2));
        wait(500);
        if (millis() > NextFaultCheck) {
            if (FaultedAtTemp > RoomTargetTemp) {
                if (RoomTemp < FaultedAtTemp && RoomTemp <= RoomTargetTemp + 1) {
                    HeatFaulted = false;
                } else if (RoomTemp < FaultedAtTemp) {
                    FaultedAtTemp = RoomTemp;
                    NextFaultCheck = millis() + 360000L;
                } else {
                    NextFaultCheck = millis() + 360000L;
                    Serial.println("Performing faulted shutdown.");
                    ShutDownHeat();
                }
            }
            if (FaultedAtTemp < RoomTargetTemp) {
                if (RoomTemp > FaultedAtTemp && RoomTemp >= RoomTargetTemp) {
                    HeatFaulted = false;
                } else if (RoomTemp > FaultedAtTemp) {
                    FaultedAtTemp = RoomTemp;
                    NextFaultCheck = millis() + 360000L;
                } else {
                    NextFaultCheck = millis() + 360000L;
                    Serial.println("Performing faulted restart.");
                    PowerUpHeat();
                }
            }
        }
    }
}

void PowerUpHeat() {
    HeatEnabled = true;
    Serial.println("Turning on heat.");
    IrSender.sendNEC(0xFF00, 0xC4, 0); // Sony TV power code
    wait(2000);
    IrSender.sendNEC(0xFF00, 0xC8, 0); // Sony TV power code
    wait(2000);
}

void ShutDownHeat() {
    HeatEnabled = false;
    Serial.println("Turning off heat.");
    wait(2000);
    IrSender.sendNEC(0xFF00, 0xC4, 0); // Sony TV power code
    wait(2000);
}

void TestHeat() {
    PowerUpHeat();
    wait(20000);
    ShutDownHeat();
    wait(2000);
}

void receive(const MyMessage& message) {
    // We only expect one type of message from controller. But we better check anyway.
    if (message.isAck()) {
        Serial.println("This is an ack from node");
    }

    if (message.type == V_LOCK_STATUS) {
        Serial.print("Switching heat to ");
        if (!HeatEnabled && MasterHeatEnable) {
            PowerUpHeat();
        }
        else {
            ShutDownHeat();
        }
        wait(1000);
    }
    if (message.type == V_HUM && message.sensor == TortHumdId) {
        saveState(TortHumdPos, message.getInt());
    }
    if (message.type == V_TEMP && message.sensor == RoomTempId) {
        saveState(RoomTempPos, message.getInt());
    }
    if (message.type == V_TEMP && message.sensor == HouseTempId) {
        HouseTemp = message.getInt();
    }
}
