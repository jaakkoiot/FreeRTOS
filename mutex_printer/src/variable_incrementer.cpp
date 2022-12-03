/*
===============================================================================
 Name        : main.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
*/

#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#endif
#endif

#include <cr_section_macros.h>

// TODO: insert other include files here

// TODO: insert other definitions and declarations here

#include "FreeRTOS.h"
#include "task.h"
#include "heap_lock_monitor.h"
#include "DigitalIoPin.h"

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

DigitalIoPin SW1(0, 17, DigitalIoPin::pinMode::pullup, true);

/* Sets up system hardware */
static void prvSetupHardware(void)
{
	SystemCoreClockUpdate();
	Board_Init();

	/* Initial LED0 state is off */
	Board_LED_Set(0, false);
}

/* LED1 toggle thread */
static void vLEDTask1(void *pvParameters) {
	//print dot - dash - dot, if state is dot (is_dot = true) toggle is faster
	bool is_dot = true;

	//print sequence: dot - dash - dot
	//vTaskDelay(1500) needs to encompass the whole dot - dash - dot cycle:
	//dash is 3 * dot and there are 3 dashes: dashes represent 18 dots in total (3 * 2 * 3 = 18)
	//there are in total 6 dots + 18 dots from dashes: total = 24 dots;
	//so dot should be a duration of 2400 / 24 = 100

	int dot = 100; 		//or: configTICK_RATE_HZ / 10
	int dash = dot * 3;	//dash length = 3 dots = 300ms


	while (1) {
		if(is_dot){
			for(int k = 0; k < 3; k++){
				Board_LED_Set(0, true);
				vTaskDelay(dot);
				Board_LED_Set(0, false);
				vTaskDelay(dot);
			}
		}else{
			for(int k = 0; k < 3; k++){
				Board_LED_Set(0, true);
				vTaskDelay(dash);
				Board_LED_Set(0, false);
				vTaskDelay(dash);
			}
		}
		is_dot = (bool) !is_dot;
	}
}

/* LED2 toggle thread */
static void vLEDTask2(void *pvParameters) {
	bool LedState = false;

	while (1) {
		Board_LED_Set(1, LedState);
		LedState = (bool) !LedState;

		/* state toggles at a rate of 2.4s */
		vTaskDelay(2400);
	}
}

/* UART outputs variable increments: 1/sec and 10/sec when SW1 is active */
static void vUARTTask(void *pvParameters) {
	unsigned int count = 0;

	unsigned int slow = configTICK_RATE_HZ;
	unsigned int fast = configTICK_RATE_HZ / 10;

	bool light = false;

	while (1) {
		DEBUGOUT("tck: %d \r\n", count);
		count++;

		Board_LED_Set(1, light);
		light = (bool) !light;

		if(SW1.read()) {
			vTaskDelay(fast);
		}else
			vTaskDelay(slow);
	}
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/* the following is required if runtime statistics are to be collected */
extern "C" {

void vConfigureTimerForRunTimeStats( void ) {
	Chip_SCT_Init(LPC_SCTSMALL1);
	LPC_SCTSMALL1->CONFIG = SCT_CONFIG_32BIT_COUNTER;
	LPC_SCTSMALL1->CTRL_U = SCT_CTRL_PRE_L(255) | SCT_CTRL_CLRCTR_L; // set prescaler to 256 (255 + 1), and start timer
}

}
/* end runtime statistics collection */

/**
 * @brief	main routine for FreeRTOS blinky example
 * @return	Nothing, function should not exit
 */
int main(void)
{
	prvSetupHardware();

	heap_monitor_setup();

	/* LED1 toggle thread */
	xTaskCreate(vLEDTask1, "vTaskLed1",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
				(TaskHandle_t *) NULL);

	/* LED2 toggle thread */
	xTaskCreate(vLEDTask2, "vTaskLed2",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
				(TaskHandle_t *) NULL);

	/* UART output thread */
	xTaskCreate(vUARTTask, "vTaskUart",
					configMINIMAL_STACK_SIZE + 128, NULL, (tskIDLE_PRIORITY + 1UL),
					(TaskHandle_t *) NULL);

	/* Start the scheduler */
	vTaskStartScheduler();

	/* Should never arrive here */
	return 1;
}

