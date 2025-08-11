#include <Wire.h>
#include <Seeed_Arduino_NFC.h>
#include <PN532/PN532_I2C/PN532_I2C.h>
#include <PN532/PN532/PN532.h>
#include "Secret.h"
//#define MY_DEBUG
//#define MY_SPECIAL_DEBUG
#define MY_BAUD_RATE 115200
#define MY_SPLASH_SCREEN_DISABLED
//#define MY_DEBUG_VERBOSE_SIGNING
//#define MY_SIGNING_SOFT
//#define MY_SIGNING_REQUEST_SIGNATURES
#define MY_SIGNING_SOFT_RANDOMSEED_PIN 11
#define MY_TRANSPORT_WAIT_READY_MS 8000
#define MY_RADIO_RF24
#define MY_NODE_ID 3
#define MY_PARENT_NODE_ID 0
#define LockChildId 0

#define LockStatusMsgId 0
#define LockTripMsgId 1
#define RemoteLockTriggerId 2
#define DoorAjarStateId 3
#define DoorAlarmEnableId 4
#define IrqPin 3
#include <MySensors.h>

PN532_I2C _pn532_i2c(Wire);
PN532 nfc(_pn532_i2c);
const long WAIT_TIME = 3000;
const long UNLOCK_TIME = 10000;
const long DoorAlarmTimeout = 30000;
const int LockOutput = A0;
const int SpkrOutput = A1;
const int AtHomeInd = A2;
const int OutsideEntryBtn = A3;
const int OutsideReadInd = 4;
const int OutsideSpeaker = 5;
const int InsideEntryBtn = 6;
const int DoorAjarSensor = 7;
const int BUFFER_SIZE = 14;
int buffer_index = 0;
bool printDebug = false;
bool AtHome = false;
bool CardPresent = false;
bool PlayedFirstTone = false;
bool DoorAlarmEnabled = true;
char buffer[BUFFER_SIZE];
long LockTimer = 0;
long IdleTime = 0;
long DoorToneTime = 0;
long DoorAjarTime = 0;
bool DoorOpened = false;
MyMessage LockStatus(LockStatusMsgId, V_LOCK_STATUS);
MyMessage DoorAjarState(DoorAjarStateId, V_LOCK_STATUS);
MyMessage DoorAlarmEnabler(DoorAlarmEnableId, V_LOCK_STATUS);
MyMessage UnlockTriggerMsg(LockTripMsgId, V_TRIPPED);
uint8_t _prevIDm[8];
unsigned long _prevTime;

void setup()
{
	pinMode(LockOutput, OUTPUT);
	pinMode(OutsideSpeaker, OUTPUT);
	pinMode(OutsideReadInd, OUTPUT);
	pinMode(AtHomeInd, OUTPUT);
	pinMode(InsideEntryBtn, INPUT_PULLUP);
	pinMode(IrqPin, INPUT);
	pinMode(OutsideEntryBtn, INPUT_PULLUP);
	pinMode(DoorAjarSensor, INPUT_PULLUP);
	Serial.println("Serial init okay!");
	nfc.begin();
	uint32_t versiondata = nfc.getFirmwareVersion();
	if (!versiondata)
	{
		Serial.print("Didn't find PN53x board");
		while (1) { delay(10); };      // halt
	}

	Serial.print("Found chip PN5"); Serial.println((versiondata >> 24) & 0xFF, HEX);
	Serial.print("Firmware ver. "); Serial.print((versiondata >> 16) & 0xFF, DEC);
	Serial.print('.'); Serial.println((versiondata >> 8) & 0xFF, DEC);

	nfc.SAMConfig();

	memset(_prevIDm, 0, 8);
	Serial.println("Door controller ready.");
}

