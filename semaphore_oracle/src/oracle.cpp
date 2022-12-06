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
#include <cstdlib>
#include "ITMwrite.h"

SemaphoreHandle_t binary_semaphore;

static void prvSetupHardware(void)
{
	SystemCoreClockUpdate();
	Board_Init();
	/* Initial LED0 state is off */
	Board_LED_Set(0, false);
}

struct Task{
	LpcUart *uart;
	Fmutex *mutx;
};


static void vReadUart(void *pvParameters) {
	//static cast void ptr to lpcuart ptr by type;
	Task *Dty = static_cast<Task *>(pvParameters);

	int count = 0;
	char str[61];

	while(1){
		int bytes = Dty->uart->read(str+count, 60-count); 	//lpcUart read-function returns nmb of chars read
		if(bytes > 0){
			count += bytes;									//one char = one byte
			str[count] = '\0';								//string end sign \0
			Dty->uart->write(str+count-bytes, bytes);		//write to uart

			if(strchr(str, '\r') != NULL || strchr(str, '\n') != NULL || count >= 60){
				Dty->uart->write(str, count);
				Dty->uart->write('\n');
				Dty->mutx->lock();
				ITM_write("[YOU] ");
				ITM_write(str);
				Dty->mutx->unlock();
				for(char ch : str){
					if(ch == '?') {
						xSemaphoreGive(semaphore);
						break;
					}
				}
				count = 0;
			}
		}
	}
}

static void vOracleTask(void *pvParameters){
	const char *ans[5] = { "You are quite lazy to be honest \n",
			"What happen to you this year?\n",
			"Can you be a bit smart to know what to do?\n",
			"Life sometime is so hard, but we still need to move on because hope is bright\n",
			"Hello there\n"};
	Task *Dty = static_cast<Task *>(pvParameters);

	while(1){
		if(xSemaphoreTake(semaphore, portMAX_DELAY) == pdTRUE){
			Dty->mutx->lock();
			ITM_write("[Oracle] Hmmm....\n");
			Dty->mutx->unlock();
			vTaskDelay(3000);
			Dty->mutx->lock();
			ITM_write("[Oracle] ");
			ITM_write(ans[rand() % 5]);
			Dty->mutx->unlock();
			vTaskDelay(2000);
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
