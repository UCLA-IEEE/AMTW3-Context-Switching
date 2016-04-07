/*
 * debug_serial.c
 *
 *  Created on: Jan 3, 2015
 *      Author: Kevin
 */

#include "driver_serial.h"

#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "inc/hw_gpio.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_uart.h"

#include <stdint.h>
#include <string.h>

//#define RECEIVE_BUFFER_SIZE (64)
//
//char RXBuffer[RECEIVE_BUFFER_SIZE];
//int16_t RXBuffer_head, RXBuffer_tail;
//
//void Serial_ISR();

const uint32_t uart_bases[8] =
{
    UART0_BASE,
    UART1_BASE,
    UART2_BASE,
    UART3_BASE,
    UART4_BASE,
    UART5_BASE,
    UART6_BASE,
    UART7_BASE,
};

const uint32_t uart_peripherals[8] =
{
    SYSCTL_PERIPH_UART0,
    SYSCTL_PERIPH_UART1,
    SYSCTL_PERIPH_UART2,
    SYSCTL_PERIPH_UART3,
    SYSCTL_PERIPH_UART4,
    SYSCTL_PERIPH_UART5,
    SYSCTL_PERIPH_UART6,
    SYSCTL_PERIPH_UART7,
};

const uint32_t gpio_peripherals[8] =
{
    SYSCTL_PERIPH_GPIOA,
    SYSCTL_PERIPH_GPIOC,
    SYSCTL_PERIPH_GPIOD,
    SYSCTL_PERIPH_GPIOC,
    SYSCTL_PERIPH_GPIOC,
    SYSCTL_PERIPH_GPIOE,
    SYSCTL_PERIPH_GPIOD,
    SYSCTL_PERIPH_GPIOE,
};

const uint32_t gpio_bases[8] =
{
    GPIO_PORTA_BASE,
    GPIO_PORTC_BASE,
    GPIO_PORTD_BASE,
    GPIO_PORTC_BASE,
    GPIO_PORTC_BASE,
    GPIO_PORTE_BASE,
    GPIO_PORTD_BASE,
    GPIO_PORTE_BASE,
};

const uint32_t gpio_configs[8] =
{
    GPIO_PA0_U0RX | GPIO_PA1_U0TX, GPIO_PC4_U1RX | GPIO_PC5_U1TX,
    GPIO_PD6_U2RX | GPIO_PD7_U2TX, GPIO_PC6_U3RX | GPIO_PC7_U3TX,
    GPIO_PC4_U4RX | GPIO_PC5_U4TX, GPIO_PE4_U5RX | GPIO_PE5_U5TX,
    GPIO_PD4_U6RX | GPIO_PD5_U6TX, GPIO_PE0_U7RX | GPIO_PE1_U7TX,
};

const uint32_t uart_gpio_pins[8] =
{
    GPIO_PIN_0 | GPIO_PIN_1,
    GPIO_PIN_4 | GPIO_PIN_5,
    GPIO_PIN_6 | GPIO_PIN_7,
    GPIO_PIN_6 | GPIO_PIN_7,
    GPIO_PIN_4 | GPIO_PIN_5,
    GPIO_PIN_4 | GPIO_PIN_5,
    GPIO_PIN_4 | GPIO_PIN_5,
    GPIO_PIN_0 | GPIO_PIN_1,
};

void Serial_init(Serial_module_e module, uint32_t baud)
{
//	RXBuffer_head = RXBuffer_tail = 0;

	SysCtlPeripheralEnable(uart_peripherals[module]);
	SysCtlPeripheralEnable(gpio_peripherals[module]);
	UARTClockSourceSet(uart_bases[module], UART_CLOCK_SYSTEM);
	UARTConfigSetExpClk(uart_bases[module], SysCtlClockGet(), baud,
	UART_CONFIG_WLEN_8 | UART_CONFIG_PAR_NONE | UART_CONFIG_STOP_ONE);
	GPIOPinConfigure(gpio_configs[module]);
	GPIOPinTypeUART(gpio_bases[module], uart_gpio_pins[module]);

	UARTIntDisable(uart_bases[module], 0xFFFFFFFF);

//	UARTFIFOLevelSet(UART0_BASE, UART_FIFO_TX1_8, UART_FIFO_RX1_8);
//	UARTIntRegister(UART0_BASE, Serial_ISR);
//	UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);
//	IntEnable(INT_UART0);

	UARTEnable(uart_bases[module]);
}

void Serial_putc(Serial_module_e module, char c)
{
	UARTCharPut(uart_bases[module], c);
}

bool Serial_avail(Serial_module_e module)
{
	return UARTCharsAvail(uart_bases[module]);
}

int Serial_getc(Serial_module_e module)
{
	if (!UARTCharsAvail(uart_bases[module]))
		return -1;
	return (int) UARTCharGet(uart_bases[module]);
}

void Serial_puts(Serial_module_e module, const char * s)
{
	uint16_t i;
	for (i = 0; i < strlen(s); i++)
	{
		if (s[i] == 0)
			break;
		UARTCharPut(uart_bases[module], s[i]);
	}
}

void Serial_writebuf(Serial_module_e module, const uint8_t* buf, uint32_t len)
{
	uint32_t i;
	for (i = 0; i < len; i++)
	{
		UARTCharPut(uart_bases[module], buf[i]);
	}
}

void Serial_flush(Serial_module_e module)
{
	while (UARTBusy(uart_bases[module]))
		;
}

//uint8_t Serial_available()
//{
//	return((RXBuffer_head >= RXBuffer_tail) ?
//			(RXBuffer_head - RXBuffer_tail) : RECEIVE_BUFFER_SIZE - (RXBuffer_tail - RXBuffer_head));
//}
//
//void Serial_ISR()
//{
//
//}
