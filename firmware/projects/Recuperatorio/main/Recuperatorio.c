/*! @mainpage Template
 *
 * @section genDesc General Description
 *
 * This section describes how the program works.
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	PIN_X	 	| 	GPIO_X		|
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 12/09/2023 | Document creation		                         |
 *
 * @author Jessenia J. Rojas Garrido (jrojasgarrido@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "timer_mcu.h"
#include "led.h"
#include <hc_sr04.h>
#include <uart_mcu.h>
/*==================[macros and definitions]=================================*/
/**
 * @def CONFIG_BLINK_PERIOD_TERMOPILA
 * @brief Periodo de adquisicion de la temperatura con la termopila
 */
#define CONFIG_BLINK_PERIOD_TERMOPILA 100
/**
* @def CONFIG_BLINK_PERIOD_MUESTREO
 * @brief Periodo de Muestreo de la señal
 */
#define CONFIG_BLINK_PERIOD_MUESTREO 1000
/**
 * @def ECHO GPIO_3
 * @brief CONFIGURACION DE ECHO
 */
#define ECHO GPIO_3
/**
 * @def TRIGGER GPIO_2
 * @brief CONFIGURACION DE TRIGGER
 */
#define TRIGGER GPIO_2
/*==================[internal data definition]===============================*/
uint8_t Dist, temperatura;
TaskHandle_t Temperatura_task_handle = NULL;
TaskHandle_t PrenderLed_task_handle = NULL;
TaskHandle_t Datos_task_handle = NULL;
/*==================[internal functions declaration]=========================*/
/**
 * @fn uint8_t MedirTemperatura(uint8_t tempEntrada)
 * @brief Funcion encargada de guardar la medicion de temperatura
 * @param [in] tempEntrada
 * @return temperatura
 */
uint8_t MedirTemperatura(uint8_t tempEntrada)
{
	Dist=HcSr04ReadDistanceInCentimeters();
	if (Dist > 8 && Dist < 12)
	{
		temperatura = tempEntrada * (3300 / 50);
	} 
	return temperatura;
}

/** @brief Tarea encargada de prender Leds */
void prenderLeds(void *param)
{
	while (1)
	{
		Dist=HcSr04ReadDistanceInCentimeters();
	if (Dist < 8)
	{
		LedOn(LED_1);
	}
	else if (Dist > 8 && Dist < 12)
	{
		LedOn(LED_2);
	}
	else if (Dist > 12)
	{
		LedOn(LED_3);
	}
	vTaskDelay(CONFIG_BLINK_PERIOD_MUESTREO / portTICK_PERIOD_MS);
	}
	
	
}

/** @brief Tarea encargada de tomar temperatura*/
void tomarTemperatura(uint8_t *tempEntrada)
{
	while (1)
	{
		uint16_t arreglo[10];
		uint8_t promedio = 0;
		Dist=HcSr04ReadDistanceInCentimeters();
		if (Dist<14){
			
		for (uint8_t i = 0; i < 10; i++)
		{
			arreglo[i] = MedirTemperatura(&tempEntrada);
		}
		for (uint8_t i = 0; i < 10; i++)
		{
			arreglo[i] = MedirTemperatura(&tempEntrada);
			promedio = (promedio + arreglo[i]) / 10;
		}
		} else break;
		vTaskDelay(CONFIG_BLINK_PERIOD_TERMOPILA / portTICK_PERIOD_MS);
	}
}

/** @brief Tarea encargada de enviar datos en serie*/
void EnviarDatos(void *param)
{
	while (1)
	{
		UartSendString(UART_PC, (char *)UartItoa(temperatura, 10));
		UartSendString(UART_PC, " Cº persona a ");
		UartSendString(UART_PC, (char *)UartItoa(Dist, 10));
		UartSendString(UART_PC, " cm\r\n");
		vTaskDelay(CONFIG_BLINK_PERIOD_MUESTREO / portTICK_PERIOD_MS);
	}
}

/*==================[external functions definition]==========================*/


void app_main(void)
{
	HcSr04Init(ECHO, TRIGGER);

	serial_config_t serial_salida = {
        .port = UART_PC,
        .baud_rate = 115200,
        .func_p = NULL,
        .param_p = NULL
    };

	UartInit(&serial_salida);
    xTaskCreate(&prenderLeds, "TAREA LEDS", 512, NULL, 5, &PrenderLed_task_handle);
	xTaskCreate(&tomarTemperatura, "TEMPERATURA", 512, NULL, 5, &Temperatura_task_handle);
	xTaskCreate(&EnviarDatos, "DATOS", 512, NULL, 5, &Datos_task_handle);
}
/*==================[end of file]============================================*/