void loop()
{
	if (Serial.available() > 0) {
		String command = Serial.readString();
		if (command == "Debug") {
			printDebug = !printDebug;
			Serial.print("Serial debugging ");
			Serial.println(printDebug ? "enabled!" : "disabled!");
		}
	}
	uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
	uint8_t uidLength;
		if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength, 50U)) {
			if (printDebug) {
				Serial.println("Found a card!");
				Serial.print("UID Length: "); Serial.print(uidLength, DEC); Serial.println(" bytes");
			}
			Serial.print("{ ");
			for (uint8_t i = 0; i < uidLength; i++)
			{
				Serial.print(uid[i], DEC);
				if (i != (uidLength - 1)) {
					Serial.print("U, ");
				}
			}
			Serial.println("U }");
			if (CheckRfidKey(uid)) {
				Unlock();
			}
			else {
				if (printDebug) {
					Serial.println("Card was not a valid entry card.");
				}
				wait(500);
			}

		}
	CardPresent = false;
	long startTime = millis();
	if (PinIsLow(DoorAjarSensor)) {
		if (DoorAjarTime == 0 && !DoorOpened) {
			DoorAjarTime = millis();
			DoorOpened = true;
			send(DoorAjarState.set(true));
		}
	}
	if (PinIsHigh(DoorAjarSensor)) {
		DoorAjarTime = 0;
		DoorOpened = false;
		send(DoorAjarState.set(false));
	}
	if (PinIsLow(InsideEntryBtn))
	{
		delay(10);
		if (printDebug) {
			Serial.println("Exit pressed");
		}
		while (PinIsLow(InsideEntryBtn))
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

	if (PinIsLow(OutsideEntryBtn))
	{
		if (printDebug) {
			Serial.println("Outside btn pressed.");
		}
		if (AtHome == true) {
			Unlock();
		} else {
			tone(OutsideSpeaker, 1800, 550);
			wait(500);
			tone(OutsideSpeaker, 1500, 1000);
		}
	}
	digitalWrite(AtHomeInd, AtHome);

	if (millis() > IdleTime + 1000)
	{
		digitalWrite(OutsideReadInd, HIGH);
		delay(5);
		digitalWrite(OutsideReadInd, LOW);
		IdleTime = millis();
	}
}

void PrintHex8(const uint8_t d) {
	Serial.print(" ");
	Serial.print((d >> 4) & 0x0F, HEX);
	Serial.print(d & 0x0F, HEX);
}

void presentation()
{
	// Send the sketch version information to the gateway and Controller
	sendSketchInfo("Secure Lock", "1.0");
	present(LockChildId, S_LOCK);
}

void ChangeLockStatus() {
	if (printDebug) {
		Serial.println("Changing modes now!");
	}
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
	tone(OutsideSpeaker, 1800, 1000);
	wait(500);
	tone(OutsideSpeaker, 1500, 1000);
	digitalWrite(LockOutput, HIGH);
	digitalWrite(OutsideReadInd, LOW);
	if (printDebug) {
		Serial.println("Door was triggered!!");
	}
	while (millis() <= LockTimer)
	{
		digitalWrite(OutsideReadInd, HIGH);
		delay(50);
		digitalWrite(OutsideReadInd, LOW);
		delay(100);
		if (digitalRead(DoorAjarSensor) == LOW) {
			//break;
		}
	}
	digitalWrite(LockOutput, LOW);
	digitalWrite(OutsideReadInd, LOW);
	LockTimer = 0;
}

bool CheckRfidKey(uint8_t keyToTest[]) {
	int sizeOfKeys = MasterKeyCount;
	for (int x = 0; x < sizeOfKeys; x++) {
		bool pass = true;
		for (int n = 0; n < 7; n++) {
			if (keyToTest[n] != MasterKeys[x][n]) {
				pass = false;
			}
		}
		if (pass) {
			return true;
		}
	}
	return false;
}

void receive(const MyMessage& message)
{
	if (message.isEcho()) {
		return;
	}
	if (message.getSensor() == LockTripMsgId) {
		// Change relay state
		AtHome = !AtHome;
		if (printDebug) {
			Serial.print("Incoming change for lock:");
			Serial.print(message.getSensor());
			Serial.print(", New status: ");
			Serial.println(AtHome ? "enabled" : "disabled");
		}
	}

	if (message.sensor == LockStatusMsgId) {
		send(LockStatus.set(AtHome));
	}

	if (message.sensor == RemoteLockTriggerId) {
		Unlock();
	}
}

void ShouldReadCard() {
	CardPresent = true;
}

bool PinIsLow(uint8_t targetPin) {
	return digitalRead(targetPin) == LOW;
}

bool PinIsHigh(uint8_t targetPin) {
	return digitalRead(targetPin) == HIGH;
}
