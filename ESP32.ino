#include <Wire.h>
#include <LiquidCrystal_I2C.h>

volatile byte rotation;
float timetaken;
float rpm;
float dtime;
int velocity;
unsigned long pevtime;


LiquidCrystal_I2C lcd(0x3F, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line display

void setup()
{
  Serial.begin(115200);

  rotation = 0;
  rpm = 0;
  pevtime = 0;

  attachInterrupt(0, magnet_detect, RISING);

  lcd.init();                      // initialize the lcd
  lcd.init();
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


void loop()
{
  if (millis() - dtime > 1000) //to drop down to zero when braked.
	velocity = (0.035) * rpm * 0.37699;  // ACTUAL CODE KM/hr
	lcd.setCursor(1,0);
	lcd.print("Speed:" + velocity);

}


void magnet_detect()//Called whenever a magnet is detected
{
  rotation++;
  dtime = millis();
  if (rotation >= 2)
  {
	timetaken = millis() - pevtime; //time in millisec
	rpm = (1000 / timetaken) * 60;
	pevtime = millis();
	rotation = 0;
	Serial.write(velocity);
  }
}
