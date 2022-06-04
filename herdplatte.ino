#include <Arduino.h>
#include <max6675.h>
#include <LiquidCrystal_I2C.h>

const int thermo_so_pin = 4;
const int thermo_cs_pin = 3;
const int thermo_sck_pin = 2;
const int relay_pin = 5;
const int button_pin = 6;

const int stepDelay = 200;

const int numberOfSteps = 2;
String stepNameList[numberOfSteps] = {"Preheat", "Reflow"};
int tempCurve[numberOfSteps][2] = {{20, 80}, // {Time, Temp} Zeit in s
                                   {20, 150}};

bool enabled = false;
int currentTemp;
int hotspotCompensation = 20;
int temperatureOvershoot = 60;

float totalTime = 0;

MAX6675 thermocouple(thermo_sck_pin, thermo_cs_pin, thermo_so_pin);
LiquidCrystal_I2C lcd(0x3F, 16, 2);

void setup()
{
  pinMode(relay_pin, OUTPUT);
  digitalWrite(relay_pin, LOW);

  pinMode(button_pin, INPUT_PULLUP);

  Serial.begin(9600);
  
  displayStats(0, false);

  for (int i = 1; i < numberOfSteps; i++)
    tempCurve[i][0] += tempCurve[i - 1][0];
}

void loop()
{
reset:
  while (!enabled)
  {
    currentTemp = thermocouple.readCelsius() + hotspotCompensation;

    checkButton();
    displayStats(0, false);
  }

  for (int i = 0; i < numberOfSteps; i++)
  {
    currentTemp = thermocouple.readCelsius() + hotspotCompensation;
    int targetTemp = tempCurve[i][1] - temperatureOvershoot;

    while (currentTemp < targetTemp) // heating step
    {
      currentTemp = thermocouple.readCelsius() + hotspotCompensation;
      digitalWrite(relay_pin, HIGH);
      countDelay(stepDelay);

      checkButton();
      displayStats(i, true);

      if (!enabled)
      {
        break;
      }
    }

    digitalWrite(relay_pin, LOW);

    for (int j = 0; j < tempCurve[i][0] * 1000 / stepDelay; j++) // waiting step
    {
      countDelay(stepDelay);
      currentTemp = thermocouple.readCelsius() + hotspotCompensation;

      checkButton();
      displayStats(i, false);

      if (!enabled)
      {
        break;
      }
    }
  }
  enabled = false;
}

void countDelay(int t)
{
  delay(t);
  totalTime += t / 1000;
}

void displayStats(int stepIndex, bool heating)
{
  lcd.clear();

  currentTemp = thermocouple.readCelsius() + hotspotCompensation;

  String buttonMessage;
  switch (enabled)
  {
  case true:
  {
    buttonMessage = "stop";
    break;
  }
  case false:
  {
    buttonMessage = "start";
    break;
  }
  }

  String heatingMessage;
  switch (heating)
  {
  case true:
  {
    heatingMessage = "Heating";
    break;
  }
  case false:
  {
    heatingMessage = "Waiting";
    break;
  }
  }

  lcd.setCursor(0, 0);
  lcd.print(stepNameList[stepIndex]);

  lcd.setCursor(9, 0);
  lcd.print(heatingMessage);

  lcd.setCursor(0, 1);
  lcd.print((int)currentTemp);
  lcd.print((char)223); //° zeichen
  lcd.print("C");

  lcd.setCursor(11, 1);
  lcd.print(buttonMessage);
}

void checkButton()
{
  if (digitalRead(button_pin) == LOW)
  {
    enabled = !enabled;
  }
  countDelay(100);
}