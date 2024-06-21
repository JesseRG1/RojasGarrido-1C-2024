/*! @mainpage Proyecto Integrador
 *
 * @section AUDIOMETRO
 *
 * El objetivo del siguiente programa es tomar distintas mediciones de amplitudes a diferentes frecuencias
 * para poder realizar la grafica audiometrica correspondiente
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	PIN_X	 	| 	GPIO_0		|
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 22/05/2024 | Proyecto Integrador                        |
 *
 * @author Jessenia Jazmin Rojas Garrido(jrojasgarrido@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "analog_io_mcu.h"
#include "uart_mcu.h"
#include "timer_mcu.h"
#include "neopixel_stripe.h"
#include "ble_mcu.h"
#include "led.h"
#include "switch.h"
/*==================[macros and definitions]=================================*/
/**
 * @def BUFFER_SIZE
 * @brief Tamaño de datos que corresponden a la señal senoidal
 */
#define BUFFER_SIZE 64
/**
 * @def CONFIG_BLINK_PERIOD_TIMER
 * @brief Periodo de timer del programa
 */
#define CONFIG_BLINK_PERIOD_TECLAS 300000
/**
 * @def CONFIG_BLINK_PERIOD_SENIAL
 * @brief Periodo de timer de la senial enviada
 */
#define CONFIG_BLINK_PERIOD_TIMER_SENIAL 125000

#define LED_BT LED_1

/*==================[internal data definition]===============================*/
/** llamada a tarea Modificar señal*/
TaskHandle_t Modificar_task_handle = NULL;
/** llamada a tarea Mandar señal */
TaskHandle_t MandarSenial_task_handle = NULL;
/** arreglo que almacena una señal senoidal */
uint8_t sine_wave[BUFFER_SIZE] = {
    128, 140, 153, 165, 177, 188, 199, 209,
    218, 226, 234, 240, 246, 251, 254, 255,
    255, 254, 251, 246, 240, 234, 226, 218,
    209, 199, 188, 177, 165, 153, 140, 128,
    116, 103, 91, 79, 67, 56, 45, 35,
    26, 18, 12, 7, 3, 1, 0, 0,
    1, 3, 7, 12, 18, 26, 35, 45,
    56, 67, 79, 91, 103, 116, 128};
/** arreglos necesarios para las tareas */
uint16_t frecuencia[7] = {
    125, 250, 500, 1000,
    2000, 4000, 8000};
float Volumen_OI[7];
/** variables enteras para el uso de los programas*/
uint8_t periodo = CONFIG_BLINK_PERIOD_TIMER_SENIAL, punto = 0;
float volumen = 0.1;
timer_config_t timer_Senial;
/*==================[internal functions declaration]=========================*/
/**
 * @fn void GuardarMedicion(void)
 * @brief Funcion encargada de guardar la medicion
 * @param [in]
 * @return
 */
void GuardarMedicion(void)
{
    Volumen_OI[punto] = volumen;
    punto++;
    if (punto == 7)
    {
        char msg[48];
        for (int16_t i = 0; i < 7; i++)
        {
            /* Formato de datos para que sean graficados en la aplicación móvil */
            sprintf(msg, "*PX%dY%.2f*\n", frecuencia[i], Volumen_OI[i]);
            BleSendString(msg);
        }
    }
} // ver de guardar datos con las teclas de la placa o el bluetooth

/**
 * @fn void ModificarPeriodo(void)
 * @brief Funcion encargada de modificar el periodo
 * @param [in]
 * @return
 */
void ModificarPeriodo()
{

    periodo = periodo / 2; // la señal disminuye su aMplitud, por el filtro que el dac tiene a la salida
    volumen = volumen * 2; // le multiplico x2 la señal compensando el filtro del dac

    TimerStop(timer_Senial.timer);
    timer_Senial.period = periodo;
    TimerInit(&timer_Senial);
    TimerStart(timer_Senial.timer);
    printf("periodo,%d", periodo);
    // frecuencia=(1/periodo)*1000000*64;//(muestras)
}

