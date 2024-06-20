/*! @mainpage Proyecto Integrador
 *
 * @section Audiometro
 *
 * El objetivo del siguiente programa es tomar distintas mediciones de frecuencias a diferentes amplitudes
 * para poder realizar la grafica audiometrica correspondiente
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
#define CHUNK 4
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
float volumen_audiometria[7];
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
    volumen_audiometria[punto] = volumen;
    punto++;
    if (punto == 7)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        char msg[128];
        static uint8_t indice = 0;
        for (uint8_t i = 0; i < CHUNK; i++)
        {
            sprintf(msg, "*P%.2f*", volumen_audiometria[i]);
            BleSendString(msg);
        }
        BleSendString(msg);
    }

} // ver de guardar datos con las teclas de la placa o el bluetooth

/**
 * @fn void ModificarPeriodo(void)
 * @brief Funcion encargada de modificar el periodo
 * @param [in]
 * @return
 */
void ModificarPeriodo(void)
{

    periodo = periodo / 2;
    // la señal disminuye su aplitud, por el filtro que el dac tiene a la salida
    // volumen = 0.1; // reinicia el volumen en cada frecuencia

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
            //GuardarMedicion();
            ModificarPeriodo();
            printf("se modifico periodo,%d\n\r", periodo);
            break;
        case SWITCH_2:
            volumen = 0.1 + volumen;
            ModificarVolumen(volumen);
            printf("se modifico volumen %.2f\n\r", volumen);
            break;
        }

    } // ver de hacer en los switches del bluetooth
}
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
    ble_config_t ble_configuration = {
        "AUDIOMETRIA",
        NULL};
    static neopixel_color_t color;
    LedsInit();
    BleInit(&ble_configuration);
    /* Se inicializa el LED RGB de la placa */
    NeoPixelInit(BUILT_IN_RGB_LED_PIN, BUILT_IN_RGB_LED_LENGTH, &color);
    NeoPixelAllOff();

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

    AnalogOutputInit();
    SwitchesInit();

    xTaskCreate(&MandarSenial, "señal", 4096, NULL, 5, &MandarSenial_task_handle);  // lo unico que anda
    xTaskCreate(&Teclas, "Modificar Señal", 4096, NULL, 5, &Modificar_task_handle); // ahora tambien anda
    TimerStart(timer_teclas.timer);
    TimerStart(timer_Senial.timer);
    printf("inicio \r\n");

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