#include<avr/io.h>
#include<avr/interrupt.h>

int dutycycle=0;

void ADC_setup(){

  ADMUX = _BV(REFS0);
  ADCSRA = _BV(ADEN) | _BV(ADIE)| _BV(ADPS0) | _BV(ADPS1) | _BV(ADPS2);
  DIDR0 = _BV(ADC0D);

  start_conversion();
}

void start_conversion(){

  ADCSRA |= _BV(ADSC);
}
int main(){

  ADC_setup();
  sei();
  Serial.begin(9600);

  while(1){
    
  }
}

ISR(ADC_vect){

  dutycycle = ADC;
  Serial.println(dutycycle);
  start_conversion();
}
