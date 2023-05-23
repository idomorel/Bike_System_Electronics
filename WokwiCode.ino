/*
Author: Ido Morel

Code built for ESP32-Wroom-32

This is the code for our final year project under the Electronics program.
---------------------------------------------------------------------------
TODO:
Implement Wifi Capabillities
*/
#define SDSPI   // Didn't know what SD module we have so I put a selection here
#define SD_CS 5 // SD card chip select pin
#define tireDiameter 0.05 // The diameter of the tire in meters

#ifdef SDSPI

#include <SD.h>
#include <SPI.h>
#include <FS.h>

#endif

//Connect the project to the wifi for the website
const char *ssid = "Manor's Wifi ";
const char *password = "6677889900";

// At first we include the needed libraries into our program
#include <LiquidCrystal_I2C.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include "esp_system.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>

// Then we define some pin values.
// We use #define because it is helpful for coding during development
// and only used by the compiler on the computer so it doesn't take space on our microcontroller.

#define nextButton 27 // The pin number for the button used as the "next page" button
#define prevButton 26 // The pin number for the button used as the "previous page" button
#define selButton 25  // The pin number for the button used as the "Select" button

int nextButtonState = 0; // The state of the button (pressed or not)
int prevButtonState = 0; // The state of the button (pressed or not)
int selButtonState = 0;  // The state of the button (pressed or not)

int nextButtonReading = 0; // The reading of the button (pressed or not)
int prevButtonReading = 0; // The reading of the button (pressed or not)
int selButtonReading = 0;  // The reading of the button (pressed or not)

int nextButtonLastState = 0; // The last state of the button (pressed or not)
int prevButtonLastState = 0; // The last state of the button (pressed or not)
int selButtonLastState = 0;  // The last state of the button (pressed or not)

unsigned long nextButtonLastDebounceTime = 0; // The last time the button output a stable state
unsigned long prevButtonLastDebounceTime = 0; // The last time the button output a stable state
unsigned long selButtonLastDebounceTime = 0;  // The last time the button output a stable state

unsigned long debounceDelay = 50; // The delay in which we consider the button state stable

volatile byte rotation;
float timetaken;
float rpm;
// float dtime;
int velocity = 0;
int bikeAngle = 0;
int lastVelocity = 0;
unsigned long totalDistance = 0;
unsigned long lastDistance = 0;
int displayMode = 0;
int lastDisplayMode = 0;
unsigned long prevTime = 0;
long unsigned int timer = 0;
long unsigned int speedChange = 0;

bool speedUnit = false; // false is km/h, true is mph.
bool zeroVelocity = false;

bool startRecording = true;
bool buttonChange = false;
int recordingResolution = 2000;
unsigned long recordingTimer = 0;
unsigned long recordingTime = 0;
unsigned long recordLogoTimer = 0;
bool recordLogoState = true;

WebServer server(80);

Adafruit_MPU6050 mpu;

LiquidCrystal_I2C lcd(0x3F, 16, 2); // set the LCD address to 0x27 for SIM or 0x3F IRL for a 16 chars and 2 line display
File myFile;


byte recordStep1[] = {
  B00000,
  B11111,
  B10001,
  B10101,
  B10101,
  B10001,
  B11111,
  B00000
};

byte recordStep2[] = {
  B00000,
  B11111,
  B10001,
  B10001,
  B10001,
  B10001,
  B11111,
  B00000
};


