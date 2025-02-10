#ifndef MAIN_H
# define MAIN_H
# include "stm32f1xx.h"
# include <stdlib.h>
# include <math.h>
# include <stdio.h>
# include <stdint.h>

typedef		uint8_t		U8;		//	Определяем U8 как uint8_t
# define	SYSCLOCK	64000000U
# define	PREFIX		0x3E	//	Префикс пакета
# define	UART_BUFFER_SIZE	4	//	буфер входящего сообщения
__IO	uint8_t	uart_rx_buffer[UART_BUFFER_SIZE];
__IO	uint8_t	uart_rx_index;
__IO	uint8_t	uart_rx_buffer[UART_BUFFER_SIZE];
__IO	uint8_t	uart_rx_index;
__IO	uint8_t	data_received_flag;
		uint8_t	response[9];	//	Длина пакета-ответа 9 байт

	// Структура для хранения запроса
	typedef struct {
		uint8_t prefix;
		uint8_t address;
		uint8_t operation;
		uint8_t crc;
	}	UART_Request_t;

	//		rcc_set.c
	void	RCC_DeInit(void);
	int		RCC_ReInit(void);
	//		uart_init.c
	void	UART_Init(void);
	void	USART1_Init(void);
	//		utils.c
	U8		CRC8(U8 data, U8 crc);
	U8		calculate_crc8(U8 *data, U8 length);
	void	send_data_str(uint8_t *data_str, uint8_t len);
	void	form_response(uint8_t sender_address);
	void	USART1_IRQHandler(void);
	void	process_uart_data(void);

#endif
