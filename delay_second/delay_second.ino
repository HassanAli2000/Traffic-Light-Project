#include<avr/io.h>
#include<avr/interrupt.h>

uint8_t count=0;

int main(){

  Serial.begin(9600);
  
  while(1){
  Serial.println("HI");  
  timer0_setup();
  }
}


void timer0_setup(){
  
  
    
  TCCR0A = _BV(WGM01);    //configuring timer0 to CTC(capture_compare_output) mode
  OCR0A  = 255;           //Max value to count up to
  TIMSK0 = _BV(OCIE0A);   //Enabling the output capture interrupt

  sei();

  TCCR0B = _BV(CS01) | _BV(CS00);   //prescaling the timer clock by the factor 64
  
  while(count>=1000)
  {
    noInterrupts();
    TIMSK0 &= ~_BV(OCIE0A);
    count=0;
  }
}

ISR(TIMER0_COMPA_vect){

  count++;
  
}
