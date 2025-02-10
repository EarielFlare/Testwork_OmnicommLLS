#include "main.h"

void	RCC_DeInit(void){
	//	включаем тактирование от HSI
	SET_BIT(RCC->CR, RCC_CR_HSION);
	//	ждем, пока HSI включится
	//	бит RCC_CR_HSIRDY изначально = 0,
	//	условие работает, пока (0 == 0)
	//	двинемся дальше, когда будет (1 == 0)
	while (READ_BIT(RCC->CR,	RCC_CR_HSIRDY)	== RESET){}

	//	устанавливаем RCC_CR_HSITRIM в дефолтное 16(dec)
	//	число "16" с учетом смещения от начала = 10000 000
	MODIFY_REG(RCC->CR, RCC_CR_HSITRIM, 1<<7);

	//	очищаем регистр CFGR
	CLEAR_REG(RCC->CFGR);
	//	ждем, пока бит RCC_CFGR_SWS обнулится
	//	бит RCC_CFGR_SWS изначально неизвестен,
	//	условие работает, пока (x != 0)
	//	двинемся дальше, когда будет (0 != 0)
	while (READ_BIT(RCC->CFGR,	RCC_CFGR_SWS)	!= RESET){}

	//	сбрасываем бит RCC_CR_PLLON (отключаем PLL)
	CLEAR_BIT(RCC->CR, RCC_CR_PLLON);
	//	ждем, пока будет сброшен RCC_CR_PLLRDY
	while (READ_BIT(RCC->CR,	RCC_CR_PLLRDY)	!= RESET){}

	//	сбрасываем бит RCC_CR_HSEON (отключаем HSE)
	//	сбрасываем бит RCC_CR_CSSON (отключаем CSS)
	CLEAR_BIT(RCC->CR, RCC_CR_HSEON | RCC_CR_CSSON);
	//	ждем, пока будет сброшен RCC_CR_HSERDY
	while (READ_BIT(RCC->CR,	RCC_CR_HSERDY)	!= RESET){}

	//	сброс бита, отвечающего за внешний источник тактирования
	//	нет тактирования - нет "1" в бите RCC_CR_HSEBYP
	CLEAR_BIT(RCC->CR, RCC_CR_HSEBYP);

	//	запись "1" в RCC_CSR_RMVF сбрасывает весь регистр CSR
	SET_BIT(RCC->CSR, RCC_CSR_RMVF);
}