void setup()
{
  Serial.begin(115200);
  displayMode = 0; // Default display mode is 0, AKA speed and distance
  rotation = 0;
  rpm = 0;
  prevTime = 0;

    WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }
  server.on("/", handleRoot);

  server.begin();
  Serial.println("HTTP server started");



  attachInterrupt(digitalPinToInterrupt(12), magnet_detect, RISING);

  lcd.init(); // Initialize the LCD
  pinMode(12, INPUT_PULLDOWN);
  pinMode(2, OUTPUT);
  // Configure our button pins to act as input pins, with the internal pullup resistor active
  pinMode(nextButton, INPUT_PULLUP);
  pinMode(prevButton, INPUT_PULLUP);
  pinMode(selButton, INPUT_PULLUP);

  lcd.createChar(0, recordStep1);
  lcd.createChar(1, recordStep2);

  lcd.backlight();
  lcd.clear();
  lcd.setCursor(1, 0);
  // Print a startup message to the LCD.
  lcd.print("Initializing...");
  lcd.setCursor(1, 1);
  lcd.print("System Active!");
  delay(3000);
  lcd.clear();

  //SD.begin(SD_CS);
  // if (!SD.begin(SD_CS))
  // {
  //   Serial.println("Card Mount Failed");
  //   return;
  // }
  // uint8_t cardType = SD.cardType();
  // if (cardType == CARD_NONE)
  // {
  //   Serial.println("No SD card attached");
  //   return;
  // }


  delay(1000);
  Serial.println("Initializing SD card...");
  if (!SD.begin(SD_CS))
  {
    Serial.println("ERROR - SD card initialization failed!");
    return; // init failed
  }
  Serial.println("SD card initialization done.");
  myFile = SD.open("/data.txt", FILE_WRITE);
  myFile.close();



  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");


  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  Serial.print("Accelerometer range set to: ");
  switch (mpu.getAccelerometerRange()) {
  case MPU6050_RANGE_2_G:
    Serial.println("+-2G");
    break;
  case MPU6050_RANGE_4_G:
    Serial.println("+-4G");
    break;
  case MPU6050_RANGE_8_G:
    Serial.println("+-8G");
    break;
  case MPU6050_RANGE_16_G:
    Serial.println("+-16G");
    break;
  }
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  Serial.print("Gyro range set to: ");
  switch (mpu.getGyroRange()) {
  case MPU6050_RANGE_250_DEG:
    Serial.println("+- 250 deg/s");
    break;
  case MPU6050_RANGE_500_DEG:
    Serial.println("+- 500 deg/s");
    break;
  case MPU6050_RANGE_1000_DEG:
    Serial.println("+- 1000 deg/s");
    break;
  case MPU6050_RANGE_2000_DEG:
    Serial.println("+- 2000 deg/s");
    break;
  }

  mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);
  Serial.print("Filter bandwidth set to: ");
  switch (mpu.getFilterBandwidth()) {
  case MPU6050_BAND_260_HZ:
    Serial.println("260 Hz");
    break;
  case MPU6050_BAND_184_HZ:
    Serial.println("184 Hz");
    break;
  case MPU6050_BAND_94_HZ:
    Serial.println("94 Hz");
    break;
  case MPU6050_BAND_44_HZ:
    Serial.println("44 Hz");
    break;
  case MPU6050_BAND_21_HZ:
    Serial.println("21 Hz");
    break;
  case MPU6050_BAND_10_HZ:
    Serial.println("10 Hz");
    break;
  case MPU6050_BAND_5_HZ:
    Serial.println("5 Hz");
    break;
  }

}

// void displayModeByButton()
// {
//   if (digitalRead(nextButton) == LOW)
//   {
//     displayMode++;
//     if (displayMode > 2)
//     {
//       displayMode = 0;
//     }
//   }
//   if (digitalRead(prevButton) == LOW)
//   {
//     displayMode--;
//     if (displayMode < 0)
//     {
//       displayMode = 2;
//     }
//   }
// }

