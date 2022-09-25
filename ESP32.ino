/*
Author: Ido Morel

Code built for ESP32-Wroom-32

This is the code for our final year project under the Electronics program.
---------------------------------------------------------------------------
TODO:
Print speed to display
*/


// At first we include the needed libraries into our program
#include <Wire.h>
#include <LiquidCrystal_I2C.h>


//Then we define some pin values.
//We use #define because it is helpful for coding during development
//and only used by the compiler on the computer so it doesn't take space on our microcontroller.

#define nextButton 99    //The pin number for the button used as the "next page" button
#define prevButton 99    //The pin number for the button used as the "previous page" button
#define selButton 99    //The pin number for the button used as the "Select" button


volatile byte rotation;
float timetaken;
float rpm;
//float dtime;
int velocity = 0;
int displayMode = 0;
unsigned long prevTime = 0;
long unsigned int timer = 0; 


LiquidCrystal_I2C lcd(0x3F, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line display

void setup()
{
  Serial.begin(115200);
  displayMode = 0; //Default display mode is 0
  rotation = 0;
  rpm = 0;
  prevTime = 0;

  attachInterrupt(13, magnet_detect, RISING);

  lcd.init(); // Initialize the LCD
  pinMode(13, INPUT);
  //Configure our button pins to act as input pins, with the internal pullup resistor active
  pinMode(nextButton, INPUT_PULLUP);
  pinMode(prevButton, INPUT_PULLUP);
  pinMode(selButton, INPUT_PULLUP);

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




void loop()
{
  
  //dtime = millis();
  if (rotation >= 2)
  {
    timetaken = millis() - prevTime; //time in millisec
    rpm = (1000 / timetaken) * 60;
    prevTime = millis();
    rotation = 0;
  }
  velocity = (0.035) * rpm * 0.37699;  // ACTUAL CODE KM/hr
  switch (displayMode)
  
  lcd.setCursor(1,0);
  lcd.print("Speed:");
  lcd.setCursor(8,0);
  if (millis() - timer > 3000){
    //lcd.clear();
    velocity = 0;
  } 
  
  
}


void magnet_detect()//Called whenever a magnet is detected
{
  timer = millis();
  rotation++;
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