// For ATMega324PA-PU
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdlib.h>

#define USART0_TX_BUF_LEN 128
#define USART1_TX_BUF_LEN 128

void initUSART();
//void initTimer();
void USART0_print(const char* str);
void USART1_print(const char* str);

struct {
    unsigned char buf[USART0_TX_BUF_LEN];
    uint8_t start;
    uint8_t end;
} USART0_TX_BUF;

struct {
    unsigned char buf[USART1_TX_BUF_LEN];
    uint8_t start;
    uint8_t end;
} USART1_TX_BUF;

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

int main()
{
  DDRD = 1 << PIND6;
  while(1) {
    PORTD ^= 1 << PIND6;
    _delay_ms(100);
    PORTD &= ~(0 << PIND6);
    _delay_ms(100);
  }
}

void initUSART()
{
    // Configure UART
    // Define baud rate
    int baud = 25;
    UBRR0H = (unsigned char) baud >> 8;
    UBRR1H = (unsigned char) baud >> 8;
    UBRR0L = (unsigned char) baud;
    UBRR1L = (unsigned char) baud;

    // Set async mode
    UCSR0C &= ~(1 << UMSEL00);
    UCSR1C &= ~(1 << UMSEL10);

    // Enable TX and RX
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);
    UCSR1B = (1 << RXEN1) | (1 << TXEN1);
    UCSR0C = (1<<USBS0) | (3<<UCSZ00);
    UCSR1C = (1<<USBS1) | (3<<UCSZ10);

    // Enable RX interrupts
    UCSR0B |= 1 << RXCIE0;
    UCSR1B |= 1 << RXCIE1;

    // UDRIE in UCSRA needs to be set to 0 according to documentation
    UCSR0A &= ~(1 << UDRIE0);
    UCSR1A &= ~(1 << UDRIE1);

    // Enable global interrupts
    SREG |= 1 << SREG_I;

    USART0_TX_BUF.start = 0;
    USART1_TX_BUF.start = 0;
    USART0_TX_BUF.end = 0;
    USART1_TX_BUF.end = 0;
    USART0_TX_BUF.buf[USART0_TX_BUF.start] = 0;
    USART1_TX_BUF.buf[USART1_TX_BUF.start] = 0;
    USART0_print("USART0 initialized.\n");
    USART1_print("USART1 initialized.\n");
}

/*
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
*/

ISR(USART0_RX_vect)
{
    unsigned char receivedData = UDR0;
    //blinkLed(1);
    // Wait while TX is ready
    while(! (UCSR0A & (1 << UDRE0)));
    UDR0 = receivedData;
    switch(receivedData) {
    case 13: // carriage returm
        while(! (UCSR0A & (1 << UDRE0)));
        UDR0 = 10; // send newline in case carriage return is received
        break;
    case 8: // backspace
        while(! (UCSR0A & (1 << UDRE0)));
        UDR0 = 127; // send newline in case carriage return is received
        break;
    default:
        break;
    }
}

ISR(USART1_RX_vect)
{
    unsigned char receivedData = UDR1;
    //blinkLed(1);
    // Wait while TX is ready
    while(! (UCSR1A & (1 << UDRE1)));
    // TODO: remove it  ?
    UDR1 = receivedData;
    switch(receivedData) {
    case 13: // carriage returm
        while(! (UCSR1A & (1 << UDRE1)));
        UDR1 = 10; // send newline in case carriage return is received
        break;
    case 8: // backspace
        while(! (UCSR1A & (1 << UDRE1)));
        UDR1 = 127; // send newline in case carriage return is received
        break;
    default:
        break;
    }
}

ISR(USART0_UDRE_vect)
{
    ++USART0_TX_BUF.end;
    if(USART0_TX_BUF.end >= USART0_TX_BUF_LEN)
        USART0_TX_BUF.end = 0;
    if(USART0_TX_BUF.end != USART0_TX_BUF.start) {
        UDR0 = USART0_TX_BUF.buf[USART0_TX_BUF.end];
    } else {
        UCSR0B &= ~(1 << UDRIE0);
    }
}

ISR(USART1_UDRE_vect)
{
    ++USART1_TX_BUF.end;
    if(USART1_TX_BUF.end >= USART1_TX_BUF_LEN)
        USART1_TX_BUF.end = 0;
    if(USART1_TX_BUF.end != USART1_TX_BUF.start) {
        UDR1 = USART1_TX_BUF.buf[USART1_TX_BUF.end];
    } else {
        UCSR1B &= ~(1 << UDRIE1);
    }
}


// TODO: fix to macro
void USART0_print(const char* str)
{
    while(*str) {
        USART0_TX_BUF.buf[USART0_TX_BUF.start] = *str;
        ++USART0_TX_BUF.start;
        if(USART0_TX_BUF.start >= USART0_TX_BUF_LEN)
            USART0_TX_BUF.start = 0;
        if(*str == 10) {
            USART0_TX_BUF.buf[USART0_TX_BUF.start] = 13;
            ++USART0_TX_BUF.start;
            if(USART0_TX_BUF.start >= USART0_TX_BUF_LEN)
                USART0_TX_BUF.start = 0;
        }
        ++str;
    }

    if(! (UCSR0B & (1 << UDRIE0))) {
        // Wwait until UDR is empty
        while(! (UCSR0A & (1 << UDRE0)));
        UDR0 = USART0_TX_BUF.buf[USART0_TX_BUF.end];
        // Enable UDRE interrupt
        UCSR0B |= 1 << UDRIE0;
    }
}

void USART1_print(const char* str)
{
    while(*str) {
        USART1_TX_BUF.buf[USART1_TX_BUF.start] = *str;
        ++USART1_TX_BUF.start;
        if(USART1_TX_BUF.start >= USART1_TX_BUF_LEN)
            USART1_TX_BUF.start = 0;
        if(*str == 10) {
            USART1_TX_BUF.buf[USART1_TX_BUF.start] = 13;
            ++USART1_TX_BUF.start;
            if(USART1_TX_BUF.start >= USART1_TX_BUF_LEN)
                USART1_TX_BUF.start = 0;
        }
        ++str;
    }

    if(! (UCSR1B & (1 << UDRIE1))) {
        // Wwait until UDR is empty
        while(! (UCSR1A & (1 << UDRE1)));
        UDR1 = USART1_TX_BUF.buf[USART1_TX_BUF.end];
        // Enable UDRE interrupt
        UCSR1B |= 1 << UDRIE1;
    }
}
