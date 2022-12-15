#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdint.h>
#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>
#include <math.h>
#include <inttypes.h>

#ifndef F_CPU   
#define F_CPU   16000000UL
#endif


#define I2C_READ    1
#define I2C_WRITE   0

#define I2C_LCD_BACKLIGHT   8
#define I2C_LCD_ENABLE      4
#define I2C_LCD_RW          2
#define I2C_LCD_RS          1

#define _addr             0x3f

void BCG(void);
void BST(void);
void LS(void);
void PD(void);


void PWM_setup(void);
void PWM_start(void);

void ADC_setup(void);
void start_conversion(void);

void setup_I2C(void);
int I2C_Start(void);
int I2C_SLA(uint8_t , uint8_t);
int I2C_Send(uint8_t );
int I2C_Stop(void);
void I2C_wait(void);

int I2C_PCF8574_LCD_Byte(uint8_t , uint8_t );
uint8_t I2C_PCF8574_LCD_Nibble(uint8_t);
int8_t LCD_PCF8574_Setup(uint8_t);
uint8_t LCD_Position(uint8_t , uint8_t );
void clear_LCD(void);
uint8_t LCD_string(uint8_t , char* , uint8_t );


char hazard[7] = {'H','A','Z','A','R','D'};
char state_0[11] = {'S','t','a','t','e','_','0',':','B','S'};
char state_1[11] = {'S','t','a','t','e','_','1',':','L','S'};
char state_2[15] = {'S','t','a','t','e','_','2',':','P','E','D','E','S','T'};
char state_3[12] = {'S','t','a','t','e','_','3',':','B','S','T'};


int i;
int dutycycle=0;


int main(void) {
    
    int i;
    
    setup_Ports();
    setup_SPI();
    setup_interrupts();
    ADC_setup();
    PWM_setup();
    setup_I2C();
    LCD_PCF8574_Setup(_addr);  
    sei();
    
    
    while(1){
      
     //BCG();


      if(!(PIND & (0b10000000))){  // S3 is held
        BCG();
      }
       if(!(PIND & (0b00010000))){ //PD S5
        //BCG();
        PD();
      }
      if(!((PIND) & (0b00001000))){ //HAZARD
        LCD_Position(_addr,0x00);
        LCD_string(_addr, hazard,6);
        while(!((PIND) & (0b00001000))){
          SPI_Send_Command(0x14,0b01000100);
          SPI_Send_Command(0x15,0b00100010);
          PORTD |= (1<<5);
          _delay_ms(1000);
          //delay_ms(300);
          SPI_Send_Command(0x14,0b00000000);
          SPI_Send_Command(0x15,0b00000000);
          PORTD &= ~(1<<5);
          _delay_ms(1000);
          //delay_ms(300);
          }
        i=0;
        while(i<=5){
          SPI_Send_Command(0x14,0b01000100);
          SPI_Send_Command(0x15,0b00100010);
          PORTD |= (1<<5);
          _delay_ms(1000);
         // delay_ms(300);
          SPI_Send_Command(0x14,0b00000000);
          SPI_Send_Command(0x15,0b00000000);
          PORTD &= ~(1<<5);
          _delay_ms(1000);
          //delay_ms(300);
          i++;
        }
        clear_LCD();
        }
        
 
      }
}

void setup_Ports() {
    DDRD  = 0b01100010; // allow UART to still be default
    PORTD = 0b10011001; // turn on pullups for all inputs
    DDRB  = 0b00101111; // define inputs and outputs
    PORTB = 0b00000100; // set SS lines to high
    DDRC  = 0b00101100; // setup LEDs as outputs and rest as inputs
    PORTC = 0b00110011; // do NOT turn on pullup for analog input
}

int8_t SPI_transfer(int8_t data) {
    
    SPDR = data;
    while ((SPSR & _BV(SPIF)) == 0) {
        ;   // wait until transfer completed
    }
    return SPDR;
}

void SPI_Send_Command(uint8_t reg, uint8_t data) {
    // Send a command + byte to SPI interface
    // SS enabled (low))
    PORTB &= ~_BV(2); 
        
    SPI_transfer(0x40);
    SPI_transfer(reg);
    SPI_transfer(data);
    
    // SS disabled (high)
    PORTB |= _BV(2);
}

uint8_t SPI_Read_Command( uint8_t reg) {
    uint8_t data;
    
    // Read a command output from SPI interface
    // SS enabled (low))
    PORTB &= ~_BV(2);    
    
    SPI_transfer(0x41);
    SPI_transfer(reg);
    data = SPI_transfer(0);
    
    // SS disabled (high)
    PORTB |= _BV(2);    
    
    return data;
}

