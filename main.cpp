#include <avr/io.h>
#include <util/delay.h>

void turn_leds(uint8_t val);


int main(void)
{
    DDRB |= 1<<PINB0;
    PORTB = 0x00;
    //TCCR1B =
    while(1)
    {
        for(int i = 0; i < 5; ++i)
        {
            PORTC = PORTC << 1;
            PORTC |= 0b00000001;
            _delay_ms(50);
        }
        _delay_ms(250);
        for(int i = 0; i < 5; ++i)
        {
            PORTC = PORTC >> 1;
            _delay_ms(50);
        }
        _delay_ms(250);
    }
}
