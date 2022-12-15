#include<avr/io.h>
#include<avr/interrupt.h>
#include<util/delay.h>

double dutycycle = 0;

int main(){

  Serial.begin(9600);
  DDRB = _BV(PORTB1);

  TCCR1A = _BV(COM1A1) | _BV(WGM12) | _BV(WGM10);
  TIMSK1 = _BV(TOIE1);
  
  OCR1A= 2273;
  TCCR1B = _BV(FOC1A);
  sei();
  
  TCCR1B = _BV(CS11);  
  
  while(1){

    Serial.println(dutycycle);
    
    _delay_ms(100);

    dutycycle += 10;

    if(dutycycle > 100)
    {
      dutycycle = 0;
    }
    
  }
}

ISR(TIMER1_OVF_vect){

  OCR1A= (dutycycle/100)*255;
  
}
