#include "LQ.h"
#include <inttypes.h>
#include <Arduino.h>
#include <Wire.h>

// When the display powers up, it is configured as follows:
//
// 1. Display clear
// 2. Function set:
//    DL = 1; 8-bit interface data
//    N = 0; 1-line display
//    F = 0; 5x8 dot character font
// 3. Display on/off control:
//    D = 0; Display off
//    C = 0; Cursor off
//    B = 0; Blinking off
// 4. Entry mode set:
//    I/D = 1; Increment by 1
//    S = 0; No shift
//
// Note, however, that resetting the Arduino doesn't reset the LCD, so we
// can't assume that its in that state when a sketch starts (and the
// LiquidCrystal constructor is called).

LQ::LQ(uint8_t lcd_addr, uint8_t lcd_cols, uint8_t lcd_rows, uint8_t charsize)
{
  _addr = lcd_addr;
  _cols = lcd_cols;
  _rows = lcd_rows;
  _charsize = charsize;
  _backlightval = LCD_BACKLIGHT;
}

void LQ::begin() {
  Wire.begin();
  _displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;

  if (_rows > 1) {
    _displayfunction |= LCD_2LINE;
  }

  // for some 1 line displays you can select a 10 pixel high font
  if ((_charsize != 0) && (_rows == 1)) {
    _displayfunction |= LCD_5x10DOTS;
  }

  // SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
  // according to datasheet, we need at least 40ms after power rises above 2.7V
  // before sending commands. Arduino can turn on way befer 4.5V so we'll wait 50
  delay(50);

  // Now we pull both RS and R/W low to begin commands
  expanderWrite(_backlightval);	// reset expanderand turn backlight off (Bit 8 =1)
  delay(1000);

  //put the LCD into 4 bit mode
  // this is according to the hitachi HD44780 datasheet
  // figure 24, pg 46

  // we start in 8bit mode, try to set 4 bit mode
  write4bits(0x03 << 4);
  delayMicroseconds(4500); // wait min 4.1ms

  // second try
  write4bits(0x03 << 4);
  delayMicroseconds(4500); // wait min 4.1ms

  // third go!
  write4bits(0x03 << 4);
  delayMicroseconds(150);

  // finally, set to 4-bit interface
  write4bits(0x02 << 4);

  // set # lines, font size, etc.
  command(LCD_FUNCTIONSET | _displayfunction);

  // turn the display on with no cursor or blinking default
  _displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
  display();

  // clear it off
  clear();

  // Initialize to default text direction (for roman languages)
  _displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;

  // set the entry mode
  command(LCD_ENTRYMODESET | _displaymode);

  home();
  //AddHebrowCharacters();
}

/********** high level commands, for the user! */
void LQ::clear() {
  command(LCD_CLEARDISPLAY);// clear display, set cursor position to zero
  delayMicroseconds(2000);  // this command takes a long time!
}

void LQ::home() {
  command(LCD_RETURNHOME);  // set cursor position to zero
  delayMicroseconds(2000);  // this command takes a long time!
}