//	настраиваем тактирование системы от внешнего
//	кварца 8MHz через PLL на частоте 48MHz
//	функция возвращает:
//  0 - завершено успешно
//  1 - не запустился кварц
//  2 - не запустился PLL
int	RCC_ReInit(void){
	__IO uint32_t	StartUpCounter;
	__IO uint32_t	HSEStartUp_TimeOut;

	HSEStartUp_TimeOut	= 10000;
	//	включаем буфер предвыборки FLASH
	//	хотя после сброса он и так выставлен в "1"
//	FLASH->ACR |= FLASH_ACR_PRFTBE;
	SET_BIT(FLASH->ACR, FLASH_ACR_PRFTBE);
	//	ожидаем установки флага FLASH_ACR_PRFTBS
	//	хотя после сброса он и так выставлен в "1"
	while (READ_BIT(FLASH->ACR,	FLASH_ACR_PRFTBS) == RESET){}

	//	так как частота ядра 48MHz < SYSCLK <= 72MHz
	//	устанавливаем 2 цикла ожидания для Flash (48MHz)
	//	сначала обнуляем содержимое FLASH_ACR_LATENCY: 000
	CLEAR_BIT(FLASH->ACR, FLASH_ACR_LATENCY);
	//	затем устанавливаем флаг FLASH_ACR_LATENCY_1: 010
	SET_BIT(FLASH->ACR, FLASH_ACR_LATENCY_1);

	//	устанавливаем бит RCC_CR_CSSON (включаем CSS)
	SET_BIT(RCC->CR, RCC_CR_CSSON);
	//	запускаем кварцевый генератор (HSE)
//	RCC->CR |= (1<<RCC_CR_HSEON_Pos);
	SET_BIT(RCC->CR, RCC_CR_HSEON);
	//	входной сигнал НЕ делим на 2: RCC_CFGR_PLLXTPRE = 0
//	RCC->CFGR &= (1<<RCC_CFGR_PLLXTPRE_Pos);
	CLEAR_BIT(RCC->CR, RCC_CFGR_PLLXTPRE);

	StartUpCounter = 0;
	do	{
		StartUpCounter++;
	}	while((READ_BIT(RCC->CR, RCC_CR_HSERDY) == 0)
			&& (StartUpCounter != HSEStartUp_TimeOut));
	//	если выход произошел по счетчику, значит HSE не работает
	//	значит, отключаем бит RCC_CR_HSEON и возвращаем "1"
	if (StartUpCounter == HSEStartUp_TimeOut){
		//	останавливаем генератор (HSE)
//		RCC->CR &= ~(1<<RCC_CR_HSEON_Pos);
		CLEAR_BIT(RCC->CR, RCC_CR_HSEON);
		return (1);
	}

	//	HCLK = SYSCLK
	//	AHB prescaler, ставим SYSCLK not divided (0xxx)
//	RCC->CFGR |= (0x00<<RCC_CFGR_HPRE_Pos);
	SET_BIT(RCC->CFGR, RCC_CFGR_HPRE_DIV1);
	//	PCLK2 = HCLK
	//	APB2 prescaler (high-speed), ставим HCLK not divided (0xx)
//	RCC->CFGR |= (0x00<<RCC_CFGR_PPRE2_Pos);
	SET_BIT(RCC->CFGR, RCC_CFGR_PPRE2_DIV1);
	//	PCLK1 = HCLK
	//	APB1 prescaler (low-speed), ставим частоту HCLK/2 (100)
//	RCC->CFGR |= (0x04<<RCC_CFGR_PPRE1_Pos);
	SET_BIT(RCC->CFGR, RCC_CFGR_PPRE1_DIV2);

	//	настраиваем PLL
	//	сначала обнуляем содержимое RCC_CFGR_PLLSRC: 0
	CLEAR_BIT(RCC->CFGR, RCC_CFGR_PLLSRC);
	//	а также обнуляем содержимое RCC_CFGR_PLLMULL: 000x
	CLEAR_BIT(RCC->CFGR, RCC_CFGR_PLLMULL);
	//	затем устанавливаем флаг RCC_CFGR_PLLSRC: 1
	//	т.е. источник тактирования PLL - кварц HSE (на 8MHz)
	SET_BIT(RCC->CFGR, RCC_CFGR_PLLSRC);
	//	а также устанавливаем флаги
	//	(RCC_CFGR_PLLMULL_2 | RCC_CFGR_PLLMULL_1): 0110
	//	поскольку это множитель 8: PLLCLK = HSE * 8 = 64MHz
	SET_BIT(RCC->CFGR, RCC_CFGR_PLLMULL_2 | RCC_CFGR_PLLMULL_1);

	//	включаем PLL
//	RCC->CR |= (1<<RCC_CR_PLLON_Pos);
	SET_BIT(RCC->CR, RCC_CR_PLLON);
	//	ожидаем, пока PLL выставит бит готовности
	StartUpCounter = 0;
	do	{
		StartUpCounter++;
	}	while((READ_BIT(RCC->CR, RCC_CR_PLLRDY) == 0)
			&& (StartUpCounter != HSEStartUp_TimeOut));
	//	если выход произошел по счетчику, значит PLL не работает
	//	значит, отключаем бит RCC_CR_PLLON и возвращаем "2"
	if (StartUpCounter == HSEStartUp_TimeOut){
		//	останавливаем генератор (HSE)
//		RCC->CR &= ~(1<<RCC_CR_HSEON_Pos);
		//	останавливаем тактирование (PLL)
//		RCC->CR &= ~(1<<RCC_CR_PLLON_Pos);
		CLEAR_BIT(RCC->CR, RCC_CR_HSEON);
		CLEAR_BIT(RCC->CR, RCC_CR_PLLON);
		return (2);
	}

	//	выбираем PLL как источник системной частоты
	//	сбрасываем бит RCC_CFGR_SW
	//	хотя в этом вообще НЕТ необходимости
//	RCC->CFGR &= ~(RCC_CFGR_SW);
//	CLEAR_BIT(RCC->CFGR, RCC_CFGR_SW);
	//	устанавливаем бит RCC_CFGR_SW_1: 10
// 	RCC->CFGR |= (0x02<<RCC_CFGR_SW_Pos);
//	RCC->CFGR |= RCC_CFGR_SW_1;
	SET_BIT(RCC->CFGR, RCC_CFGR_SW_1);
//	ожидаем установки бита готовности RCC_CFGR_SWS
// 	while ((RCC->CFGR & RCC_CFGR_SWS) != (0x02<<RCC_CFGR_SWS_Pos)){}
	while (READ_BIT(RCC->CFGR, RCC_CFGR_SWS) != RCC_CFGR_SWS_1){}

	//	после переключения на тактирование от PLL
	//	отключаем внутренний RC-генератор (HSI)
//	RCC->CR &= ~(1<<RCC_CR_HSION_Pos);
//	CLEAR_BIT(RCC->CR, RCC_CR_HSION);

	return (0);
}
