#include "stm8s.h"
#include "milis.h"
#include "stm8_hd44780.h"
//#include "delay.h"
#include "uart1.h"
#include <stdio.h>

#define _ISOC99_SOURCE
#define _GNU_SOURCE

#define LED_PORT GPIOC
#define LED_PIN  GPIO_PIN_5
#define LED_HIGH   GPIO_WriteHigh(LED_PORT, LED_PIN)
#define LED_LOW  GPIO_WriteLow(LED_PORT, LED_PIN)
#define LED_REVERSE GPIO_WriteReverse(LED_PORT, LED_PIN)

#define MS1_PORT GPIOG
#define MS1_PIN  GPIO_PIN_4
#define MS1_HIGH   GPIO_WriteHigh(MS1_PORT, MS1_PIN)
#define MS1_LOW  GPIO_WriteLow(MS1_PORT, MS1_PIN)
#define MS1_REVERSE GPIO_WriteReverse(MS1_PORT, MS1_PIN)

#define MS2_PORT GPIOG
#define MS2_PIN  GPIO_PIN_5
#define MS2_HIGH   GPIO_WriteHigh(MS2_PORT, MS2_PIN)
#define MS2_LOW  GPIO_WriteLow(MS2_PORT, MS2_PIN)
#define MS2_REVERSE GPIO_WriteReverse(MS2_PORT, MS2_PIN)


#define BTN_PORT GPIOE
#define BTN_PIN  GPIO_PIN_4
#define BTN_PUSH (GPIO_ReadInputPin(BTN_PORT, BTN_PIN)==RESET) 

#define NCODER_CLK_PORT GPIOB
#define NCODER_CLK_PIN GPIO_PIN_5
#define NCODER_DATA_PORT GPIOB
#define NCODER_DATA_PIN GPIO_PIN_4
#define NCODER_SW_PORT GPIOB
#define NCODER_SW_PIN GPIO_PIN_3
#define NCODER_GET_CLK (GPIO_ReadInputPin(NCODER_CLK_PORT, NCODER_CLK_PIN) != RESET)
#define NCODER_GET_DATA ( GPIO_ReadInputPin(NCODER_DATA_PORT, NCODER_DATA_PIN)!=RESET)
#define NCODER_GET_SW ( GPIO_ReadInputPin(SW_PORT, NCODER_SW_PIN )==RESET)

void tim2_setup(void){
     TIM2_TimeBaseInit(TIM2_PRESCALER_8, 40000); 
    //TIM2_ITConfig(TIM2_IT_UPDATE, ENABLE);
    TIM2_OC1Init(                // inicializujeme kanál 1 (TM2_CH1)
        TIM2_OCMODE_PWM1,        // režim PWM1
        TIM2_OUTPUTSTATE_ENABLE, // Výstup povolen (TIMer ovládá pin)
        3000,                    // výchozí hodnota šířky pulzu (střídy) 1056/1600 = 66%
        TIM2_OCPOLARITY_HIGH      // Polarita LOW protože LED rozsvěcím 0 (spol. anoda)
     );


     TIM2_OC1PreloadConfig(ENABLE);

     TIM2_Cmd(ENABLE);
}

void setup(void)
{
    CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);      // taktovani MCU na 16MHz
    GPIO_Init(LED_PORT, LED_PIN, GPIO_MODE_OUT_PP_LOW_SLOW);
    GPIO_Init(BTN_PORT, BTN_PIN, GPIO_MODE_IN_FL_NO_IT);

    GPIO_Init(NCODER_CLK_PORT, NCODER_CLK_PIN, GPIO_MODE_IN_FL_NO_IT);
    GPIO_Init(NCODER_DATA_PORT, NCODER_DATA_PIN, GPIO_MODE_IN_FL_NO_IT);
    GPIO_Init(NCODER_SW_PORT, NCODER_SW_PIN, GPIO_MODE_IN_PU_NO_IT);

    GPIO_Init(GPIOB,GPIO_PIN_7,GPIO_MODE_IN_PU_NO_IT); // nastavíme pin PG4 jako vstup s vnitřním pull-up rezistorem  v pospisech nesedi jmena pinu
    GPIO_Init(GPIOB,GPIO_PIN_6,GPIO_MODE_IN_PU_NO_IT); // nastavíme pin PG5 jako vstup s vnitřním pull-up rezistorem
    GPIO_Init(GPIOD,GPIO_PIN_7,GPIO_MODE_IN_PU_NO_IT); // nastavíme pin PG4 jako vstup s vnitřním pull-up rezistorem

    GPIO_Init(MS1_PORT, MS1_PIN, GPIO_MODE_OUT_PP_LOW_SLOW);
    GPIO_Init(MS2_PORT, MS2_PIN, GPIO_MODE_OUT_PP_LOW_SLOW);



    init_milis();
    init_uart1();
    lcd_init();
    tim2_setup();
}

int minule = 0;
int8_t check_ncoder(void)
{

    if (minule == 0 && NCODER_GET_CLK == 1) {
        // vzestupná hrana 
        minule = 1;
        if (NCODER_GET_DATA == 0) {
            return 1;
        } else {
            return -1;
        }
    } 
    else if (minule == 1 && NCODER_GET_CLK == 0) {
        // sestupná hrana 
        minule = 0;
        if (NCODER_GET_DATA == 0) {
            return -1;
        } else {
            return 1;
        }
    }
    return 0;
}

