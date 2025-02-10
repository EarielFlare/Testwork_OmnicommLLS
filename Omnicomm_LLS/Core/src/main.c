#include <main.h>

int main(void){
	//	ДЕинициализации HSE/PLL (переход на HSI)
	RCC_DeInit();
	//	инициализация HSE на частоте 64MHz
	//	если НЕ получилось включить тактирование 64MHz
	if (RCC_ReInit() != 0)
		return (0);

	UART_Init();	//	конфигурируем UART
	data_received_flag = 0;
	form_response(0xA5);	//	рандомная посылка-ответ

	while(1){
		if (data_received_flag == 1)
			process_uart_data();
	}
}
