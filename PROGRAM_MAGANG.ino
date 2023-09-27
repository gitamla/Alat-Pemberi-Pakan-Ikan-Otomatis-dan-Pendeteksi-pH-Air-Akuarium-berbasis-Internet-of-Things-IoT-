#include <OneWire.h>
#include <DallasTemperature.h>
#include "RTClib.h"
#include <Wire.h>
#include <ESP32Servo.h>
//#include <ESP8266WiFi.h>
//#include <BlynkSimpleEsp8266.h>
#include <BlynkSimpleEsp32.h>

#define BLYNK_TEMPLATE_ID "TMPL6Kik3dMhG" 
#define BLYNK_TEMPLATE_NAME "PAKAN IKAN"
#define BLYNK_AUTH_TOKEN "Dr8hu99Wtr8R3-A7oZs3WWGQ4s649NqW"
#define BLYNK_PRINT Serial

char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "Yollanda";
char pass[] = "bentardulu";

BlynkTimer timer;
Servo myservo;
int manualFeed;
WidgetLED ledmanual (V6);

//-------RTC DS3231--------//
RTC_DS3231 rtc;
char daysOfTheWeek[7][12] = {"Minggu", "Senin", "Selasa", "Rabu", "Kamis", "Jum'at", "Sabtu"};
String tahunsekarang, bulansekarang, harisekarang;
String jamsekarang, menitsekarang, detiksekarang;
String hari, tahun, waktu;

//-------PH--------//
const int phpin  = 35;
float Po = 0, phstep;
int nilaianalogph;
double teganganph;

//untuk kalibrasi PH Sensor
float PH4 = 3.0; //3.00
float PH7 = 11.5;

//-------RELAY--------//
#define pompaup 19
#define pompadown 21  

//-------ULTRASONIK UP--------//
const int trigup = 22;
const int echoup = 23;
long durasiup, jarakup;

//-------ULTRASONIK DOWN--------//
const int trigdown = 14;
const int echodown = 27;
long durasidown, jarakdown;

//-------ULTRASONIK PAKAN--------//
const int trigpakan = 5;
const int echopakan = 4;
long durasipakan, jarakpakan;

//-------SENSOR SUHU AIR--------//
#define oneWireBus 33
OneWire oneWire(oneWireBus);
DallasTemperature sensors (&oneWire);
float tempC; 
float tempF;

//-------FLOW--------//
#define sensorflow 18 

float calibrationFactor = 4.5;
volatile byte pulseCount = 0;
float flowRate;
unsigned int flowMilliLitres = 0;
unsigned long totalMilliLitres = 0;
unsigned long oldTime = 0;


void IRAM_ATTR pulseCounter() {
  pulseCount++;
}

void setup() {
  Serial.begin(9600);
  Wire.begin(26,25);
  sensors.begin();
  rtc.begin();  
  rtc.adjust(DateTime(F(__DATE__),F(__TIME__)));
  //rtc.adjust(DateTime(2023, 7, 5, 15, 54, 20 ));   //Kalibrasi
  myservo.attach(12);

  pinMode(sensorflow, INPUT_PULLUP);
  digitalWrite(sensorflow,HIGH);
  pulseCount        = 0;
  flowRate          = 0.0;
  flowMilliLitres   = 0;
  totalMilliLitres  = 0;
  oldTime           = 0;
  attachInterrupt(digitalPinToInterrupt(sensorflow), pulseCounter, FALLING);

  pinMode(trigup, OUTPUT);
  pinMode(echoup, INPUT);
  pinMode(trigdown, OUTPUT);
  pinMode(echodown, INPUT);
  pinMode(trigpakan, OUTPUT); 
  pinMode(echopakan, INPUT);
  pinMode(phpin, INPUT); 
  
  pinMode(pompaup, OUTPUT);
  pinMode(pompadown, OUTPUT);
  digitalWrite(pompaup, HIGH);    // Pompa PH UP Mati
  digitalWrite(pompadown, HIGH);  // Pompa PH DOWN Mati
  
  /*timer.setInterval(1000L, modulrtc);
  timer.setInterval(1000L, ultrasonikup);
  timer.setInterval(1000L, ultrasonikdown);
  timer.setInterval(1000L, ultrasonikpakan);
  timer.setInterval(1000L, suhuair);
  timer.setInterval(1000L, sensorph);
  timer.setInterval(1000L, flow);
  timer.setInterval(1000L, autoPH);*/
  
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  int wifi_ctr = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  Blynk.begin(auth, ssid, pass, "sgp1.blynk.cloud", 80);
}

//-------BUTTON MANUAL--------//
BLYNK_WRITE(V5) {
  manualFeed = param.asInt();
  if (manualFeed == 1){
    myservo.write(45);
    ledmanual.on();
  }
  if (manualFeed == 0){
    myservo.write(0);
    ledmanual.off();
  }
}

void modulrtc() {
  DateTime now = rtc.now();

  tahunsekarang = now.year();
  bulansekarang = now.month();
  harisekarang = now.day();
  jamsekarang = now.hour();
  menitsekarang = now.minute();
  detiksekarang = now.second();
  hari = daysOfTheWeek[now.dayOfTheWeek()];
  tahun = harisekarang + "/" + bulansekarang + "/" + tahunsekarang;
  waktu = jamsekarang + ":" + menitsekarang + ":" + detiksekarang;
  Serial.print (hari);
  Serial.print ("\t");
  Serial.print (tahun);
  Serial.print ("\t");
  Serial.print (waktu);
  Serial.println ();

  Blynk.virtualWrite(V0, hari);
  Blynk.virtualWrite(V1, tahun);
  Blynk.virtualWrite(V2, waktu);

  //-------FEEDING OTOMATIS-------//
  if (now.hour() == 8 && now.minute() == 0 && now.second() == 0) {
    myservo.write(45);
    }
    else if (now.hour() == 18 && now.minute() == 0 && now.second() == 0) {
      myservo.write(45);
      }
      else {
        myservo.write(0);
        }
}

