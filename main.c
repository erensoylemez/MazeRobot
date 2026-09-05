#include <msp430.h>
#include "Afficheur.h"
#include "ADC.h"
#include <stdbool.h>

#define CRITICAL_THRESHOLD 750
#define SIDE_THRESHOLD     350
#define PWM_PERIOD         1000
#define PWM_DUTY_A         650
#define PWM_DUTY_B         560
#define PWM_DUTY_A_FWD     540
#define COUNTER_THRESHOLD  45
#define DRIFT_DEADBAND     30

void pwm_init()
{
    TA1CTL = TASSEL_2 | MC_1 | TACLR;
    TA1CCR0 = PWM_PERIOD - 1;
    TA1CCR1 = PWM_DUTY_A;
    TA1CCTL1 = OUTMOD_7;
    TA1CCR2 = PWM_DUTY_B;
    TA1CCTL2 = OUTMOD_7;
    P2SEL |= (BIT2 + BIT4);
    P2DIR |= (BIT2 + BIT4);
}

void pwm_stop()
{
    P2SEL &= ~(BIT2 + BIT4);
    P2OUT &= ~(BIT2 + BIT4);
}

void pwm_start()
{
    P2SEL |= (BIT2 + BIT4);
}

void move_forward()
{
    TA1CCR1 = PWM_DUTY_A_FWD;
    TA1CCR2 = PWM_DUTY_B;
    pwm_start();
    P2OUT &= ~BIT1;
    P2OUT |= BIT5;
}

void move_right()
{
    pwm_stop();
    P2OUT |= (BIT2 + BIT4);
    P2OUT &= ~(BIT1 + BIT5);
    __delay_cycles(380000);
}

void move_left()
{
    pwm_stop();
    P2OUT |= (BIT2 + BIT4 + BIT1 + BIT5);
    __delay_cycles(410000);
}

void nudge_left()
{
    pwm_stop();
    P2OUT |= (BIT2 + BIT4 + BIT1 + BIT5);
    __delay_cycles(40000);
}

void stop()
{
    pwm_stop();
}

void Delay_ms(unsigned int ms)
{
    unsigned int i;
    while(ms--)
        for(i = 0; i < 123; i++);
}

unsigned int read_sensor_avg(int channel)
{
    unsigned long sum = 0;
    unsigned int i;
    for(i = 0; i < 4; i++)
    {
        ADC_Demarrer_conversion(channel);
        sum += ADC_Lire_resultat();
        __delay_cycles(5000);
    }
    return (unsigned int)(sum / 4);
}

void main(void)
{
    WDTCTL = WDTPW | WDTHOLD;
    P2DIR |= (BIT1 + BIT2 + BIT4 + BIT5);
    P2OUT = 0;

    unsigned int counter = 0;
    unsigned int prev_right_sensor = 0;

    ADC10AE0 |= (BIT0 + BIT1 + BIT2 + BIT7);
    __delay_cycles(2000000);
    ADC_init();
    pwm_init();

    unsigned int forward_sensor, right_sensor, light_sensor;

    while(1)
    {
        ADC_Demarrer_conversion(7);
        light_sensor = ADC_Lire_resultat();

        if(light_sensor > 150)
        {
            while(light_sensor > 150)
            {
                ADC_Demarrer_conversion(7);
                light_sensor = ADC_Lire_resultat();
                stop();
                __delay_cycles(200000);
            }
        }
        else
        {

            forward_sensor = read_sensor_avg(0);
            right_sensor   = read_sensor_avg(1);

            if(right_sensor > prev_right_sensor + DRIFT_DEADBAND && right_sensor > CRITICAL_THRESHOLD - 30)
            {
                nudge_left();
            }

            prev_right_sensor = right_sensor;

            if(right_sensor <= SIDE_THRESHOLD)
            {
                if(counter >= COUNTER_THRESHOLD || forward_sensor >= CRITICAL_THRESHOLD)
                {
                    move_right();
                    move_forward();
                    __delay_cycles(200000);
                    stop();
                    __delay_cycles(500000);
                    counter = 0;
                }

                else
                {
                    move_forward();
                    counter++;
                }
            }

            else if(forward_sensor <= CRITICAL_THRESHOLD)
            {
                move_forward();
                counter = 0;
            }

            else
            {
                counter = 0;
                stop();
                move_left();
                stop();
                __delay_cycles(500000);
            }

            Delay_ms(10);
        }
    }
}
