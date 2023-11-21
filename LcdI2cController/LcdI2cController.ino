#include <SoftwareSerial.h>
#include <Keypad.h>

const uint8_t buzzerPin = 10;
const byte ROWS = 4; //four rows
const byte COLS = 4; //three columns
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'S','0','E','D'}
};
byte rowPins[ROWS] = { 2, 3, 4, 5 }; //connect to the row pinouts of the keypad
byte colPins[COLS] = { 6, 7, 8, 9 }; //connect to the column pinouts of the keypad

SoftwareSerial SoftConn(11, 12);
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

void setup() {
    Serial.begin(115200);
    SoftConn.begin(115200);
    SoftConn.listen();
}

void BuzzerBeep(int beepCount) {
    int loops = beepCount;
    while (loops > 0) {
        tone(buzzerPin, 2000, 75);
        delay(150);
        loops--;
    }
}

void loop() {
    while (SoftConn.available()) {
        uint8_t byte = SoftConn.read();
        Serial.print("Byte from SoftConn: ");
        Serial.println(byte);
        switch (byte)
        {
        case 0x00:
            BuzzerBeep(2);
            break;
        case 0x01:
            BuzzerBeep(5);
            break;
        default:
            break;
        }
    }
    char key = keypad.getKey();

    if (key != NO_KEY) {
        switch (key)
        {
        case '1':
            SoftConn.write(0x01);
            break;
        case '2':
            SoftConn.write(0x02);
            break;
        case '3':
            SoftConn.write(0x03);
            break;
        case '4':
            SoftConn.write(0x04);
            break;
        case '5':
            SoftConn.write(0x05);
            break;
        case '6':
            SoftConn.write(0x06);
            break;
        case '7':
            SoftConn.write(0x07);
            break;
        case '8':
            SoftConn.write(0x08);
            break;
        case '9':
            SoftConn.write(0x09);
            break;
        case 'S':
            SoftConn.write(0x10);
            break;
        case 'E':
            SoftConn.write(0x20);
            break;
        case 'A':
            SoftConn.write(0x0A);
            break;
        case 'B':
            SoftConn.write(0x0B);
            break;
        case 'C':
            SoftConn.write(0x0C);
            break;
        case 'D':
            SoftConn.write(0x0D);
            break;
        case '0':
            int zero = 0x00;
            SoftConn.write(zero);
            break;
        default:
            break;
        }
        delay(100);
    }
}