void LQ::setCursor(uint8_t col, uint8_t row) {
  int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
  if (row > _rows) {
    row = _rows - 1;  // we count rows starting w/0
  }
  command(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

// Turn the display on/off (quickly)
void LQ::noDisplay() {
  _displaycontrol &= ~LCD_DISPLAYON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LQ::display() {
  _displaycontrol |= LCD_DISPLAYON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turns the underline cursor on/off
void LQ::noCursor() {
  _displaycontrol &= ~LCD_CURSORON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LQ::cursor() {
  _displaycontrol |= LCD_CURSORON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turn on and off the blinking cursor
void LQ::noBlink() {
  _displaycontrol &= ~LCD_BLINKON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LQ::blink() {
  _displaycontrol |= LCD_BLINKON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// These commands scroll the display without changing the RAM
void LQ::scrollDisplayLeft(void) {
  command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}
void LQ::scrollDisplayRight(void) {
  command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

// This is for text that flows Left to Right
void LQ::leftToRight(void) {
  _displaymode |= LCD_ENTRYLEFT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// This is for text that flows Right to Left
void LQ::rightToLeft(void) {
  _displaymode &= ~LCD_ENTRYLEFT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'right justify' text from the cursor
void LQ::autoscroll(void) {
  _displaymode |= LCD_ENTRYSHIFTINCREMENT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'left justify' text from the cursor
void LQ::noAutoscroll(void) {
  _displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// Allows us to fill the first 8 CGRAM locations
// with custom characters
void LQ::createChar(uint8_t location, uint8_t charmap[]) {
  location &= 0x7; // we only have 8 locations 0-7
  command(LCD_SETCGRAMADDR | (location << 3));
  for (int i = 0; i < 8; i++) {
    write(charmap[i]);
  }
}

// Turn the (optional) backlight off/on
void LQ::noBacklight(void) {
  _backlightval = LCD_NOBACKLIGHT;
  expanderWrite(0);
}

void LQ::backlight(void) {
  _backlightval = LCD_BACKLIGHT;
  expanderWrite(0);
}
bool LQ::getBacklight() {
  return _backlightval == LCD_BACKLIGHT;
}


/*********** mid level commands, for sending data/cmds */

inline void LQ::command(uint8_t value) {
  send(value, 0);
}

inline size_t LQ::write(uint8_t value) {
  send(value, Rs);
  return 1;
}


/************ low level data pushing commands **********/

// write either command or data
void LQ::send(uint8_t value, uint8_t mode) {
  uint8_t highnib = value & 0xf0;
  uint8_t lownib = (value << 4) & 0xf0;
  write4bits((highnib) | mode);
  write4bits((lownib) | mode);
}

void LQ::write4bits(uint8_t value) {
  expanderWrite(value);
  pulseEnable(value);
}

void LQ::expanderWrite(uint8_t _data) {
  Wire.beginTransmission(_addr);
  Wire.write((int)(_data) | _backlightval);
  Wire.endTransmission();
}

void LQ::pulseEnable(uint8_t _data) {
  expanderWrite(_data | En);	// En high
  delayMicroseconds(1);		// enable pulse must be >450ns

  expanderWrite(_data & ~En);	// En low
  delayMicroseconds(50);		// commands need > 37us to settle
}

void LQ::load_custom_character(uint8_t char_num, uint8_t *rows) {
  createChar(char_num, rows);
}

void LQ::setBacklight(uint8_t new_val) {
  if (new_val) {
    backlight();		// turn backlight on
  } else {
    noBacklight();		// turn backlight off
  }
}

void LQ::printstr(const char c[]) {
  //This function is not identical to the function used for "real" I2C displays
  //it's here so the user sketch doesn't have to be changed
  print(c);
}

void LQ::PrintString(String text)
{
  clear();
  setCursor(0, 0);
  print(text);
  if (text.length() > _cols)
  {
    int textLength = text.length() - 1;
    for (int i = _cols; i <= textLength; ++i)
    {
      setCursor(i - _cols, 1);
      print(text[i]);
    }
  }
}

void LQ::AddHebrowCharacters()
{
  /*?????????? ????????????:
    for(int i = 0; i < 27; ++i)
    {
      createChar(i, HebrewCharecters[i]);
    }
  */
}

void LQ::PrintHebrowString(String text)
{
  byte ThisChar;
  for (int i = 0; i < 16 && i <= text.length(); ++i)
  {
    //setCursor(16 - i, 0);
    switch(text[i])
    {
      int X = 0;
      case '??': ThisChar = HebrewCharecters[0]; break;
      case '??': ThisChar = HebrewCharecters[1]; break;
      case '??': ThisChar = HebrewCharecters[2]; break;
      case '??': ThisChar = HebrewCharecters[3]; break;
      case '??': ThisChar = HebrewCharecters[4]; break;
      case '??': ThisChar = HebrewCharecters[5]; break;
      case '??': ThisChar = HebrewCharecters[6]; break;
      case '??': ThisChar = HebrewCharecters[7]; break;
      case '??': ThisChar = HebrewCharecters[8]; break;
      case '??': ThisChar = HebrewCharecters[9]; break;
      case '??': ThisChar = HebrewCharecters[10]; break;
      case '??': ThisChar = HebrewCharecters[11]; break;
      case '??': ThisChar = HebrewCharecters[12]; break;
      case '??': ThisChar = HebrewCharecters[13]; break;
      case '??': ThisChar = HebrewCharecters[14]; break;
      case '??': ThisChar = HebrewCharecters[15]; break;
      case '??': ThisChar = HebrewCharecters[16]; break;
      case '??': ThisChar = HebrewCharecters[17]; break;
      case '??': ThisChar = HebrewCharecters[18]; break;
      case '??': ThisChar = HebrewCharecters[19]; break;
      case '??': ThisChar = HebrewCharecters[20]; break;
      case '??': ThisChar = HebrewCharecters[21]; break;
      case '??': ThisChar = HebrewCharecters[22]; break;
      case '??': ThisChar = HebrewCharecters[23]; break;
      case '??': ThisChar = HebrewCharecters[24]; break;
      case '??': ThisChar = HebrewCharecters[25]; break;
      case '??': ThisChar = HebrewCharecters[26]; break;
      break;
    }
    WriteCostumChar(ThisChar, 16 - i, 1);
  }
}


static byte LQ::HebrewCharecters[27][8] = {
  {
    B10010,
    B10010,
    B01010,
    B00100,
    B01100,
    B10010,
    B10001,
    B10001
  },
  {
    B00000,
    B00000,
    B11110,
    B00010,
    B00010,
    B00010,
    B00010,
    B11111
  },
  {
    B00000,
    B00000,
    B11110,
    B00010,
    B00010,
    B01110,
    B10010,
    B10010
  },
  {
    B00000,
    B00000,
    B11111,
    B00010,
    B00010,
    B00010,
    B00010,
    B00010
  },
  {
    B00000,
    B00000,
    B11111,
    B00001,
    B00001,
    B10001,
    B10001,
    B10001
  },
  {
    B00000,
    B00000,
    B01100,
    B00100,
    B00100,
    B00100,
    B00100,
    B00100
  },
  {
    B00000,
    B10000,
    B01110,
    B00101,
    B00100,
    B01000,
    B01000,
    B01000
  },
  {
    B00000,
    B00000,
    B11111,
    B10001,
    B10001,
    B10001,
    B10001,
    B10001
  },
  {
    B00000,
    B00000,
    B10011,
    B10101,
    B10001,
    B10001,
    B10001,
    B11111
  }, {
    B00000,
    B00000,
    B01110,
    B00010,
    B00010,
    B00000,
    B00000,
    B00000
  }, {
    B00000,
    B00000,
    B11110,
    B00001,
    B00001,
    B00001,
    B00001,
    B11110
  }, {
    B10000,
    B10000,
    B11111,
    B00001,
    B00010,
    B00010,
    B00100,
    B01000
  }, {
    B00000,
    B00000,
    B10110,
    B01001,
    B10001,
    B10001,
    B10001,
    B10111
  }, {
    B00000,
    B00000,
    B00111,
    B00001,
    B00001,
    B00001,
    B00001,
    B11111
  }, {
    B00000,
    B00000,
    B11111,
    B10001,
    B10001,
    B10001,
    B10001,
    B01110
  }, {
    B00000,
    B00000,
    B00101,
    B00101,
    B00101,
    B00101,
    B00101,
    B11111
  }, {
    B00000,
    B00000,
    B00000,
    B11110,
    B10010,
    B11010,
    B00010,
    B11110
  }, {
    B00000,
    B00000,
    B10001,
    B01010,
    B00100,
    B00010,
    B00001,
    B11111
  }, {
    B00000,
    B11111,
    B00001,
    B00001,
    B10001,
    B10001,
    B10000,
    B10000
  }, {
    B00000,
    B00000,
    B11111,
    B00001,
    B00001,
    B00001,
    B00001,
    B00001
  }, {
    B00000,
    B00000,
    B10101,
    B10101,
    B10101,
    B10101,
    B10101,
    B01110
  }, {
    B00000,
    B00000,
    B01111,
    B01001,
    B01001,
    B01001,
    B01001,
    B11001
  }, {
    B11110,
    B00010,
    B00010,
    B00010,
    B00010,
    B00010,
    B00010,
    B00010
  }, {
    B00000,
    B00000,
    B11111,
    B01001,
    B01001,
    B01001,
    B01001,
    B01111
  }, {
    B00110,
    B00010,
    B00010,
    B00010,
    B00010,
    B00010,
    B00010,
    B00010
  }, {
    B11110,
    B10010,
    B11010,
    B00010,
    B00010,
    B00010,
    B00010,
    B00000
  }, {
    B10001,
    B10001,
    B01010,
    B00100,
    B00010,
    B00010,
    B00001,
    B00001
  }
};

bool LQ::storeThisHebrowLetterInCharcterOne_OrReturnAFalseIfItsNotAHebrowLetter(char letter)
{

  byte tost[] = {B10001, B10001, B01010, B00100, B00010, B00010, B00001, B00001};
  createChar(0, tost);
  return (true);
}


void LQ::WriteCostumChar(byte strature[8], int X, int Y)
{
  createChar(1, strature);
  setCursor(X, Y);
  write(1);
}

void LQ::printHebrowLetter(int letter, int X, int Y)
{
  WriteCostumChar(HebrewCharecters[letter - 1], X, Y);
}

void LQ::PrintHebrowArryOfInts(int text[])
{/*
  byte thisChar[sizeof(text)];
  for(int i = 0; i < sizeof(text); ++i)
  {
   thisChar = HebrewCharecters[text[i]];
   WriteCostumChar(thisChar, 16 - i, 0);
  }
  */
}