void setup_SPI() {
  SPCR = _BV(SPE)|_BV(MSTR);   // set master SPI, SPI mode 0 operation
  SPSR = 0;                    // clear interrupt flags and oscillator mode.

    // Now that the SPI interface is configured we need to send SPI commands to configure the MCP23S17
    // port expander IC
    // Configure Expander

    SPI_Send_Command(0x0A, 0x40);   // register IOCONA (port A data direction)
    SPI_Send_Command(0x00, 0x11);   // register IODIRA (port A data direction)
    SPI_Send_Command(0x01, 0x11);   // register IODIRB (port B data direction)
    SPI_Send_Command(0x0c, 0x11);   // register GPPUA (port A GPIO Pullups)
    SPI_Send_Command(0x0d, 0x11);   // register GPPUB (port B GPIO Pullups)
    SPI_Send_Command(0x04, 0x11);   // register GPINTENA (enable port A interrupts)
    SPI_Send_Command(0x05, 0x11);   // register GPINTENB (enable port B interrupts)
    SPI_Send_Command(0x08, 0x00);   // register INTCONA (port A interrupt on change)
    SPI_Send_Command(0x09, 0x00);   // register INTCONB (port B interrupt on change)
}
void setup_interrupts() {
    EICRA = 0b00000001; // set interrupt to occur on logical change
    EIMSK = 0b00000001; // enable interrupt 0 & 1
}

void BCG(void){
  
           LCD_Position(_addr,0x00);
           LCD_string(_addr, state_0,10);
           SPI_Send_Command(0x14,0b00001000);
           SPI_Send_Command(0x15,0b10000000);
           PORTB &= ~(1<<0);
           
           _delay_ms(8000);

          if (((~SPI_Read_Command(0x13) & 0b00010000) == 0b00010000)||((~SPI_Read_Command(0x12) & 0b00000001) == 0b00000001)||(((~PIND) & 0b10000000)== 0b10000000)) {
          
              _delay_ms(3000);//delay_ms(300);
            }
            
           SPI_Send_Command(0x14,0b00000100);
           SPI_Send_Command(0x15,0b01000000);
           
           _delay_ms(3000);
           
           SPI_Send_Command(0x14,0b00000010);
           SPI_Send_Command(0x15,0b00100000);
           PORTB |= (1<<0);  
           
           _delay_ms(3000);
           
          clear_LCD();
  
  }

  
void BST(void){
         // i=0;
          //while(i<2){
          LCD_Position(_addr,0x00);
          LCD_string(_addr, state_3,11);
           PORTB |= (1<<0);
           SPI_Send_Command(0x14,0b10001000);
           SPI_Send_Command(0x15,0b00100000);
           //delay_ms(300);
           _delay_ms(2000);
           //i++;
            //}
            if((~SPI_Read_Command(0x12) & 0b00010000) == 0b00010000){
              //while(i<4){
                _delay_ms(4000);//delay_ms(300);
           //i++;}
            }
          // i=0;
        //  while(i<3){
           //PORTB |= (1<<0);
           SPI_Send_Command(0x14,0b01000100);
           //SPI_Send_Command(0x13,0b0010 0000);
           //delay_ms(300);
           _delay_ms(3000);
           //i++;
            //}i=0;
           //while(i<3){
           SPI_Send_Command(0x14,0b00100010);
           PORTB |= (1<<0);
           //delay_ms(300);
           _delay_ms(3000);
           //i++;}
           clear_LCD();
  
  }    

  void LS(void){
    LCD_Position(_addr,0x00);
    LCD_string(_addr, state_1,10);
//    i=0;
//    while(i<3){
           SPI_Send_Command(0x14,0b00000000);
           SPI_Send_Command(0x15,0b00001000);
           //delay_ms(300);
           _delay_ms(3000);
          // i++;
           //}
           if((~SPI_Read_Command(0x13) & 0b00000001) == 0b00000001){
            //while(i<5){
            _delay_ms(5000); //delay_ms(300);
            //i++;}
            }
           //i=0;
          //while(i<3){           
           SPI_Send_Command(0x15,0b00000100);
           //delay_ms(300);
           _delay_ms(3000);
           //i++;
          // }//i=0;
           //while(i<3){
           SPI_Send_Command(0x15,0b00000010);
           //delay_ms(300);
           _delay_ms(3000);
           //i++;
           //}
           clear_LCD();
  }
    

void PD (void) {
   //i=0;
    LCD_Position(_addr,0x00);
    LCD_string(_addr, state_2,14);
    
            PORTD |= (1<<6);
            PWM_start();
            for(int i=0; i<=70; i++){
              _delay_ms(100);
              dutycycle+=15;
              }
              TCCR1B &= ~_BV(CS11);

            PORTD &= ~(1<<6);

            _delay_ms(3000);

            PORTD &= ~(1<<6);
            PORTD |= (1<<5);

           _delay_ms(3000);

           clear_LCD();
  }


void ADC_setup(){

  ADMUX = _BV(REFS0);
  ADCSRA = _BV(ADEN) | _BV(ADIE)| _BV(ADPS0) | _BV(ADPS1) | _BV(ADPS2);
  DIDR0 = _BV(ADC0D);

  start_conversion();
}

