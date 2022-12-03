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
#include "Fmutex.h"
#include "LpcUart.h"

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/* Sets up system hardware */
static void prvSetupHardware(void)
{
	SystemCoreClockUpdate();
	Board_Init();

	/* Initial LED0 state is off */
	Board_LED_Set(0, false);
}

/* Define buttons */
DigitalIoPin SW1(0, 17, DigitalIoPin::pullup, true);
DigitalIoPin SW2(1, 11, DigitalIoPin::pullup, true);
DigitalIoPin SW3(1, 9, DigitalIoPin::pullup, true);

/* serial communication definition */
LpcPinMap none = {-1, -1}; // unused pin has negative values in
LpcPinMap txpin = {.port = 0, .pin = 18 }; // transmit pin that goes to debugger's UART->USB converter
LpcPinMap rxpin = { .port = 0, .pin = 13 }; // receive pin that goes to debugger's UART->USB converter
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


struct ButtonTask{
	int btn_number;
};


//using arrays to map ports & pins
static const int port_map[] {0, 0, 1, 1};  // first element is dummy
static const int pin_map[] {0, 17, 11, 9};


LpcUart lpcUart(cfg);
Fmutex m;

static void vPushButton(void *pvParameters){
	ButtonTask *btn_index = static_cast <ButtonTask *> (pvParameters);
	DigitalIoPin button(port_map[btn_index->btn_number], pin_map[btn_index->btn_number], DigitalIoPin::pullup, true);
	while(1){
		if(button.read()){
			char buff[50];
			snprintf(buff, 50, "Sw%d pressed\r\n", btn_index->btn_number);
			m.lock();
			lpcUart.write(buff);
			m.unlock();
		}
		vTaskDelay(100);
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

	static ButtonTask t1 {1};
	static ButtonTask t2 {2};
	static ButtonTask t3 {3};

	/* RTOS threads */
	xTaskCreate(vPushButton, "Button1",
					configMINIMAL_STACK_SIZE + 80, &t1, (tskIDLE_PRIORITY + 1UL),
					(TaskHandle_t *) NULL);

	xTaskCreate(vPushButton, "Button2",
					configMINIMAL_STACK_SIZE + 80, &t2, (tskIDLE_PRIORITY + 1UL),
					(TaskHandle_t *) NULL);

	xTaskCreate(vPushButton, "Button3",
					configMINIMAL_STACK_SIZE + 80, &t3, (tskIDLE_PRIORITY + 1UL),
					(TaskHandle_t *) NULL);

	/* Start the scheduler */
	vTaskStartScheduler();

	/* Should never arrive here */
	return 1;
}