void setButtonsState()
{
  nextButtonReading = digitalRead(nextButton);
  prevButtonReading = digitalRead(prevButton);
  selButtonReading = digitalRead(selButton);

  if (nextButtonReading != nextButtonLastState)
  {
    nextButtonLastDebounceTime = millis();
  }
  if (prevButtonReading != prevButtonLastState)
  {
    prevButtonLastDebounceTime = millis();
  }
  if (selButtonReading != selButtonLastState)
  {
    selButtonLastDebounceTime = millis();
  }

  if ((millis() - nextButtonLastDebounceTime) > debounceDelay)
  {
    if (nextButtonReading != nextButtonState)
    {
      nextButtonState = nextButtonReading;
      buttonChange = true;
    }
  }
  if ((millis() - prevButtonLastDebounceTime) > debounceDelay)
  {
    if (prevButtonReading != prevButtonState)
    {
      prevButtonState = prevButtonReading;
      buttonChange = true;
    }
  }
  if ((millis() - selButtonLastDebounceTime) > debounceDelay)
  {
    if (selButtonReading != selButtonState)
    {
      selButtonState = selButtonReading;
      buttonChange = true;
    }
  }
  

  nextButtonLastState = nextButtonReading;
  prevButtonLastState = prevButtonReading;
  selButtonLastState = selButtonReading;
}

void displayModeByButtonState()
{
  if (nextButtonState == LOW)
  {
    displayMode++;
    if (displayMode > 2)
    {
      displayMode = 0;
    }
    lcd.clear();
  }
  if (prevButtonState == LOW)
  {
    displayMode--;
    if (displayMode < 0)
    {
      displayMode = 2;
    }
    lcd.clear();
  }
}

void setSpeed()
{
  if (rotation >= 2)
  {
    timetaken = millis() - prevTime; // time in millisec
    rpm = (1000 / timetaken) * 60;
    prevTime = millis();
    rotation = 0;
    velocity = rpm * tireDiameter * 0.001 * 3.14159265358979323846264338327950 * 60; // ACTUAL CODE KM/hr
  }

  if (millis() - timer > 3000)
  {
    velocity = 0;
    zeroVelocity = false;
    timer = millis();
  }
}

void setDistance()
{
  if (velocity != lastVelocity)
  {
    lastDistance = totalDistance;
    totalDistance = totalDistance + (lastVelocity * speedChange / 1000);
    lastVelocity = velocity;
    speedChange = millis();
  }
}

void periodicLogToSD()
{
  myFile = SD.open("/data.txt", FILE_WRITE);
  if (myFile)
  {
    myFile.print(millis());
    myFile.print(",");
    myFile.print(velocity);
    myFile.print(",");
    myFile.print(totalDistance);
    myFile.print(",");
    myFile.print(bikeAngle);
    myFile.println();
    myFile.close();
  }
  myFile = SD.open("/data.txt");
  if (myFile)
  {
    Serial.print("SD: ");
    while (myFile.available()){
      Serial.print((char)myFile.read());
    }
    myFile.close();
  }
  else
  {
    Serial.println("error opening data.txt");
  }
}

void loop()
{
  server.handleClient();

  setButtonsState();

  if (selButtonState == LOW)
  {
    startRecording = !startRecording;
    if (startRecording)
    {
      recordingTimer = millis();
    }
  }

  if (startRecording)
  {
    if (millis() - recordingTimer > recordingResolution)
    {
      periodicLogToSD();
      recordingTimer = millis();
    }
  }

  if (buttonChange == true)
  {
    displayModeByButtonState();
    buttonChange = false;
  }
  if (displayMode != lastDisplayMode)
  {
    lcd.clear();
    lastDisplayMode = displayMode;
  }

  setSpeed();
  detectAngle();
  setDistance();

  switch (displayMode)
  {
  case 0:
    displaySpeed();
    break;
  case 1:
    displayDistance();
    break;
  case 2:
    displayUnderConstruction();
    break;
  }
}

void displayUnderConstruction(){
  lcd.setCursor(5, 0);
  lcd.print("Under");
  lcd.setCursor(2, 1);
  lcd.print("Construction!");
}

