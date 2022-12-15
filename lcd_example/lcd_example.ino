#include <math.h>
#include <stdlib.h>
#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <compat/twi.h>
#include <stdint.h>


#define I2C_READ    1
#define I2C_WRITE   0

#define I2C_LCD_BACKLIGHT   8
#define I2C_LCD_ENABLE      4
#define I2C_LCD_RW          2
#define I2C_LCD_RS          1

char hazard[7] = {'H','A','Z','A','R','D'};
char state_0[11] = {'S','t','a','t','e','_','0',':','B','S'};
char state_1[11] = {'S','t','a','t','e','_','1',':','L','S'};
char state_2[15] = {'S','t','a','t','e','_','2',':','P','E','D','E','S','T'};
char state_3[12] = {'S','t','a','t','e','_','3',':','B','S','T'};

void setup_I2C() {
    DDRC &= 0x0f;
    PORTC |= 0x30;  // Port C pins 4 and 5 set to input with pullups
    TWBR = 193;   //193
    TWSR = 0;
    TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA);
} 

int I2C_Start() {
    // Send I2C Start flag
    TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN);
    I2C_wait();
    return ((TWSR & 0xf8) == 0x08);
}

int I2C_SLA(uint8_t addr, uint8_t rw) {
    // Send I2C slave address
    TWDR = (addr << 1) | (rw & 1);
    TWCR = _BV(TWINT) | _BV(TWEN);
    I2C_wait();
    return ((TWSR & 0xf8) == 0x18);
}

int I2C_PCF8574_LCD_Byte(uint8_t data, uint8_t flags) {
    return I2C_PCF8574_LCD_Nibble ((data & 0xf0) | (flags & 0x0f)) && 
    I2C_PCF8574_LCD_Nibble (((data << 4) & 0xf0) | (flags & 0x0f));
}

int I2C_PCF8574_LCD_Nibble(uint8_t data) {
    TWDR = data | 0x04;
    TWCR = _BV(TWINT) | _BV(TWEN);
    I2C_wait();
    if ((TWSR & 0xf8) == 0x28) {
        TWDR = data & (~0x04);
        TWCR = _BV(TWINT) | _BV(TWEN);
        I2C_wait();
    }
    return ((TWSR & 0xf8) == 0x28);
}

int8_t LCD_PCF8574_Setup(uint8_t addr) {
    if (!(I2C_Start() && I2C_SLA(addr, I2C_WRITE))) {
        return -1;
    }
    I2C_Send(0);    // ensure the PCF8574 enable line is low
    I2C_PCF8574_LCD_Nibble(0x30);
    I2C_PCF8574_LCD_Nibble(0x30);
    I2C_PCF8574_LCD_Nibble(0x30);
    I2C_PCF8574_LCD_Nibble(0x20);
    I2C_PCF8574_LCD_Byte(0x0C, 0x08);   // display on, cursor on and blinking
    I2C_PCF8574_LCD_Byte(0x01, 0x08);   // clear and move home
    I2C_Stop();
    return 0;
}

int I2C_Send(uint8_t data) {
    // Send I2C data byte
    TWDR = data;
    TWCR = _BV(TWINT) | _BV(TWEN);
    I2C_wait();
    return ((TWSR & 0xf8) == 0x28);
}

void I2C_Stop() {
    // Send I2C Stop flag
    TWCR = _BV(TWINT) | _BV(TWSTO) | _BV(TWEN);
      for (volatile long x = 0; x < 100; x++) {
      }
      return 1  ;
}

void I2C_wait() {
    while ((TWCR & _BV(TWINT)) == 0) {
    }
}

int8_t LCD_Position(uint8_t addr, uint8_t posn) {
    if (!(I2C_Start() && I2C_SLA(addr, I2C_WRITE))) {
        return -1;
    }
    I2C_PCF8574_LCD_Byte(0x80 | posn, 0x08);   // set DRAM address
    I2C_Stop();
    return 0;
}

int8_t LCD_string(uint8_t addr, char *str, uint8_t len) {
    if (!(I2C_Start() && I2C_SLA(addr, I2C_WRITE))) {
        return -1;
    }
    while (len--) {
        I2C_PCF8574_LCD_Byte(*str++, I2C_LCD_BACKLIGHT | I2C_LCD_RS);
    }
    I2C_Stop();
    return 0;
}
int main () {
  setup_I2C();
  LCD_PCF8574_Setup(0x3f);
  
  while(1){

    LCD_Position(0x3f,0x00);
    LCD_string(0x3f, hazard,6);

    _delay_ms(1000);

    I2C_Start();
    I2C_SLA(0x3f,I2C_WRITE);
    I2C_PCF8574_LCD_Byte(0x01,0x08);
    I2C_Stop();
    
    LCD_Position(0x3f,0x00);
    LCD_string(0x3f, state_0,10);

    _delay_ms(1000);

    I2C_Start();
    I2C_SLA(0x3f,I2C_WRITE);
    I2C_PCF8574_LCD_Byte(0x01,0x08);
    I2C_Stop();

    LCD_Position(0x3f,0x00);
    LCD_string(0x3f, state_1,10);

    _delay_ms(1000);

    I2C_Start();
    I2C_SLA(0x3f,I2C_WRITE);
    I2C_PCF8574_LCD_Byte(0x01,0x08);
    I2C_Stop();

    LCD_Position(0x3f,0x00);
    LCD_string(0x3f, state_2,14);

    _delay_ms(1000);

    I2C_Start();
    I2C_SLA(0x3f,I2C_WRITE);
    I2C_PCF8574_LCD_Byte(0x01,0x08);
    I2C_Stop();

    LCD_Position(0x3f,0x00);
    LCD_string(0x3f, state_3,11);

    _delay_ms(1000);
    
    I2C_Start();
    I2C_SLA(0x3f,I2C_WRITE);
    I2C_PCF8574_LCD_Byte(0x01,0x08);
    I2C_Stop();
  }
}
