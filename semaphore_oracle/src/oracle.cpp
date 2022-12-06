#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#endif
#endif

#include <cr_section_macros.h>
#include "FreeRTOS.h"
#include "task.h"
#include "heap_lock_monitor.h"
#include "semphr.h"
#include <cstring>


SemaphoreHandle_t binary_semaphore;

static void prvSetupHardware(void)
{
	SystemCoreClockUpdate();
	Board_Init();
	/* Initial LED0 state is off */
	Board_LED_Set(0, false);
}

//TO-DO for tasks: add separate task for different indicator function

static void vread(void *pvParameters){

	int ch;
	while(1){
		ch = Board_UARTGetChar();
		if(ch != EOF){
			if(ch =='\n') Board_UARTPutChar('\r');
			Board_UARTPutChar(ch);
			if(ch == '\r')Board_UARTPutChar('\n');
			xSemaphoreGive(binary_semaphore);
		}
	}
}


static void vblink(void *pvParameters){

	while(1){
		if(xSemaphoreTake(binary_semaphore, portMAX_DELAY) == pdTRUE){
			Board_LED_Set(0, true);
			vTaskDelay(100);
			Board_LED_Set(0,false);
			vTaskDelay(100);
		}
	}

}


/* the following is required if runtime statistics are to be collected */
extern "C" {

void vConfigureTimerForRunTimeStats( void ) {
	Chip_SCT_Init(LPC_SCTSMALL1);
	LPC_SCTSMALL1->CONFIG = SCT_CONFIG_32BIT_COUNTER;
	LPC_SCTSMALL1->CTRL_U = SCT_CTRL_PRE_L(255) | SCT_CTRL_CLRCTR_L; // set prescaler to 256 (255 + 1), and start timer
}

}

int main(void)
{
 	prvSetupHardware();
	heap_monitor_setup();
	binary_semaphore = xSemaphoreCreateBinary();

	xTaskCreate(vblink, "blink",
				configMINIMAL_STACK_SIZE + 64, NULL, (tskIDLE_PRIORITY + 1UL),
				(TaskHandle_t *) NULL);

	xTaskCreate(vread, "read",
				configMINIMAL_STACK_SIZE + 64, NULL, (tskIDLE_PRIORITY + 1UL),
				(TaskHandle_t *) NULL);

	/* Start the scheduler */
	vTaskStartScheduler();

	/* Should never arrive here */
	return 1;
}