void sensorph() {
  nilaianalogph = analogRead(phpin);
  //Serial.print("Nilai ADC Ph: ");
  //Serial.println(nilaianalogph);
  teganganph = (3.3 / 1024.0) * nilaianalogph;
  //Serial.print("TeganganPh: ");
  //Serial.println(teganganph);

  phstep = (PH4 - PH7) / 3;
  Po = 7.00 + ((PH7 - teganganph) / phstep); 
  //Po =(abs(7.00 + ((PH7 - teganganph) / phstep))) / 10;     //Po = 7.00 + ((teganganPh7 - TeganganPh) / PhStep);
  Serial.print("pH Value : ");
  Serial.println(Po);
  Blynk.virtualWrite(V3, Po);
}

void autoPH() {
  if (Po < 5.0){
    digitalWrite(pompaup, LOW);
    if (totalMilliLitres >= 100){
      digitalWrite(pompaup, HIGH);
      flowMilliLitres = 0;
      totalMilliLitres = 0;
      return;
    }
  }
  
  if (Po > 8.5){
    digitalWrite(pompadown, LOW);
    if (totalMilliLitres >= 100){
      digitalWrite(pompadown, HIGH);
      flowMilliLitres = 0;
      totalMilliLitres = 0;
      return;
    }
  }
  
  else if (Po >= 5.0 && Po <= 8.5){
    digitalWrite(pompaup, HIGH);
    digitalWrite(pompadown, HIGH);
  }
}

void ultrasonikup () {
  digitalWrite(trigup, LOW);
  delayMicroseconds(2);
  digitalWrite(trigup, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigup, LOW);
  delayMicroseconds(2);

  durasiup = pulseIn(echoup, HIGH);
  jarakup = durasiup * 0.034 / 2;
  Serial.print("pH UP : ");
  Serial.print(jarakup);
  Serial.print ("cm");
  Serial.print ("\t");

  if (jarakup > 4) {
    Blynk.virtualWrite(V8, "pH UP Habis");
    Serial.println ("pH UP Habis");
  }
  else {
    Blynk.virtualWrite(V8, "pH UP Tersedia");
    Serial.println ("pH UP Tersedia");
  }
}

void ultrasonikdown () {
  digitalWrite(trigdown, LOW);
  delayMicroseconds(2);
  digitalWrite(trigdown, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigdown, LOW);
  delayMicroseconds(2);

  durasidown = pulseIn(echodown, HIGH);
  jarakdown = durasidown * 0.034 / 2;
  Serial.print("pH DOWN : ");
  Serial.print(jarakdown);
  Serial.print ("cm");
  Serial.print ("\t");

  if (jarakdown > 4) {
    Blynk.virtualWrite(V9, "pH DOWN Habis");
    Serial.println ("pH DOWN Habis");
  }
  else {
    Blynk.virtualWrite(V9, "pH DOWN Tersedia");
    Serial.println ("pH DOWN Tersedia");
  }
}

void ultrasonikpakan () {
  digitalWrite(trigpakan, LOW);
  delayMicroseconds(2);
  digitalWrite(trigpakan, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigpakan, LOW);
  delayMicroseconds(2);

  durasipakan = pulseIn(echopakan, HIGH);
  jarakpakan = durasipakan * 0.034 / 2;
  Serial.print("Pakan : ");
  Serial.print(jarakpakan);
  Serial.print ("cm");
  Serial.print ("\t");

  if (jarakpakan > 5) {
    Blynk.virtualWrite(V7, "Pakan Kosong");
    Serial.println ("Pakan Kosong");
  }
  else {
    Blynk.virtualWrite(V7, "Pakan Penuh");
    Serial.println ("Pakan Penuh");
  }
}

//---------SUHU AIR---------//
void suhuair() {
  sensors.requestTemperatures();       // send the command to get temperatures
  //tempC = (abs(sensors.getTempCByIndex(0))) /4.5;
  tempC = sensors.getTempCByIndex(0);  // read temperature in °C
  //tempF = sensors.getTempFByIndex(0); // convert °C to °F

  Serial.print("Temperature : ");
  Serial.print(tempC);    
  Serial.println("°C");

  Blynk.virtualWrite(V4, tempC);
}

void flow(){
  if((millis() - oldTime) > 1000) {
    detachInterrupt(digitalPinToInterrupt(sensorflow));
    flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / calibrationFactor;
    oldTime = millis();
    flowMilliLitres = (flowRate / 60) * 1000;
    totalMilliLitres += flowMilliLitres;

    // Print the flow rate for this second in litres / minute
    /*Serial.print("Flow rate: ");
    Serial.print(flowMilliLitres, DEC);  // Print the integer part of the variable
    Serial.print("mL/Second");
    Serial.print("\t");*/           

    // Print the cumulative total of litres flowed since starting
    Serial.print("Liquid Quantity : ");        
    Serial.print(totalMilliLitres,DEC);
    Serial.println("mL");      
        
    // Reset the pulse counter so we can start incrementing again
    pulseCount = 0;

    // Enable the interrupt again now that we've finished sending output
    attachInterrupt(digitalPinToInterrupt(sensorflow), pulseCounter, FALLING);
  }
}

void loop() {
  Blynk.run();
  timer.run();
  modulrtc();
  ultrasonikup();
  ultrasonikdown();
  ultrasonikpakan();
  suhuair();
  sensorph();
  flow();
  autoPH();
}
