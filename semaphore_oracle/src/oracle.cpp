
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
#include "LpcUart.h"
#include "ITMwrite.h"
#include "Fmutex.h"
#include <cstring>
#include <cstdlib>


SemaphoreHandle_t semaphore;

static void prvSetupHardware(void)
{
	SystemCoreClockUpdate();
	Board_Init();

	ITM_init();

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
				ITM_write("[YOU]: ");
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
	const char *ans[10] = {
			"Fortune favours the bold... \n",
			"You must open the third eye. See the unseen.\n",
			"Please assess the situation, you are just confused?\n",
			"RUN! Or walk. Just move. You'know, for your blood pressure.\n",
			"I am become death, destroyer of worlds!\n",
			"I am feeling a little peckish, put your hand on the USB port.\n",
			"Have you been swimming lately?\n",
			"I heard abyss is nice this time of NULL\n",
			"You had nine lives. One to go.\n",
			"Ahhh. You're too young to understand, mortal\n"};

	Task *locker = static_cast<Task *>(pvParameters);

	while(1){
		if(xSemaphoreTake(semaphore, portMAX_DELAY) == pdTRUE){
			locker->mutx->lock();
			ITM_write("[Oracle]: Hmmm....\n");
			locker->mutx->unlock();
			vTaskDelay(3000);
			locker->mutx->lock();
			ITM_write("[Oracle]: ");
			ITM_write(ans[rand() % 10]);
			locker->mutx->unlock();
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

	//counting semaphore & basic mutex guard
	semaphore = xSemaphoreCreateCounting(3, 0);
	Fmutex *mutex = new Fmutex();


	LpcPinMap none = {-1, -1}; // unused pin has negative values in

	LpcPinMap txpin = {.port = 0, .pin = 18 }; // transmit pin that goes to debugger's UART->USB converter
	LpcPinMap rxpin = { .port = 0, .pin = 13 }; // receive pin that goes to debugger's UART->USB converter

	//UART config using txpin, rxpin and 115200 baud rate
	LpcUartConfig cfg = {
			.pUART = LPC_USART0,
			.speed = 115200,
			.data = UART_CFG_DATALEN_8 | UART_CFG_PARITY_NONE | UART_CFG_STOPLEN_1,
			.rs485 = false,
			.tx = txpin,
			.rx = rxpin,
			.rts = none,
			.cts = none
	};

	//debug UART using the cfg above
	LpcUart *debug = new LpcUart(cfg);

	static Task task = { debug, mutex};

	xTaskCreate(vReadUart, "vReadLpcUart",
				configMINIMAL_STACK_SIZE + 128, &task, (tskIDLE_PRIORITY + 1UL),
				(TaskHandle_t *) NULL);

	xTaskCreate(vOracleTask, "vOracleTask",
				configMINIMAL_STACK_SIZE + 128, &task, (tskIDLE_PRIORITY + 1UL),
				(TaskHandle_t *) NULL);

	/* Start the scheduler */
	vTaskStartScheduler();

	/* Should never arrive here */
	return 1;
}
