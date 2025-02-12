#include "main.h"

__IO	uint8_t	uart_rx_buffer[UART_BUFFER_SIZE];
__IO	uint8_t	uart_rx_index;
__IO	uint8_t	response[9];

__IO	uint8_t	tx_buffer[TX_BUFFER_SIZE];  // Буфер передачи
__IO	uint8_t	tx_index = 0;
__IO	uint8_t	tx_length = 0;

U8	CRC8(U8 data, U8 crc){
    U8 i = data ^ crc;
    crc = 0;
    if(i & 0x01) crc ^= 0x5e;
    if(i & 0x02) crc ^= 0xbc;
    if(i & 0x04) crc ^= 0x61;
    if(i & 0x08) crc ^= 0xc2;
    if(i & 0x10) crc ^= 0x9d;
    if(i & 0x20) crc ^= 0x23;
    if(i & 0x40) crc ^= 0x46;
    if(i & 0x80) crc ^= 0x8c;
    return crc;
}

U8	calculate_crc8(volatile U8 *data, U8 length) {
    U8	crc = 0;
    U8	i = 1;
    while (i < (length - 1)){
    	crc = CRC8(data[i], crc);
    	i++;
    }
    return crc;
}

//	функция передачи оригинальных данных (массив response[])
//void	send_data_str(volatile uint8_t *data_str, uint8_t len) {
//    if (tx_length == 0) {
//        tx_length = len;
//        tx_index = 0;
//        // Запускаем передачу первого байта
//        WRITE_REG(USART1->DR, data_str[tx_index++]);
//        // Включаем прерывание TXE (чтобы отправлять остальные байты в IRQ)
//        SET_BIT(USART1->CR1, USART_CR1_TXEIE);
//    }
//}

//	функция начала безопасной передачи данных (работа с массивом-копией):
void	send_data_str(volatile uint8_t *data_str, uint8_t len){
    if (tx_length == 0 && len <= TX_BUFFER_SIZE) {  // Проверяем, влезает ли сообщение в буфер
        memcpy((uint8_t *)tx_buffer, (const uint8_t *)data_str, len);  // Копируем данные в безопасный буфер
        tx_length = len;
        tx_index = 0;
        // Запускаем передачу первого байта
        WRITE_REG(USART1->DR, tx_buffer[tx_index++]);
        // Включаем прерывание TXE (чтобы отправлять остальные байты в IRQ)
        SET_BIT(USART1->CR1, USART_CR1_TXEIE);
    }
}

//	Основная функция формирования посылки
//	Адрес отправителя = 0xA5
void	form_response(uint8_t sender_address){
    response[0] = PREFIX;             // 3Eh - Префикс
    response[1] = sender_address;     // Адрес отправителя
    response[2] = 0x06;               // Код операции (ответ на запрос 06h)
    response[3] = 28;                 // Температура (28 градусов)
    response[4] = 0x34;               // Уровень топлива (младший байт)
    response[5] = 0x12;               // Уровень топлива (старший байт)
    response[6] = 0x78;               // Частота (младший байт)
    response[7] = 0x56;               // Частота (старший байт)

    // Рассчитываем CRC (XOR всех байтов, кроме PREFIX)
    U8	crc = calculate_crc8(response, sizeof(response));
    response[8] = crc;
}

//	Обработчик прерывания USART1
//	Флаг RXNE в USART1->SR сбрасывается автоматически при чтении USART1->DR
void USART1_IRQHandler(void) {
    if (USART1->SR & USART_SR_RXNE) {  // Флаг RXNE (приём данных)
        uint8_t received_byte = USART1->DR;
        if (uart_rx_index < UART_BUFFER_SIZE) {
            uart_rx_buffer[uart_rx_index++] = received_byte;
        }
        if (uart_rx_index >= UART_BUFFER_SIZE) {
            uart_rx_index = 0;
            data_received_flag = 1;	//	Флаг о принятии полного сообщения
        }
    }

    if (READ_BIT(USART1->SR, USART_SR_TXE)
    		&& READ_BIT(USART1->CR1, USART_CR1_TXEIE)) {  // Передача данных
//    	//	при использовании оригинального массива response[]:
//    	if (tx_index < tx_length) {
//       		WRITE_REG(USART1->DR, response[tx_index++]);
    	//	при использовании копии - массива tx_buffer[]
    	//	типа безопасное копирование, данные на передачу в массиве-копии:
    	if (tx_index < tx_length) {
    		WRITE_REG(USART1->DR, tx_buffer[tx_index++]);
    	} else {
    		// Отключаем TXEIE, так как передача завершена
    		CLEAR_BIT(USART1->CR1, USART_CR1_TXEIE);
    		tx_length = 0;
    	}
    }
}

//	Обработка принятого сообщения
void process_uart_data(void) {
    UART_Request_t		request;

    request.prefix    = uart_rx_buffer[0];
    request.address   = uart_rx_buffer[1];
    request.operation = uart_rx_buffer[2];
    request.crc       = uart_rx_buffer[3];

    // Проверяем префикс
    if (request.prefix != 0x31) {
        data_received_flag = 0;
        return;
    }
    // Проверяем CRC
    uint8_t calculated_crc = calculate_crc8((uint8_t *)uart_rx_buffer, sizeof(uart_rx_buffer));
    if (calculated_crc != request.crc) {
        data_received_flag = 0;
        return;
    }
    // Если операция 06h, вызываем data_proc()
    if (request.operation == 0x06) {
    	//	запуск передачи данных по USART (по байтам)
    	send_data_str(response, 9);
    }
    data_received_flag = 0;  // Сбрасываем флаг
}
