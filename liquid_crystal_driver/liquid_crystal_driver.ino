#include <math.h>
#include <stdlib.h>
#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <compat/twi.h>
#include <Wire.h>


//**************************************************************//
//                        I2C_driver_APIs                       //
//                         & Definitions                        //
//*************************************************************//

//*** I2C_definitions  ***//

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif

#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif


#define TWI_BUFFER_LENGTH 32
#define TWI_FREQ 100000L

#define TWI_READY 0
#define TWI_MRX   1
#define TWI_MTX   2
#define TWI_SRX   3
#define TWI_STX   4

  
static volatile uint8_t twi_state;
static volatile uint8_t twi_slarw;
static volatile uint8_t twi_sendStop;      // should the transaction end with a stop
static volatile uint8_t twi_inRepStart;

static uint8_t txAddress;
static uint8_t txBuffer[TWI_BUFFER_LENGTH];
static uint8_t txBufferIndex;
static uint8_t txBufferLength;


//*** I2C_APIs  ***//

void twi_init(void)
{
  txBufferIndex = 0;
  txBufferLength = 0;
  
  // initialize state
  twi_state = TWI_READY;
  twi_sendStop = true;    // default value
  twi_inRepStart = false;
  
  // activate internal pullups for twi.
  digitalWrite(SDA, 1);
  digitalWrite(SCL, 1);

  // initialize twi prescaler and bit rate
  cbi(TWSR, TWPS0);
  cbi(TWSR, TWPS1);
  TWBR = ((F_CPU / TWI_FREQ) - 16) / 2;

  /* twi bit rate formula from atmega128 manual pg 204
  SCL Frequency = CPU Clock Frequency / (16 + (2 * TWBR))
  note: TWBR should be 10 or higher for master mode
  It is 72 for a 16mhz Wiring board with 100kHz TWI */

  // enable twi module, acks, and twi interrupt
  TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA);
}

//**************************************************************//
//                        Liquid_crystal_APIs                   //
//                           & definitions                      //
//*************************************************************//

//*** LCD_definitions  ***//

//commands
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

// flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

// flags for backlight control
#define LCD_BACKLIGHT 0x08
#define LCD_NOBACKLIGHT 0x00

#define En B00000100  // Enable bit
#define Rw B00000010  // Read/Write bit
#define Rs B00000001  // Register select bit

uint8_t  _addr = 0x3F;
uint8_t _cols = 16;
uint8_t  _rows = 2;
//uint8_t  _charsize = charsize;
uint8_t  _backlightval = LCD_BACKLIGHT;

uint8_t _displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
uint8_t _displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;



int main (void){
  twi_init();
  LCD_begin();
  
  while(1){
  LCD_blink();
  delay(500);
  LCD_noBlink();
  delay(500);
    }
  
  
}

void LCD_begin(){
  
  uint8_t _displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;
  
  _displayfunction |= LCD_2LINE;
  delay(50);
  
  expanderWrite(_backlightval);  // reset expanderand turn backlight off (Bit 8 =1)
  delay(1000);

  expanderWrite(_backlightval);  // reset expanderand turn backlight off (Bit 8 =1)
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
  LCD_display();

  // clear it off
  LCD_clear();

  // Initialize to default text direction (for roman languages)
  uint8_t _displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;

  // set the entry mode
  command(LCD_ENTRYMODESET | _displaymode);

  LCD_home();
  }

void LCD_clear(){
  command(LCD_CLEARDISPLAY);// clear display, set cursor position to zero
  delayMicroseconds(2000);  // this command takes a long time!
}

void LCD_home(){
  command(LCD_RETURNHOME);  // set cursor position to zero
  delayMicroseconds(2000);  // this command takes a long time!
}

void LCD_display() {
  _displaycontrol |= LCD_DISPLAYON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}
inline void command(uint8_t value) {
  LCD_send(value, 0);
}

int LCD_write(uint8_t value) {
  LCD_send(value, Rs);
  return 1;
}

void LCD_send(uint8_t value, uint8_t mode) {
  uint8_t highnib=value&0xf0;
  uint8_t lownib=(value<<4)&0xf0;
  write4bits((highnib)|mode);
  write4bits((lownib)|mode);
}

void write4bits(uint8_t value) {
  expanderWrite(value);
  pulseEnable(value);
}

void expanderWrite(uint8_t _data){
  Wire.beginTransmission(_addr);
  Wire.write((int)(_data) | _backlightval);
  Wire.endTransmission();
}

void pulseEnable(uint8_t _data){
  expanderWrite(_data | En);  // En high
  delayMicroseconds(1);   // enable pulse must be >450ns

  expanderWrite(_data & ~En); // En low
  delayMicroseconds(50);    // commands need > 37us to settle
}

void LCD_blink() {
  _displaycontrol |= LCD_BLINKON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void LCD_backlight(void) {
  _backlightval=LCD_BACKLIGHT;
  expanderWrite(0);
}

void LCD_noBacklight(void) {
  _backlightval=LCD_NOBACKLIGHT;
  expanderWrite(0);
}
void LCD_noBlink() {
  _displaycontrol &= ~LCD_BLINKON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}
