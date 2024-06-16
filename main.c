#include <msp430.h>
#include <math.h>
// Krisjanis Ivbulis 191RMC097
/**
 * main.c
 */

void interrupt_init()
{
    // Atljaut globalos interupts
    __bis_SR_register(GIE);
}

void timer_init()
{
    // Stop timer before changing register
    // SMCLK source used by peripherals on Vcc = 3.3 is 16MHz
    // 16e6 ticks per second  62.5 nanoseconds per tick
    // 1e6 nanoseconds in milisecond -> nanoseconds in milisecond (1e6)/1 tick time in miliseconds (0.000625ms
    // 16000 ticks in 1 milisecond
    // Timer need to count 16000 times, to reach 1 milisecond
    // Timer single cycle will be 16000/8 (clock divide) 2000 times per ms)
    // TIMER A control register = TA source sel (SMCLK), (IDx = 3, divider 8), MC_1 count up to value in TACCR0 value SET WHEN START TIMER,

    TACTL |= TASSEL_2; // Select clock source
    TACCR0 = 0;        // Clear register
    TACCTL0 |= CCIE;   // Enable interrupt for CCR0.
}

void delay_ms(unsigned int ms)
{
    if (ms == 0)
        return;
    // 2000 timer counts is 1ms if clock is 16MHz, divider 8
    unsigned int count = ms * 2000 - 1;
    TACCR0 = count;
    TACTL |= TASSEL_2 | ID_3 | MC_1; // Set clock divider and mode of operaion (count up to TACCR0), clear counter
    // Counts till 2000 timer cycles enables interrupt
}

// Ir iespeeja veikt multiple cahnnels, continous conversion un tad nevajag uzsaakt ADC moduli katru reizi.
// Saja gadijuma, vienkarsibas deelj paliek Single channel, single conversion
// Ieksejais temperaturas sensors
int temp_internal()
{
    // Vr = VRef vr = vss (peec sleeguma sheemas)
    ADC10CTL0 = (SREF_1 | REFON | ADC10SHT_3 | ADC10SR | ADC10ON);

    // Channel for internal temp sensor (1010)
    ADC10CTL1 = INCH_10;

    int t = 0;

    delay_ms(1);

    // Enable conversion and Start conversion
    ADC10CTL0 |= ENC | ADC10SC;
    // While adc is busy
    while (ADC10CTL1 & BUSY)
        ;
    // Store conversion
    t = ADC10MEM;
    // Disable conversion// as is single conversion, not needed
    ADC10CTL0 &= ~ENC;

    // Convert to c
    // int ir 4baiti. saglabata vertiba ir 10 biti, kas ieklaujas int datu tipa
    //>>10 ir liidziigs kaa /1023, bet nenjem veera noapalosanu un ir aatraak.
    int intDegC = ((t - 673) * 423) >> 10;

    return intDegC;
}

// Steinhart-Hart koeficineti un formula
int steinhart_coeff(float adc_raw)
{
    // Works dont touch.
    const float B = 0.00022524621886;
    const float A = 0.00115821021574;
    const float G = 0.00000084708606;
    const float D = 0.000000063171671;

    // 10kO reference resistor
    const float r_ref = 10000.0;

    const float v_max = 3.3;                    // 3.3V
    const float v = (adc_raw * v_max) / 1023.0; // actual voltage

    const float R = r_ref * (v / (v_max - v)); // resistance value

    float log1 = logf(R);
    float log2 = powf(log1, 2);
    float log3 = powf(log1, 3);

    // long steinhart-hart equation
    //     const float T = 1.0 / (A + (B * logf(R)) + G * powf(logf(R), 2)+ D * powf(logf(R), 3));

    const float T = 1.0 / (A + B * log1 + G * log2);

    const float T_c = T - 273.15;

    return (int)T_c;
}

// Arejais temperaturas sensors
int temp_periph()
{
    // A0  - input channel 0
    // SREF0, Vr ir Vcc un VR ir Vss
    ADC10CTL0 = (SREF_0 | REFON | ADC10SHT_3 | ADC10SR | ADC10ON);

    ADC10CTL1 = INCH_0;

    int t = 0;
    delay_ms(1);
    // Enable conversion and Start conversion
    ADC10CTL0 |= ENC | ADC10SC;
    // While adc is busy
    while (ADC10CTL1 & BUSY)
        ;
    // Store conversion
    t = ADC10MEM;
    // Disable conversion
    ADC10CTL0 &= ~ENC;
    // convert to c
    int deg_c = steinhart_coeff(t);

    return deg_c;
}

int main(void)
{
    // Lai nenotiktu MCU resets kad beidzas taimeris.
    WDTCTL = WDTPW | WDTHOLD; // stop watchdog timer

    // Reset p1out
    P1OUT = 0U;
    interrupt_init();
    timer_init();

    volatile int i_temp = 0;
    volatile int p_temp = 0;

    while (1)
    {
        delay_ms(1);
        i_temp = temp_internal();
        delay_ms(1);
        p_temp = temp_periph();
        delay_ms(1);
    }

    return 0;
}

#pragma vector = TIMERA0_VECTOR
__interrupt void TIMERA0_VECTOR_ISR(void)
{
    TACTL = MC_0; // Stop timer
    TACCTL0 &= ~CCIFG;
    TACTL |= TACLR; // Clear counter
}
