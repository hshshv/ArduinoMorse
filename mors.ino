#include "LQ.h"

#define speakerPin 11
#define transTone 300

#define buzzerPin 10

#define dotTime 200
#define dashTime 600
#define delayBetweenSigns 200
#define delayBetweenLetters 800
#define delayBetweenWords 2000
#define delayBeforStart 1000

LQ myLcd(0x3F, 16, 2);

class mors
{
  private:
    void(*onFunc)();
    void(*offFunc)();
    void blnk(int duration)
    {
      onFunc();
      delay(duration);
      offFunc();
      delay(delayBetweenSigns);
    }
    dot()
    {
      blnk(dotTime);
    }
    dash()
    {
      blnk(dashTime);
    }
    dot(int times)
    {
      for (int i = 0; i < times; ++i)
      {
        dot();
      }
    }
    dash(int times)
    {
      for (int i = 0; i < times; ++i)
      {
        dash();
      }
    }
  public:
    void transChar(const char &chr)
    {
      switch (chr)
      {
        case 'a': dot(); dash(); break;
        case 'b': dash(); dot(3); break;
        case 'c': dash(); dot(); dash(); dot(); break;
        case 'd': dash(); dot(2); break;
        case 'e': dot(); break;
        case 'f': dot(2); dash(); dot(); break;
        case 'g': dash(2); dot(); break;
        case 'h': dot(4); break;
        case 'i': dot(2); break;
        case 'j': dot(); dash(3); break;
        case 'k': dash(); dot(); dash(); break;
        case 'l': dot(); dash(); dot(2); break;
        case 'm': dash(2); break;
        case 'n': dash(); dot(); break;
        case 'o': dash(3); break;
        case 'p': dot(); dash(2); dot(); break;
        case 'q': dash(2); dot(); dash(); break;
        case 'r':  dot(); dash(); dot(); break;
        case 's': dot(3); break;
        case 't': dash(); break;
        case 'u': dot(2); dash(); break;
        case 'v': dot(3); dash(); break;
        case 'w': dot(); dash(2); break;
        case 'x': dash(); dot(2); dash(); break;
        case 'y': dash(); dot(); dash(2); break;
        case 'z': dash(2); dot(2); break;
        case '0': dash(5); break;
        case '1': dot(); dash(4); break;
        case '2': dot(2); dash(3); break;
        case '3': dot(3); dash(2); break;
        case '4': dot(4); dash(1); break;
        case '5': dot(5); break;
        case '6': dash(); dot(4); break;
        case '7': dash(2); dot(3); break;
        case '8': dash(3); dot(2); break;
        case '9': dash(4); dot(); break;
          break;
      }
      delay(delayBetweenLetters);
    }
    mors(void(*onPtr)(), void(*offPtr)())
    {
      onFunc = onPtr;
      offFunc = offPtr;
    }

    transmit(const String& txt)
    {
      offFunc();
      delay(delayBeforStart);
      for (int i = 0; i < txt.length(); ++i)
      {
        if (txt[i] == ' ')
        {
          delay(delayBetweenWords);
          continue;
        }
        transChar(txt[i]);
      }
    }
};

void turnOn()
{
  myLcd.backlight();
  tone(speakerPin, transTone);
  //digitalWrite(buzzerPin, HIGH);
}
void turnOff()
{
  myLcd.noBacklight();
  noTone(speakerPin);
  //digitalWrite(buzzerPin, LOW);
}

mors trans = mors(turnOn, turnOff);

String txt = "hello world";

void setup()
{
  pinMode(speakerPin, OUTPUT);
  myLcd.begin();
  myLcd.clear();
  myLcd.print("open monitor");
  Serial.begin(9600);
}
char ltr = 'b';
String messege = "";
void loop()
{
  myLcd.clear();
  myLcd.setCursor(0, 0);
  myLcd.print("open monitor");
  myLcd.backlight();

  Serial.println("enter text to send in morse: ");
  while (!Serial.available()) {}
  messege = Serial.readString();
  messege.toLowerCase();
  Serial.println(messege);

  myLcd.clear();
  myLcd.setCursor(0, 0);
  myLcd.print(messege);
  myLcd.setCursor(0, 1);
  myLcd.print("in morse code");

  trans.transmit(messege);

  Serial.print("done. ");
}
