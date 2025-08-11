#include "Secret.h"
#include <ReceiveOnlySoftwareSerial.h>
//#define MY_DEBUG
#define MY_TRANSPORT_WAIT_READY_MS 3000
#define MY_RADIO_RF24
#define MY_NODE_ID 3
#define MY_PARENT_NODE_ID 0
#include <MySensors.h>
#define LockChildId 0

#define LockStatusMsgId 0
#define LockTripMsgId 1
#define RemoteLockTriggerId 2

ReceiveOnlySoftwareSerial ssrfid(3);
const long WAIT_TIME = 3000;
const long UNLOCK_TIME = 10000;
const int LockOutput = A5;
const int SpkrOutput = A4;
const int AtHomeInd = 4;
const int OutsideEntryBtn = 7;
const int LEDG = 5;
const int LEDB = 6;
const int InsideEntryBtn = 8;
const int BUFFER_SIZE = 14;
int buffer_index = 0;
bool AtHome = false;
char buffer[BUFFER_SIZE];
long LockTimer = 0;
long IdleTime = 0;
MyMessage LockStatus(LockStatusMsgId, V_LOCK_STATUS);
MyMessage UnlockTriggerMsg(LockTripMsgId, V_TRIPPED);

void setup()
{
    pinMode(LockOutput, OUTPUT);
    pinMode(SpkrOutput, OUTPUT);
    pinMode(LEDG, OUTPUT);
    pinMode(LEDB, OUTPUT);
    pinMode(AtHomeInd, OUTPUT);
    pinMode(InsideEntryBtn, INPUT_PULLUP);
    pinMode(OutsideEntryBtn, INPUT_PULLUP);
    ssrfid.begin(9600);
    Serial.println("Serial init okay!");
}

void loop()
{
    long startTime = millis();
    if (digitalRead(InsideEntryBtn) == LOW)
    {
        delay(10);
        Serial.println("Exit pressed");
        while (digitalRead(InsideEntryBtn) == LOW)
        {
            if (millis() >= startTime + 2000)
            {
                ChangeLockStatus();
            }
        }
        if (millis() < startTime + 2000)
        {
            Unlock();
        }
    }

    if (digitalRead(OutsideEntryBtn) == false)
    {
        if (AtHome == true) {
            Unlock();
        }
    }
    digitalWrite(AtHomeInd, AtHome);

    if (ssrfid.available() > 0)
    {
        ProcessRfidTag();
    }

    if (millis() > IdleTime + 1000)
    {
        digitalWrite(LEDG, HIGH);
        delay(5);
        digitalWrite(LEDG, LOW);
        IdleTime = millis();
    }
}

void presentation()
{
    // Send the sketch version information to the gateway and Controller
    sendSketchInfo("Secure Lock", "1.0");
    present(LockChildId, S_LOCK);
}

void ChangeLockStatus() {
    Serial.println("Changing modes now!");
    AtHome = !AtHome;
    send(LockStatus.set(AtHome));
    digitalWrite(AtHomeInd, LOW);
    bool flash = false;
    for (int f = 0; f < 10; f++)
    {
        flash = !flash;
        if (f >= 7)
        {
            if (flash == true && AtHome == false)
            {
                flash = false;
            }
        }
        digitalWrite(AtHomeInd, flash);
        delay(500);
        digitalWrite(AtHomeInd, LOW);
    }
}

void Unlock()
{
    send(UnlockTriggerMsg.set(true));
    LockTimer = millis() + UNLOCK_TIME;
    digitalWrite(LockOutput, HIGH);
    digitalWrite(LEDG, LOW);
    digitalWrite(LEDB, LOW);
    while (millis() <= LockTimer)
    {
        digitalWrite(LEDB, HIGH);
        delay(50);
        digitalWrite(LEDB, LOW);
        delay(100);
    }
    digitalWrite(LockOutput, LOW);
    digitalWrite(LEDB, LOW);
    while (ssrfid.available() > 0)
    {
        ssrfid.read();
    }
    LockTimer = 0;
}

void ProcessRfidTag() {
    bool call_extract_tag = false;
    char startChar = 2;
    char endChar = 3;
    char ssvalue = ssrfid.read();
    if (ssvalue == -1)
    {
        return;
    }
    if (ssvalue == startChar)
    {
        buffer_index = 0;
    }
    else if (ssvalue == endChar)
    {
        call_extract_tag = true;
    }
    if (buffer_index >= BUFFER_SIZE)
    {
        return;
    }

    buffer[buffer_index++] = ssvalue;
    if (call_extract_tag == true)
    {
        char sendme[12];
        for (int i = 1; i < 13; i++)
        {
            sendme[i - 1] = buffer[i];
            Serial.write(buffer[i]);
        }
        if (CheckRfidKey(sendme))
        {
            Unlock();
        }
        else
        {
            bool flash = false;
            for (int i = 0; i < 6; i++)
            {
                flash = !flash;
                digitalWrite(LEDB, flash);
                delay(25);
            }
            digitalWrite(LEDB, LOW);
        }
    }
}

bool CheckRfidKey(char keyToTest[]) {
    bool ChkFlag = false;
    for (int x = 0; x < 8; x++) {
        char Key[13];
        char compKey[13];
        strcpy(Key, MasterKeys[x]);
        strcpy(compKey, keyToTest);
        if (strcmp(Key, compKey) == 0 && LockTimer == 0)
        {
            return true;
        }
    }
    return false;
}
void receive(const MyMessage& message)
{
    Serial.println("Got a message!");
    if (message.isEcho()) {
        return;
    }
    if (message.getSensor() == LockTripMsgId) {
        // Change relay state
        AtHome = !AtHome;
        Serial.print("Incoming change for lock:");
        Serial.print(message.getSensor());
        Serial.print(", New status: ");
        Serial.println(AtHome ? "enabled" : "disabled");
    }

    if (message.sensor == LockStatusMsgId) {
        send(LockStatus.set(AtHome));
    }

    if (message.sensor == RemoteLockTriggerId) {
        Unlock();
    }
}