int main(void)
{
    uint32_t time = 0;
    uint32_t time1 = 0;
    int16_t bagr = 0;
    char text[32];

    _Bool minuly_stav_on_tlacitka=0; // 0=tlačítko bylo minule uvolněné, 1=tlačítko bylo minule stisknuté 
    _Bool minuly_stav_off_tlacitka=0; // 0=tlačítko bylo minule uvolněné, 1=tlačítko bylo minule stisknuté 
    _Bool aktualni_stav_tlacitka=0; // 0=tlačítko je uvolněné, 1= tlačítko je stisknuté

    _Bool minuly_stav_on_tlacitka2=0; // 0=tlačítko bylo minule uvolněné, 1=tlačítko bylo minule stisknuté 
    _Bool minuly_stav_off_tlacitka2=0; // 0=tlačítko bylo minule uvolněné, 1=tlačítko bylo minule stisknuté 
    _Bool aktualni_stav_tlacitka2=0; // 0=tlačítko je uvolněné, 1= tlačítko je stisknuté

    _Bool minuly_stav_on_tlacitka3=0; // 0=tlačítko bylo minule uvolněné, 1=tlačítko bylo minule stisknuté 
    _Bool minuly_stav_off_tlacitka3=0; // 0=tlačítko bylo minule uvolněné, 1=tlačítko bylo minule stisknuté 
    _Bool aktualni_stav_tlacitka3=0; // 0=tlačítko je uvolněné, 1= tlačítko je stisknuté

    setup();

    
    while (1) {
        lcd_gotoxy(0,0);
        lcd_puts("smer:");

        lcd_gotoxy(0,1);
        lcd_puts("rychlost:");
        //////////////////////////////////////////////////////////////////////////////////////////////////////////////tlačítko 1
        // zjistíme stav "ON" tlačítka
            if(GPIO_ReadInputPin(GPIOB,GPIO_PIN_7)==RESET){ // pokud je na vstupu od "ON" tlačítka log.0 tak...
            aktualni_stav_tlacitka=1; // ...je tlačítko stisknuté
            }
            else{ // jinak je ...
            aktualni_stav_tlacitka=0; // ...tlačítko uvolněné
            }
            // zjišťujeme jestli nastal "okamžik stisku"
            if(minuly_stav_on_tlacitka==0 && aktualni_stav_tlacitka==1){
            lcd_gotoxy(6,0);
            lcd_puts("         ");
            
            lcd_gotoxy(6,0);
            lcd_puts("R  vpravo");

            MS2_LOW;
            MS1_HIGH;
            
            }       
            minuly_stav_on_tlacitka = aktualni_stav_tlacitka; // přepíšeme minulý stav tlačítka aktuálním
            //////////////////////////////////////////////////////////////////////////////////////////////////////////////tlačítko 2
            // zjistíme stav "ON" tlačítka
            if(GPIO_ReadInputPin(GPIOB,GPIO_PIN_6)==RESET){ // pokud je na vstupu od "ON" tlačítka log.0 tak...
            aktualni_stav_tlacitka2=1; // ...je tlačítko stisknuté
            }
            else{ // jinak je ...
            aktualni_stav_tlacitka2=0; // ...tlačítko uvolněné
            }
            // zjišťujeme jestli nastal "okamžik stisku"
            if(minuly_stav_on_tlacitka2==0 && aktualni_stav_tlacitka2==1){

            lcd_gotoxy(6,0);
            lcd_puts("         ");
            
            lcd_gotoxy(6,0);
            lcd_puts("X  stop");

            MS1_LOW;
            MS2_LOW;
            
            }
            minuly_stav_on_tlacitka2 = aktualni_stav_tlacitka2; // přepíšeme minulý stav tlačítka aktuálním
            ///////////////////////////////////////////////////////////////////////////////////////////////////////////tlačítko 3
            // zjistíme stav "ON" tlačítka
            if(GPIO_ReadInputPin(GPIOD,GPIO_PIN_7)==RESET){ // pokud je na vstupu od "ON" tlačítka log.0 tak...
            aktualni_stav_tlacitka3=1; // ...je tlačítko stisknuté
            }
            else{ // jinak je ...
            aktualni_stav_tlacitka3=0; // ...tlačítko uvolněné
            }
            // zjišťujeme jestli nastal "okamžik stisku"
            if(minuly_stav_on_tlacitka3==0 && aktualni_stav_tlacitka3==1){

            lcd_gotoxy(6,0);
            lcd_puts("          ");

            lcd_gotoxy(6,0);
            lcd_puts("L  vlevo");

        
            MS1_LOW;
            MS2_HIGH;
            
            }
            minuly_stav_on_tlacitka3 = aktualni_stav_tlacitka3; // přepíšeme minulý stav tlačítka aktuálním
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        if (milis() - time > 33) {
            LED_REVERSE;
            time = milis();
            printf("\r  %5d     ", bagr);

            lcd_gotoxy(9,1);
            sprintf(text, "       ");
            lcd_puts(text);

            lcd_gotoxy(10,1);
            sprintf(text, "%d", bagr);
            lcd_puts(text);


        }

        TIM2_SetCompare1(bagr * 100);

        if (milis() - time1 > 1) {
            time1 = milis();
        }
        bagr += check_ncoder();

    }
}

/*-------------------------------  Assert -----------------------------------*/
#include "__assert__.h"
