/*
Author: Ido Morel

Code built for ESP32-Wroom-32

This is the code for our final year project under the Electronics program.
---------------------------------------------------------------------------
TODO:
Implement Wifi Capabillities
*/
#include "esp_system.h"

#define SDSPI   // Didn't know what SD module we have so I put a selection here
#define SD_CS 5 // SD card chip select pin

#ifdef SDSPI

#include <SD.h>
#include <SPI.h>
#include <FS.h>

#endif

// At first we include the needed libraries into our program
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define tireDiameter 0.05 // The diameter of the tire in meters

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



  SD.begin(SD_CS);
  if (!SD.begin(SD_CS))
  {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE)
  {
    Serial.println("No SD card attached");
    return;
  }
  Serial.println("Initializing SD card...");
  if (!SD.begin(SD_CS))
  {
    Serial.println("ERROR - SD card initialization failed!");
    return; // init failed
  }
  Serial.println("SD card initialization done.");
  myFile = SD.open("/data.txt", FILE_WRITE);
  myFile.close();

  lcd.backlight();
  lcd.clear();
  lcd.setCursor(1, 0);
  // Print a startup message to the LCD.
  lcd.print("Initializing...");
  lcd.setCursor(1, 1);
  lcd.print("System Active!");
  delay(3000);
  lcd.clear();
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
    velocity = rpm * tireDiameter * 0.001 * 3.14159265358979323846264338327950 * 2 * 60; // ACTUAL CODE KM/hr
    //velocity = (0.035) * rpm * 0.37699; // ACTUAL CODE KM/hr
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