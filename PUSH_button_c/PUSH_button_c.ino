#include<avr/io.h>

int main(void){
  
  DDRB = 0x00;
  PORTB = 0x01;

  Serial.begin(9600);
  while(1)
  {
  if(!(PINB & (0b00000001)))
   {
    Serial.println("HI");
    //delay(100);
    }
  }
}
