// Defined nodes
#include "LcdViewMethods.h"
#define GatewayNodeId 0
#define GatewayTempSensorId 1

#define DoorNodeID 3
#define DoorLockStatusId 0
#define UnlockTriggerId 1

#define IelytNodeID 4
#define IelytaTempSensorId 0
#define IelytaHumdSensorId 1
#define IelytaHumdStatusId 2
#define IelytaVapeAlarmId 3
#define IelytaMasterHumdEnableId 4
#define IelytaRiseAboveStateId 5
#define IelytaFallBelowStateId 6

#define LoganNodeID 5
#define LoganHeatSensorId 0
#define OreoTempSensorId 1
#define OreoHumdSensorId 2
#define LoganHeatStatusId 3
#define LoganMasterHeatEnableId 4
#define LoganMasterHumdEnableId 5
#define LoganRiseAboveStateId 6
#define LoganFallBelowStateId 7

#define BuzzerPin = 14;

#include <ESP8266WiFi.h>

#include <Arduino.h>
#include <U8g2lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif
#include <SoftwareSerial.h>

#ifndef STASSID
#define STASSID "ATL-2G"
#define STAPSK  "L1tA177!"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

const char* host = "10.0.10.254";
const uint16_t port = 5003;

// Screen timeout variables.
const unsigned long screenTimeout = 15000;
const unsigned long clientTimeout = 20000;
const unsigned long connectRetryTimer = 5000;
unsigned long screenEnableTime;
unsigned long lastMessageRecTime;
unsigned long lastReconTime;
bool screenEnabled = false;

const uint8_t buzzerPin = 12;

bool sendCmd = true;
float lTemp, oTemp, oHumd, iTemp, iHumd, rTemp, rHumd;
bool lHeatState, lHeatEnable, lHumdEnable, lockState, lockTrigger, iHumdState, iHumdEnable;
bool masterMenuEnabled, settingsMenuEnabled, optionsMenuEnabled;
String debugString = "Debug";

WiFiClient client;
SoftwareSerial SoftConn(12, 14);
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);


void u8g2_prepare(void) {
    //u8g2.setFlipMode(1);
    u8g2.setFontMode(1);
    u8g2.setDrawColor(1);
    u8g2.setFontRefHeightExtendedText();
    u8g2.setFontPosTop();
    u8g2.setFontDirection(0);
    u8g2.setFont(u8g2_font_6x10_tf);
}

void handleReconnect() {
    client.stop();
    sendCmd = false;
    delay(100);
    if (millis() > connectRetryTimer + lastReconTime) {
        lastReconTime = millis();
        if (!client.connect(host, port)) {
            Serial.println("connection failed");
            delay(5000);
        }
        else {
            Serial.println("connection re-established.");
            lastMessageRecTime = millis();
            delay(100);
            sendCmd = true;
        }

    }
}

void MasterMenuUpdate() {
    if (screenEnabled) {
        u8g2.setPowerSave(0);
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_6x12_tf);
        u8g2.setCursor(0, 0);
        if (sendCmd) {
            u8g2.print("Room Temp: " + String(rTemp) + " *F");
        }
        else {
            u8g2.print("CONNECTION ERROR!");
        }
        u8g2.drawHLine(0, 15, 128);
        String drawnState = lockState ? "Unlocked" : "Locked";
        String doorStr = "Door State: " + String(drawnState);
        String loganTemp = "Logan: " + String(lTemp) + " *F:" + String(oHumd) + " %";
        u8g2.setCursor(0, 17);
        u8g2.print(doorStr);
        u8g2.drawHLine(0, 32, 128);
        u8g2.setCursor(0, 34);
        u8g2.print(loganTemp);
        u8g2.drawHLine(0, 48, 128);
        u8g2.setCursor(0, 50);
        u8g2.print("Ielyta: " + String(iTemp) + " *F" + String(iHumd) + " %");
        u8g2.sendBuffer();
        u8g2.setFont(u8g2_font_6x10_tf);
    }
    else {
        u8g2.setPowerSave(1);
    }
}