/**
 * @fn void ModificarVolumen(void)
 * @brief Funcion encargada de modificar la amplitud (volumen)
 * @param [in]
 * @return
 */
void ModificarVolumen(float volumen)
{
    if (volumen < 1)
        volumen = 0.1 + volumen;
    // printf ("%d\r",volumen);
}

/** @brief Tarea encargada de mandar la señal */
void MandarSenial(void *param)
{
    uint8_t i = 0;
    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        i++;
        AnalogOutputWrite(volumen * sine_wave[i]);
        if (i == BUFFER_SIZE)
            i = 0;
        // printf("%.2f\r\n", volumen * sine_wave[i]);
    }
}

/** @brief Tarea encargada de la funcion de las teclas */
void Teclas(void *param)
{
    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        uint8_t teclas;
        teclas = SwitchesRead();
        switch (teclas)
        {
        case SWITCH_1:
            GuardarMedicion();
            ModificarPeriodo();
            printf("se modifico periodo,%d\n\r", periodo);
            break;
        case SWITCH_2:
            volumen = 0.1 + volumen;
            ModificarVolumen(volumen);
            printf("se modifico volumen %.2f\n\r", volumen);
            break;
        }
    }
}
/*void BotonBluetooth(uint8_t *data, uint8_t lenght)
{
    switch (data[0])
    {
    case 'B':
        GuardarMedicion();
        break;
    case 'b':
        break;
    }
}*/

/** @brief Tarea encargada del timer de la señal */
void FuncTimerSenial(void *param)
{
    xTaskNotifyGive(MandarSenial_task_handle);
}
/** @brief Tarea encargada del timer de las teclas */
void FuncTimerTeclas(void *param)
{
    xTaskNotifyGive(Modificar_task_handle);
}

/*==================[external functions definition]==========================*/
void app_main(void)
{
    // se inicializa el bluetooth
    ble_config_t ble_configuration = {
        "AUDIOMETRIA",
       //BotonBluetooth
            NULL};
    static neopixel_color_t color;
    LedsInit();
    BleInit(&ble_configuration);
    /* Se inicializa el LED RGB de la placa */
    NeoPixelInit(BUILT_IN_RGB_LED_PIN, BUILT_IN_RGB_LED_LENGTH, &color);

    // Inicializamos Timers
    timer_config_t timer_teclas = {
        .timer = TIMER_A,
        .period = CONFIG_BLINK_PERIOD_TECLAS,
        .func_p = FuncTimerTeclas,
        .param_p = NULL};

    periodo = CONFIG_BLINK_PERIOD_TIMER_SENIAL;
    timer_Senial.timer = TIMER_B,
    timer_Senial.period = CONFIG_BLINK_PERIOD_TECLAS,
    timer_Senial.func_p = FuncTimerSenial,
    timer_Senial.param_p = NULL;

    TimerInit(&timer_teclas);
    TimerInit(&timer_Senial);

    // Inicializamos salida Analogica
    AnalogOutputInit();

    // inicilizamos Switches
    SwitchesInit();

    xTaskCreate(&MandarSenial, "señal", 115200, NULL, 5, &MandarSenial_task_handle);
    xTaskCreate(&Teclas, "Modificar Señal", 115200, NULL, 5, &Modificar_task_handle);

    TimerStart(timer_teclas.timer);
    TimerStart(timer_Senial.timer);
    printf("inicio \r\n");
    NeoPixelAllOff();

    // CONFIGURACION DE ENCENDIDO Y APAGADO DEL BLUETOOTH
    while (1)
    {
        vTaskDelay(CONFIG_BLINK_PERIOD_TIMER_SENIAL / portTICK_PERIOD_MS);
        switch (BleStatus())
        {
        case BLE_OFF:
            LedOff(LED_BT);
            break;
        case BLE_DISCONNECTED:
            LedToggle(LED_BT);
            break;
        case BLE_CONNECTED:
            LedOn(LED_BT);
            break;
        }
    }
}
/*==================[end of file]============================================*/