void start_conversion(){

  ADCSRA |= _BV(ADSC);
}

void PWM_setup(){
  
  DDRB |= _BV(PORTB1);

  TCCR1A = _BV(COM1A1) | _BV(WGM12) | _BV(WGM10);
  TIMSK1 = _BV(TOIE1);
  
  //OCR1A= 2273;
  TCCR1B = _BV(FOC1A);

}

void PWM_start(){

  TCCR1B = _BV(CS11);  
}


//***************//
//              I2C_APIs                       //
//***************// 
 
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

int I2C_Send(uint8_t data) {
    // Send I2C data byte
    TWDR = data;
    TWCR = _BV(TWINT) | _BV(TWEN);
    I2C_wait();
    return ((TWSR & 0xf8) == 0x28);
}

int I2C_Stop() {
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



//***************//
//              LCD_APIs                       //
//***************//

int I2C_PCF8574_LCD_Byte(uint8_t data, uint8_t flags) {
    return I2C_PCF8574_LCD_Nibble ((data & 0xf0) | (flags & 0x0f)) && 
    I2C_PCF8574_LCD_Nibble (((data << 4) & 0xf0) | (flags & 0x0f));
}

uint8_t I2C_PCF8574_LCD_Nibble(uint8_t data) {
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


uint8_t LCD_Position(uint8_t addr, uint8_t posn) {
    if (!(I2C_Start() && I2C_SLA(addr, I2C_WRITE))) {
        return -1;
    }
    I2C_PCF8574_LCD_Byte(0x80 | posn, 0x08);   // set DRAM address
    I2C_Stop();
    return 0;
}

void clear_LCD(){

  I2C_Start();
  I2C_SLA(_addr,I2C_WRITE);
  I2C_PCF8574_LCD_Byte(0x01,0x08);
  I2C_Stop();
}

uint8_t LCD_string(uint8_t addr, char *str, uint8_t len) {
    if (!(I2C_Start() && I2C_SLA(addr, I2C_WRITE))) {
        return -1;
    }
    while (len--) {
        I2C_PCF8574_LCD_Byte(*str++, I2C_LCD_BACKLIGHT | I2C_LCD_RS);
    }
    I2C_Stop();
    return 0;
}

ISR(INT0_vect) {
    uint8_t data1 = ~SPI_Read_Command(0x13);
    uint8_t data2 = ~SPI_Read_Command(0x12);

    if (((data1 & 0b00010000) == 0b00010000)||((data2 & 0b00000001) == 0b00000001)||(((~PIND) & 0b10000000)== 0b10000000)) {// S0 is held
          while(1){
            BCG();
          if(((SPI_Read_Command(0x12) & 0b00000001) == 0b00000001)){break;};
          }
    }

    if((data2 & 0b00010000) == 0b00010000){// S1 is held
          while(1){
            BCG();
            BST();
            
         if((SPI_Read_Command(0x12) & 0b00010000) == 0b00010000){break;}

          }
          }
           
    if(((data1 & 0b00000001) == 0b00000001)&&(!((PIND) & 0b00010000))){ //When S2+S5 is held
    while(1){
      LS();
      PD();
      //i=0;
        //  while(i<8){
           SPI_Send_Command(0x14,0b00001000);
           SPI_Send_Command(0x15,0b10000000);
        
           _delay_ms(8000);
//           i++;
//           }
          if (((~SPI_Read_Command(0x13) & 0b00010000) == 0b00010000)||((~SPI_Read_Command(0x12) & 0b00000001) == 0b00000001)||(((~PIND) & 0b10000000)== 0b10000000)) {
           // while(i<10){
           _delay_ms(3000);
           //i++;}
            }//i=0;
           // while(i<3){
           SPI_Send_Command(0x14,0b00000100);
           SPI_Send_Command(0x15,0b01000000);
           _delay_ms(3000);
           //i++;
           //}
          // i=0;
           //while(i<3){
           SPI_Send_Command(0x14,0b00000010);
           SPI_Send_Command(0x15,0b00100000);
            
           
           _delay_ms(3000);
           //i++;
          //}
     if(((SPI_Read_Command(0x12) & 0b00000001) == 0b00000001)&&(((PIND) & 0b00010000)== 0b00010000)){break;}
      
      }
    }
     
    if((data1 & 0b00000001) == 0b00000001){//LS When S2 is held
      while(1){ 
      BCG();
      LS();

      if((SPI_Read_Command(0x13) & 0b00000001) == 0b00000001){ break;}
      }
    }
    if((data1 & 0b00010000) == 0b00010000){  // S4 is held
      while(1){
        BCG();
        if((SPI_Read_Command(0x13) & 0b00010000) == 0b00010000){break;}
        }

      }
}

ISR(ADC_vect){

  dutycycle = ADC;
  start_conversion();
}

ISR(TIMER1_OVF_vect){

  OCR1A= (dutycycle/100)*255;
  //OCR1A= (dutycycle/100)*255;
  
}