void DoorTriggeredScreen() {
    SoftConn.write(0x01);
    u8g2.setPowerSave(0);
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_t0_17_tf);
    u8g2.drawBox(0, 0, 128, 22);
    u8g2.drawBox(0, 44, 128, 26);
    u8g2.sendBuffer();
    for (int i = 0; i < 9; i++) {
        u8g2.updateDisplay();
        u8g2.drawStr(0, 24, "");
        delay(500);
        u8g2.updateDisplay();
        u8g2.drawStr(0, 24, "*DOOR OPENED!*");
        delay(500);
    }
    u8g2.updateDisplay();
    u8g2.drawStr(0, 24, "*DOOR OPENED!*");
    delay(5000);
    u8g2.setPowerSave(1);
}

void DoorStateRequested() {
    SoftConn.write(0x00);
    u8g2.setPowerSave(0);
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_t0_17_tf);
    u8g2.drawStr(0, 14, "*DOOR TRIGGER*");
    u8g2.drawStr(0, 30, "*MESSAGE SENT*");
    u8g2.sendBuffer();
    delay(5000);
    u8g2.setPowerSave(1);
}

void DoorStateMessage() {
    SoftConn.write(0x00);
    u8g2.setPowerSave(0);
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_t0_17_tf);
    u8g2.drawStr(0, 14, "*DOOR TRIGGER*");
    if (lockState) {
        u8g2.drawStr(16, 30, "*UNLOCKED*");
    }
    else {
        u8g2.drawStr(28, 30, "*LOCKED*");
    }
    u8g2.sendBuffer();
    delay(5000);
    u8g2.setPowerSave(1);
}

int DrawMasterSettingsMenu(String entries[], int count) {
    if (settingsMenuEnabled) {
        int selectedValue = 1;
        int boxLocX = 0;
        int boxLocY = 16;
        int menuItemCount = count;
        int hozPos = 16;
        int boxSpacing = 12;
        int textOffset = 0;
        int keypadEntry = SoftConn.read();
        while (keypadEntry != 16) {
            u8g2.clearBuffer();
            u8g2.setFont(u8g2_font_6x10_tf);
            u8g2.setCursor(0, 0);
            u8g2.println("Device Setup " + String(selectedValue) + " " + String(menuItemCount));
            u8g2.drawHLine(0, 14, 128);
            u8g2.setDrawColor(1);
            u8g2.drawBox(boxLocX, boxLocY, 64, 12);
            u8g2.setDrawColor(2);
            for (int e = 0; e < menuItemCount; e++) {
                u8g2.setCursor(textOffset, hozPos);
                u8g2.println(entries[e]);
                hozPos += boxSpacing;
                if (e == 3) {
                    hozPos = 16;
                    textOffset = 64;
                }
            }
            hozPos = 16;
            textOffset = 0;
            if (selectedValue >= 1 && selectedValue < 5) {
                boxLocX = 0;
                if (keypadEntry == 2 && selectedValue > 1) {
                    selectedValue--;
                    boxLocY -= 12;
                    u8g2.sendBuffer();
                    delay(100);
                }
                if (keypadEntry == 8) {
                    selectedValue++;
                    boxLocY += 12;
                    u8g2.sendBuffer();
                    delay(100);
                }
                if (selectedValue == 4) {
                    boxLocY = 52;
                    boxLocX = 0;
                    u8g2.sendBuffer();
                }
            }
            if (selectedValue > 4 && selectedValue < menuItemCount) {
                boxLocX = 64;
                if (keypadEntry == 8) {
                    if ((selectedValue == 5 && boxLocY == 16) || selectedValue != 5) {
                        selectedValue++;
                        boxLocY += 12;
                        u8g2.sendBuffer();
                        delay(100);
                    }
                }
                if (selectedValue == 5) {
                    boxLocY = 16;
                    u8g2.sendBuffer();
                }
                if (keypadEntry == 2) {
                    selectedValue--;
                    if (selectedValue != 4) {
                        boxLocY -= 12;
                    }
                    u8g2.sendBuffer();
                    delay(100);
                }
                if (selectedValue == 4) {
                    boxLocY = 52;
                    boxLocX = 0;
                    u8g2.sendBuffer();
                }
            }
            if (digitalRead(15) == 1 && selectedValue == menuItemCount) {
                selectedValue--;
                boxLocY -= 12;
                u8g2.sendBuffer();
            }
            u8g2.sendBuffer();
            delay(200);
            if (keypadEntry == 32) {
                return selectedValue;
            }
        }
    }
    Serial.println("Menu exit.");
    return 0;
}


