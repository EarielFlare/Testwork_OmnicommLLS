#include "main.h"

void	UART_Init(void){
	//	этот кусок кода позволняет сконфигурировать ПИНЫ
	//	TX(PB6) и RX(PB7), которые по умолчанию не являются UART
	//	поэтому здесь надо сделать REMAP (переназначение)!
    // Отключаем I2C1, если он был включен
    RCC->APB1ENR &= ~RCC_APB1ENR_I2C1EN;
    // Включаем альтернативное переназначение USART1 на PB6/PB7
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;  // Включаем тактирование AFIO
    AFIO->MAPR |= AFIO_MAPR_USART1_REMAP;  // Перенос USART1 на PB6/PB7
	//	включаем тактирование UART1 (шина APB2)
	RCC->APB2ENR	|= RCC_APB2ENR_USART1EN;
	//	включаем тактирование порта B
	RCC->APB2ENR	|= RCC_APB2ENR_IOPBEN;
	//	настройка вывода PB6 (TX1)
	//	в режим альтернативной функции с активным выходом
	//	alternate function output, push-pull, 50MHz speed
	//	биты CNF = 10, биты MODE = 11
	GPIOB->CRL &= ~(GPIO_CRL_MODE6 | GPIO_CRL_CNF6);
	GPIOB->CRL |= GPIO_CRL_CNF6_1; // Альтернативный push-pull
	GPIOB->CRL |= (GPIO_CRL_MODE6_1 | GPIO_CRL_MODE6_0); // 50MHz
	//	настройка вывода PB7 (RX1)
	//	в режим входа с подтягивающим резистором
	//	input pull-up,
	//	биты CNF = 10, биты MODE = 00, ODR = 1
	GPIOB->CRL &= ~(GPIO_CRL_MODE7 | GPIO_CRL_CNF7);
	GPIOB->CRL |= GPIO_CRL_CNF7_1; // Вход с подтяжкой
	GPIOB->ODR |= GPIO_ODR_ODR7; // Подтяжка к питанию

	// конфигурация UART1
	USART1->CR1 = USART_CR1_UE; // разрешаем USART1, сбрасываем остальные биты
	//	устанавливаем скорость обмена.
	//	частота тактирования UART1 64 мГц, нам нужна скорость 19200 бод
	//	вычисляем значение USARTDIV
	//	USARTDIV = Fck / (16 * BAUD) = 64000000 / (16 * 19200) = 208,3333
	//	значение регистра USART_BRR = 208,3333 * 16 = 3333
	USART1->BRR = 3333;	//	скорость 19200 бод
	//	разрешаем передатчик и приемник
	USART1->CR1 |= USART_CR1_TE | USART_CR1_RE;
	USART1->CR2 = 0;
	USART1->CR3 = 0;
	//	разрешаем прерывание по приему
	USART1->CR1 |= USART_CR1_RXNEIE;
	// разрешаем прерывания UART1 в контроллере прерываний
	NVIC_EnableIRQ(USART1_IRQn);
    NVIC_SetPriority(USART1_IRQn, 1);
}
