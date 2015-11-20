#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdlib.h>

#define USART_TX_BUF_LEN 128

void initUSART();
void initTimer();
void USART_print(const char* str);

struct {
    unsigned char buf[USART_TX_BUF_LEN];
    uint8_t start;
    uint8_t end;
} USART_TX_BUF;

char tmp_str[6];
uint32_t count = 0;

void blinkLed(int times)
{
    while(times) {
        PORTB ^= (1 << PINB0);
        _delay_ms(50);
        PORTB ^= (1 << PINB0);
        _delay_ms(50);
        --times;
    }
}

void main()
{
    // Pin to measure impulse time
    //DDRB |= (0 << PINB0);
    //PORTB = (0 << PINB0);
    // LED pin
    DDRD |= (1 << PIND7);
    PORTD |= (0 << PIND7);

    // Trig pin
    PORTD |= (1 << PIND6);
    DDRD |= (1 << PIND6);

    blinkLed(5);
    initUSART();
    initTimer();

    USART_print("Main started\n");

    while(1) {
        PORTD |= 1 << PIND6;
        _delay_us(12);
        PORTD &= ~(1 << PIND6);
        _delay_ms(2000);
    }
}

void initUSART()
{
    // Configure UART
    // Define baud rate
    int baud = 25;
    UBRRH = (unsigned char) baud >> 8;
    UBRRL = (unsigned char) baud;

    // Set async mode
    UCSRC &= ~(1 << UMSEL);

    // Enable TX and RX
    UCSRB = (1 << RXEN) | (1 << TXEN);
    UCSRC = (1<<URSEL) | (1<<USBS) | (3<<UCSZ0);

    // Enable RX interrupts
    UCSRB |= 1 << RXCIE;

    // UDRIE in UCSRA needs to be set to 0 according to documentation
    UCSRA &= ~(1 << UDRIE);

    // Enable global interrupts
    SREG |= 1 << SREG_I;

    USART_TX_BUF.start = 0;
    USART_TX_BUF.end = 0;
    USART_TX_BUF.buf[USART_TX_BUF.start] = 0;
    USART_print("USART initialized.\n");
}

void initTimer()
{
    // Enable interrupts
    SREG |= 1 << SREG_I;
    
    // Set prescaler
    //TCCR1B |= (1 << CS10) | (1 << CS11);
    TCCR1B |= 1<<CS10;

    // Set timer mode
    // Set CTC mode
    //TCCR1B |= 1 << WGM12;

    // Set compare number
    //OCR1A = 15624;

    // Enable timer interrupts
    //TIMSK |= 1 << OCIE1A;
    TIMSK |= 1 << TICIE1;

    // Activate Input Capture Noise Canceler
    TCCR1B |= 1 << ICNC1;
    // Set rising edge detect
    TCCR1B |= 1 << ICES1;

    USART_print("Timer initialized\n");
}

ISR(USART_RXC_vect)
{
    unsigned char receivedData = UDR;
    //blinkLed(1);
    // Wait while TX is ready
    while(! (UCSRA & (1 << UDRE)));
    UDR = receivedData;
    switch(receivedData) {
    case 13: // carriage returm
        while(! (UCSRA & (1 << UDRE)));
        UDR = 10; // send newline in case carriage return is received
        break;
    case 8: // backspace
        while(! (UCSRA & (1 << UDRE)));
        UDR = 127; // send newline in case carriage return is received
        break;
    default:
        break;
    }
}

ISR(USART_UDRE_vect)
{
    ++USART_TX_BUF.end;
    if(USART_TX_BUF.end >= USART_TX_BUF_LEN)
        USART_TX_BUF.end = 0;
    if(USART_TX_BUF.end != USART_TX_BUF.start) {
        UDR = USART_TX_BUF.buf[USART_TX_BUF.end];
    } else {
        UCSRB &= ~(1 << UDRIE);
    }
}

/*
ISR(TIMER1_COMPA_vect)
{
    ++count;
    PORTD ^= 1 << PIND7;
    itoa(count, tmp_str, 10);
    USART_print("Timer tick: " );
    USART_print(tmp_str);
    USART_print("\n");
}
*/

/*
ISR(TIMER1_CAPT_vect)
{
    // Disable interrupts
    PORTD ^= 1 << PIND7;
    char cm[5];
    cli();
    uint16_t tmp = ICR1;
    // Enable interrupts
    sei();
    if(TCCR1B & (1 << ICES1)) {
        // It is rising edge
        TCNT1 = 0;
        // Set interrupt for failing edge
        TCCR1B &= ~(1 << ICES1);
    } else {
        uint8_t result = tmp / 58;
        USART_print("Result: ");
        USART_print(itoa(result, cm, 10));
        USART_print(" cm.\n");
        TCCR1B |= 1 << ICES1;
    }
}
*/

void USART_print(const char* str)
{
    while(*str) {
        USART_TX_BUF.buf[USART_TX_BUF.start] = *str;
        ++USART_TX_BUF.start;
        if(USART_TX_BUF.start >= USART_TX_BUF_LEN)
            USART_TX_BUF.start = 0;
        if(*str == 10) {
            USART_TX_BUF.buf[USART_TX_BUF.start] = 13;
            ++USART_TX_BUF.start;
            if(USART_TX_BUF.start >= USART_TX_BUF_LEN)
                USART_TX_BUF.start = 0;
        }
        ++str;
    }

    if(! (UCSRB & (1 << UDRIE))) {
        // Wwait until UDR is empty
        while(! (UCSRA & (1 << UDRE)));
        UDR = USART_TX_BUF.buf[USART_TX_BUF.end];
        // Enable UDRE interrupt
        UCSRB |= 1 << UDRIE;
    }
}

// TICIE1 = 1 -- enable Input Capture interrupt
// when event comes to ICR pin, ICF1 flag is set, TCNT1 is written to ICR1 register
//
