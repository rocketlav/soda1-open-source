#include <Adafruit_BMP280.h> // Изменить в этой бибилотеке адрес устройства на 0x76
#include <SoftwareSerial.h>
#include <SD.h>

/// Контактные пины, не трогать, в плате они фиксированные
SoftwareSerial GPS(9, 10); //Инициализация GPS по программному сериалу RX, TX
Adafruit_BMP280 BMP;   //Инициализация барометра
const int chipSelect = 8;
File dataFile;

// ПЕРЕМЕННЫЕ

unsigned long DelayForBMP = 125; // Время опроса барометра
unsigned long BufferForBMP = 0;

unsigned long DelayForSD = 10000; // Время сохранения данных на сд
unsigned long BufferForSD = 0;

float BMSKoef = 0.01346; //Voltage

unsigned long StartTime = 0;
unsigned long SaveTimer = 5500; // Время раскрытия после отрыва
unsigned long SaveEndTime = 1500; // Время работы двигателя системы открытия
unsigned long EndBuffer = 0;

unsigned long Save2Timer = 1000; // + SaveTimer система спасения
unsigned long Save2EndTime = 1500; // Время работы двигателя системы открытия
unsigned long End2Buffer = 0;

bool StartDrive = false;
bool Start2Drive = false;

void setup()
{
  pinMode(A0, INPUT); // VCC VOlTAGE
  pinMode(A3, INPUT_PULLUP); // jumper
  pinMode(3, INPUT); // пружина (Верхний)
  pinMode(2, INPUT); // голова (Нижний)
  // motor
  pinMode(4, OUTPUT); // Пружина
  pinMode(5, OUTPUT);

  pinMode(6, OUTPUT); // Голова
  pinMode(7, OUTPUT);

  // Инициализация всех портов
  Serial.begin(9600); // БОД радио модуля
  while (!Serial) {
    // wait for serial port to connect. Needed for native USB port only
  }

  GPS.begin(9600);

  if (!BMP.begin()) {  //Подключаем барометр
    Serial.println("BMP NOT FOUND");
  }
  else Serial.println("BMP IS OK");

  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
  }
  else Serial.println("CARD IS OK");

  dataFile = SD.open("datalog.txt", FILE_WRITE);

  float voltage = analogRead(A0) * BMSKoef;
  Serial.println("Voltage: " + String(voltage));
  dataFile.println("Voltage: " + String(voltage));

  Serial.println("GPS value: ");
  // GPS tester
  bool gpsTester = false;
  while (!gpsTester) {
    while (GPS.available()) {
      char a = GPS.read();
      Serial.write(a); // Вывод в радио
      gpsTester = true;
    }
  }

  Serial.println("");
  Serial.println("INIT END");

  bool isReady = false;
  int headValue = digitalRead(2);
  int springValue = digitalRead(3);
  bool ok = true;
  while (!isReady) {
    if (headValue == 0 && springValue == 0 && ok) {
      Serial.println("READY FOR START, WAITING FOR A JUMPER UNLOCK");
      ok = false;
    }
    if (headValue == 0 && springValue == 0) {
      while (GPS.available()) {
        char a = GPS.read();
        dataFile.write(a);
        Serial.write(a); // Вывод в радио
      }
    }
    if (digitalRead(A3) == 1) {
      isReady = true;
      Serial.println("START");
    }



    int head = digitalRead(2);
    if (head != headValue) {
      if (head == 1) {
        headValue = 1;



        Serial.println("SPRING IS OPENNING");
        digitalWrite(4, HIGH);
        digitalWrite(5, LOW);
        delay(SaveEndTime * 2);
        digitalWrite(4, LOW);
        digitalWrite(5, LOW);
        Serial.println("SPRING OPENED");

      }
      else {
        headValue = 0;
        Serial.println("SPRING IS CLOSING");
        digitalWrite(4, LOW);
        digitalWrite(5, HIGH);
        delay(SaveEndTime);
        digitalWrite(4, LOW);
        digitalWrite(5, LOW);
        Serial.println("SPRING CLOSED");
        ok = true;
      }
    }

    int spring = digitalRead(3);
    if (spring != springValue) {
      if (spring == 1) {
        springValue = 1;

        Serial.println("HEAD IS OPENNING");
        digitalWrite(6, HIGH);
        digitalWrite(7, LOW);
        delay(Save2EndTime * 2);
        digitalWrite(6, LOW);
        digitalWrite(7, LOW);
        Serial.println("HEAD OPENED");
      }
      else {
        springValue = 0;
        Serial.println("HEAD IS CLOSING");
        digitalWrite(6, LOW);
        digitalWrite(7, HIGH);
        delay(Save2EndTime);
        digitalWrite(6, LOW);
        digitalWrite(7, LOW);
        Serial.println("HEAD CLOSED");
        ok = true;
      }
    }
  }
  StartTime = millis();
}

void loop()
{
  // BMP280
  if (millis() - BufferForBMP > DelayForBMP) {
    float info = BMP.readAltitude(1013.25);
    dataFile.println("<MILLIS : " + String(millis() - StartTime) + " BMP : " + String(info) + ">");
    //Serial.println("MILLIS : " + String(millis() - StartTime) + " BMP : " + String(info));
    BufferForBMP = millis();
  }

  // GPS
  while (GPS.available()) {
    char a = GPS.read();
    dataFile.write(a);
    Serial.write(a); // Вывод в радио
  }

  if (millis() - BufferForSD > DelayForSD) {
    if (dataFile) dataFile.close();
    dataFile = SD.open("datalog.txt", FILE_WRITE);

    BufferForSD = millis();
  }

  if (!StartDrive && millis() - StartTime > SaveTimer) {
    StartDrive = true;
    EndBuffer = millis();
    digitalWrite(6, HIGH);
    digitalWrite(7, LOW);
  }
  if (!Start2Drive && millis() - StartTime > SaveTimer + Save2Timer) {
    Start2Drive = true;
    End2Buffer = millis();
    digitalWrite(4, HIGH);
    digitalWrite(5, LOW);
  }
  if (millis() - EndBuffer > SaveEndTime * 2) {
    digitalWrite(6, LOW);
    digitalWrite(7, LOW);
  }
  if (millis() - End2Buffer > Save2EndTime * 2) {
    digitalWrite(4, LOW);
    digitalWrite(5, LOW);
  }

}
