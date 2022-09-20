#include <Wire.h>
#include <LiquidCrystal_I2C.h>

volatile byte rotation;
float timetaken;
float rpm;
float dtime;
int velocity = 0;
unsigned long pevtime = 0;
long unsigned int timer = 0; 


LiquidCrystal_I2C lcd(0x3F, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line display

void setup()
{
  Serial.begin(115200);

  rotation = 0;
  rpm = 0;
  pevtime = 0;

  attachInterrupt(13, magnet_detect, RISING);

  lcd.init();
  pinMode(13, INPUT);                      // initialize the lcd
  //lcd.init();
  // Print a message to the LCD.
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("Initializing...");
  lcd.setCursor(1, 1);
  lcd.print("System Active!");
  delay(3000);
  lcd.clear();
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

void loop()
{
//to drop down to zero when braked.
//lcd.clear();

dtime = millis();
if (rotation >= 2)
{
timetaken = millis() - pevtime; //time in millisec
rpm = (1000 / timetaken) * 60;
pevtime = millis();
rotation = 0;

Serial.write(velocity);
}
velocity = (0.035) * rpm * 0.37699;  // ACTUAL CODE KM/hr
Serial.println(timer);
lcd.setCursor(1,0);
lcd.print("Speed:");
lcd.setCursor(8,0);
if (millis() - timer > 2000){
  velocity = 0;
  lcd.clear();

} 
lcd.print(velocity);
}


// void magnet_detect()//Called whenever a magnet is detected
// {
//   lcd.setCursor(1,1);
//   lcd.print("Interrupt");
//   rotation++;
//   dtime = millis();
//   if (rotation >= 2)
//   {
// 	timetaken = millis() - pevtime; //time in millisec
// 	rpm = (1000 / timetaken) * 60;
// 	pevtime = millis();
// 	rotation = 0;
// 	Serial.write(velocity);
//   }
// }

void magnet_detect()//Called whenever a magnet is detected
{
timer = millis();
rotation++;
}
