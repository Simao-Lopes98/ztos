/*
 * timer.c
 * Timer Hal - Timer Hardware Abstraction Layer
 */

/* Includes */

#include "timer.h"

/* Defines */

/* Type definitions */

/* Private variables */
static TIM_TypeDef * timerDrvs [] = 
                    {
                    TIM1,
                    TIM2,
                    TIM3,
                    TIM4
                    };

/* Private function prototypes */
static STATUS checkTimerDrvNum (timerDrvNum_t timerNum);
static void timer2Init (void);


STATUS timerDrvInit (timerDrvNum_t timerNum)
{
    if (checkTimerDrvNum(timerNum) != OK)
    {
        return ERROR;
    }

    /* 
    TODO: Add other timer implementations
    */
    switch (timerNum)
    {
    case TIMER_DRV_2:
        timer2Init();
        break;
    default:
        return ERROR;
        break;
    }

    return OK;
}

STATUS timerDrvEnable (timerDrvNum_t timerNum)
{
    TIM_TypeDef * timerDrv = NULL;

    if (checkTimerDrvNum(timerNum) != OK)
    {
        return ERROR;
    }

    timerDrv = timerDrvs[timerNum];

    timerDrv->CR1 |= TIM_CR1_CEN;
    return OK;
}

STATUS timerDrvStop (timerDrvNum_t timerNum)
{
    TIM_TypeDef * timerDrv = NULL;

    if (checkTimerDrvNum(timerNum) != OK)
    {
        return ERROR;
    }

    timerDrv = timerDrvs[timerNum];

    timerDrv->CR1 &= ~TIM_CR1_CEN;
    return OK;
}

static void timer2Init (void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    /*CLK Prescaler*/
    TIM2->PSC = 512;

    /* 
    Value to be compared with calculated by:
    fout = fclk / ((PSC + 1) x (ARR + 1))
    For this config, 16 with 512 PSC will lead to timer being triggered each 10ms
    */
    TIM2->ARR = 16;
    
    /* Enable IRQ */
    NVIC_EnableIRQ (TIM2_IRQn);
    
    /* Enable ISR */
    TIM2->DIER |= TIM_DIER_UIE;

}

static STATUS checkTimerDrvNum (timerDrvNum_t timerNum)
{
    if (timerNum > TIMER_DRV_4 || timerNum < TIMER_DRV_1)
    {
        return ERROR;
    }

    return OK;  
}