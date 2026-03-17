/* main.c */

#include <stdio.h>
#include <stm32f1xx.h>
#include <delay.h>
#include <stddef.h>  // for size_t
#include <unistd.h>  // for STDERR_FILENO
#include "gpio.h"
#include "uart.h"
#include "timer.h"
#include "sllinkedlist.h"


char msg[USART_RCV_BUFF_MAX_SIZE];

int main(void)
{

	/* Init GPIO Port C, Init GPIO Port A , Init GPIO Port A, Init AFIO */
	RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;

	/* Set GPIO on PORT C - P13 */
	GPIOC->CRH &= ~(0xF << ((13 - 8) * 4));
	GPIOC->CRH |=  (0x2 << ((13 - 8) * 4));

	if (usartDrvInit (USART_DRV_1, 115200) != OK)
		while (1){/* system halt */}
		
	if (usartDrvEnable (USART_DRV_1) != OK)
		while (1){/* system halt */}

	SYSTEM_LOG ("USART Drv 1 Enabled!");


	while(1)
	{
		/* Blink */
		SYSTEM_LOG ("System heartbeat");

		/* DBG: Toggle LED*/
		GPIOC->ODR ^= 1 << 13;

		/* DBG */
		printf ("TIM Counter: %lu\r\n", TIM2->CNT);
		ms_delay(1000);
	}
}