void displaySpeed()
{
  if ((velocity < 10 && lastVelocity > 9) || (velocity == 0 && zeroVelocity == false))
  {
    zeroVelocity = true;
    lcd.clear();
  }

  if (startRecording){
    if (millis() - recordLogoTimer > 500)
    {
      recordLogoTimer = millis();
      recordLogoState = !recordLogoState;
    }
    if (recordLogoState)
    {
      lcd.setCursor(0, 0);
      lcd.write(byte(0));
    }
    else
    {
      lcd.setCursor(0, 0);
      lcd.write(byte(1));
    }
  }

  lcd.setCursor(1, 0);
  lcd.print("Speed:");
  lcd.setCursor(8, 0);
  lcd.print(velocity);
  lcd.setCursor(11, 0);
  if (speedUnit == false)
  {
    lcd.print("km/h");
  }
  else
  {
    lcd.print("mph");
  }
}

void displayDistance()
{
  lcd.setCursor(1, 0);
  lcd.print("Distance:");
  lcd.setCursor(11, 0);
  lcd.print(totalDistance);
  lcd.setCursor(15, 0);
  lcd.print("m");
}



void magnet_detect() // Called whenever a magnet is detected
{
  timer = millis();
  rotation++;
  digitalWrite(2, HIGH);
  //Serial.println("Magnet detected");
}

void detectAngle() 
{
  /* Get new sensor events with the readings */
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  bikeAngle = g.gyro.x;
  if(g.gyro.x>=5){
    Serial.print("We're going up!");}
  else if(g.gyro.x<=-5){
    Serial.print("We're going down!");}
  else{
    Serial.print("We're not on a slope!");}
  Serial.println("");
}

void handleRoot() {
  char msg[1500];

  snprintf(msg, 1500,
           "<html>\
  <head>\
    <meta http-equiv='refresh' content='4'/>\
    <meta name='viewport' content='width=device-width, initial-scale=1'>\
    <link rel='stylesheet' href='https://use.fontawesome.com/releases/v5.7.2/css/all.css' integrity='sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr' crossorigin='anonymous'>\
    <title>Bicycle auxiliary system</title>\
    <style>\
    html { font-family: Arial; display: inline-block; margin: 0px auto; text-align: center;}\
    h2 { font-size: 3.0rem; }\
    p { font-size: 3.0rem; }\
    .units { font-size: 1.2rem; }\
    .dht-labels{ font-size: 1.5rem; vertical-align:middle; padding-bottom: 15px;}\
    </style>\
  </head>\
  <body>\
    <h2>Bicycle auxiliary system!</h2>\
      <p>\
      <span class='dht-labels'>Distance </span>\
        <span>%.2f</span>\
        <sup class='units'>km</sup>\
      </p>\
      <p>\
        <span class='dht-labels'>Speed </span>\
        <span>%.2f</span>\
        <sup class='units'>km/h</sup>\
        </p>\
        <p>\
        <span class='dht-labels'>Avg Speed </span>\
        <span>%.2f</span>\
        <sup class='units'>km/h</sup>\
      </p>\
  </body>\
</html>",
           totalDistance, velocity, velocity
          );
  server.send(200, "text/html", msg);
}
// void loop()
// {
//   if (millis() - dtime > 1000) //to drop down to zero when braked.
// 	velocity = (0.035) * rpm * 0.37699;  // ACTUAL CODE KM/hr
// 	lcd.setCursor(1,0);
// 	lcd.print("Speed:");
//   lcd.setCursor(8,0);
//   //lcd.setCursor(1,1);
//   //lcd.print(analogRead(13));
// }

// void magnet_detect()//Called whenever a magnet is detected
// {
//   lcd.setCursor(1,1);
//   lcd.print("Interrupt");
//   rotation++;
//   dtime = millis();
//   if (rotation >= 2)
//   {
// 	timetaken = millis() - prevTime; //time in millisec
// 	rpm = (1000 / timetaken) * 60;
// 	prevTime = millis();
// 	rotation = 0;
// 	Serial.write(velocity);
//   }
// }