void setup() {
    Serial.begin(115200);
    SoftConn.begin(115200);

    u8g2.begin();
    u8g2_prepare();

    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    static bool wait = false;

    Serial.print("connecting to ");
    Serial.print(host);
    Serial.print(':');
    Serial.println(port);

    if (!client.connect(host, port)) {
        Serial.println("connection failed");
    }
    lastReconTime = millis();
}

void loop() {
    if (SoftConn.available() && masterMenuEnabled) {
        uint8_t byte = SoftConn.read();
        Serial.print("Byte received from ESP: ");
        Serial.println(byte);
        switch (byte)
        {
        case 10:
            screenEnableTime = millis();
            screenEnabled = true;
            break;
        case 11:
            client.println("3;1;1;0;36;1");
            delay(100);
            DoorStateRequested();
            break;
        case 12:     
            masterMenuEnabled = false;
            settingsMenuEnabled = true;
            break;
        case 13:

            break;
        default:
            break;
        }
    }
    if (screenEnabled) {
        if (millis() > screenEnableTime + screenTimeout) {
            screenEnabled = false;
        }
    }
    if (millis() > clientTimeout + lastMessageRecTime) {
        handleReconnect();
    }
    if (sendCmd) {
        String temp = "";
        while (client.available()) {
            char ch = static_cast<char>(client.read());
            temp = temp + ch;
        }
        if (temp != "") {
            lastMessageRecTime = millis();
            int index = 0;
            int delIndex = 0;
            int count = 0;
            String Array[6];
            while (count < temp.length()) {
                delIndex = temp.indexOf(";", count);
                if (index == 5 && delIndex != -1) {
                    delIndex = delIndex - 2;
                }
                if (index == 5 && delIndex == -1) {
                    delIndex = temp.length() - 1;
                }
                Array[index] = temp.substring(count, delIndex);
                count = delIndex + 1;
                index = index + 1;
                if (index == 6) {
                    break;
                }
            }
            int nodeId = Array[0].toInt();
            int sensorId = Array[1].toInt();
            if (nodeId == DoorNodeID) {
                if (sensorId == DoorLockStatusId) {
                    bool oldState = lockState;
                    lockState = Array[5].toInt();
                    if (oldState != lockState) {
                        DoorStateMessage();
                    }
                }
                if (sensorId == UnlockTriggerId) {
                    DoorTriggeredScreen();
                }
            }
            if (nodeId == GatewayNodeId) {
                if (sensorId == GatewayTempSensorId) {
                    rTemp = Array[5].toFloat();
                }
            }
            if (nodeId == IelytNodeID) {
                if (sensorId == IelytaTempSensorId) {
                    iTemp = Array[5].toFloat();
                }
                if (sensorId == IelytaHumdSensorId) {
                    iHumd = Array[5].toFloat();
                }
                if (sensorId == IelytaHumdStatusId) {
                    iHumdState = Array[5].toInt();
                }
            }
            if (nodeId == LoganNodeID) {
                if (sensorId == LoganHeatSensorId) {
                    lTemp = Array[5].toFloat();
                }
                if (sensorId == LoganHeatStatusId) {
                    lHeatState = Array[5].toInt();
                }
                if (sensorId == OreoHumdSensorId) {
                    oHumd = Array[5].toFloat();
                }
                if (sensorId == OreoTempSensorId) {
                    oTemp = Array[5].toFloat();
                }
                if (sensorId == LoganMasterHeatEnableId) {
                    lHeatState = Array[5].toInt();
                }
            }
        }
    }
    if (masterMenuEnabled) {
        MasterMenuUpdate();
    }
    if (settingsMenuEnabled) {
        const int cnt = 6;
        String sensors[cnt] = { "Display", "Gateway", "Door", "Logan", "Ielyta", "Test" };
        int selection = DrawMasterSettingsMenu(sensors, cnt);
        Serial.print
    }
}

