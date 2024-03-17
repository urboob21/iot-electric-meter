/**
 * @file MQ2.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "tasks_common.h"
#include "mqtt_app.h"
#include "lcd2004_app.h"
#include "mq2.h"
#include "string.h"
#include "gpio_app.h"

extern int g_mqtt_connect_status;

/*****************************Globals***********************************************/
float LPGCurve[3] = {2.3, 0.21, -0.47}; // two points are taken from the curve.
                                        // with these two points, a line is formed which is "approximately equivalent"
                                        // to the original curve.
                                        // data format:{ x, y, slope}; point1: (lg200, 0.21), point2: (lg10000, -0.59)
float COCurve[3] = {2.3, 0.72, -0.34};
;                                         // two points are taken from the curve.
                                          // with these two points, a line is formed which is "approximately equivalent"
                                          // to the original curve.
                                          // data format:{ x, y, slope}; point1: (lg200, 0.72), point2: (lg10000,  0.15)
float SmokeCurve[3] = {2.3, 0.53, -0.44}; // two points are taken from the curve.
                                          // with these two points, a line is formed which is "approximately equivalent"
                                          // to the original curve.
                                          // data format:{ x, y, slope}; point1: (lg200, 0.53), point2: (lg10000,  -0.22)
float Ro = 10;                            // Ro is initialized to 10 kilo ohms

float lpg = 0;
float co = 0;
float smoke = 0;
int lastReadTime = 0;
/**
 * CONNECTION:
 *
 * ----------------------------------------------------------
 *              MQ2 Sensor                ESP32
 * ----------------------------------------------------------
 *                  Vin                    3.3
 *                  GND                    GND
 *                Output Pin               25 (Channel 8)
 * -----------------------------------------------------------
 */

/* GPIO 25 For channel 8 --> gas sensor analog pin */
static const adc_channel_t channel = MQ2_PIN_CHANEL;

/* ADC capture width is 10 (Range 0-1023) */
static const adc_bits_width_t width = 10;

/* No input attenumation, ADC can measure up to approx. 800 mV. */
static const adc_atten_t atten = ADC_ATTEN_DB_11;

static esp_adc_cal_characteristics_t *adc_chars;

void begin()
{
    Ro = MQCalibration();
    printf("Ro: %.2lf Kohm\n", Ro);
}

float getCop()
{
    return co;
}
/***************************** MQCalibration ****************************************
Input:   mq_pin - analog channel
Output:  Ro of the sensor
Remarks: This function assumes that the sensor is in clean air. It use
         MQResistanceCalculation to calculates the sensor resistance in clean air
         and then divides it with RO_CLEAN_AIR_FACTOR. RO_CLEAN_AIR_FACTOR is about
         10, which differs slightly between different sensors.
************************************************************************************/

float MQCalibration()
{

    float val = 0;
    adc1_config_width(width);
    adc1_config_channel_atten(channel, atten);
    for (int i = 0; i < CALIBARAION_SAMPLE_TIMES; i++)
    {
        /* take multiple samples */
        int caliread = adc1_get_raw(channel);
        val += MQResistanceCalculation(caliread);
        printf("Sample: %d : Calibration ADC :%d: MQResistanceCalculation: %.4f:\n", i, caliread, val);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    val = val / CALIBARAION_SAMPLE_TIMES; // calculate the average value

    val = val / RO_CLEAN_AIR_FACTOR; // divided by RO_CLEAN_AIR_FACTOR yields the Ro
                                     // according to the chart in the datasheet

    printf("MQ2 Calibration value: %f\n", val);

    return val;
}

/****************** MQResistanceCalculation ****************************************
Input:   raw_adc - raw value read from adc, which represents the voltage
Output:  the calculated sensor resistance
Remarks: The sensor and the load resistor forms a voltage divider. Given the voltage
         across the load resistor and its resistance, the resistance of the sensor
         could be derived.
************************************************************************************/
float MQResistanceCalculation(int raw_adc)
{
    return (((float)RL_VALUE * (1023 - raw_adc) / raw_adc));
}

/****************** read infinite times ****************************************/
float *read(bool print)
{

    lpg = MQGetGasPercentage(MQRead() / Ro, GAS_LPG);
    co = MQGetGasPercentage(MQRead() / Ro, GAS_CO);
    smoke = MQGetGasPercentage(MQRead() / Ro, GAS_SMOKE);

    if (lpg > 1000 || co > 1000 || smoke > 1000)
    {
        printf("[ GAS DETECTTED ]\n");
        printf("LPG::%.2f::ppm CO::%.2f::ppm SMOKE::%.2f::ppm \n", lpg, co, smoke);
    }
    else
    {
        printf("XXX [ GAS NOT DETECTED ] XXX\n");
        printf("LPG::%.2f::ppm CO::%.2f::ppm SMOKE::%.2f::ppm \n", lpg, co, smoke);
    }
    lastReadTime = xthal_get_ccount();
    float *values = (float *)malloc(3 * sizeof(float));
    if (values == NULL)
    {
        // Handle memory allocation failure
        return NULL;
    }
    values[0] = lpg;
    values[1] = co;
    values[2] = smoke;
    return values;
}

/*****************************  MQRead *********************************************
Input:   mq_pin - analog channel
Output:  Rs of the sensor
Remarks: This function use MQResistanceCalculation to caculate the sensor resistenc (Rs).
         The Rs changes as the sensor is in the different consentration of the target
         gas. The sample times and the time interval between samples could be configured
         by changing the definition of the macros.
************************************************************************************/
float MQRead()
{
    int i;
    float rs = 0;
    adc1_config_width(width);
    adc1_config_channel_atten(channel, atten);

    int val = adc1_get_raw(channel);
    uint32_t voltage = esp_adc_cal_raw_to_voltage(val, adc_chars);

    for (i = 0; i < READ_SAMPLE_TIMES; i++)
    {
        rs += MQResistanceCalculation(val);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    rs = rs / READ_SAMPLE_TIMES;
    return rs;
}

/*****************************  MQGetGasPercentage **********************************
Input:   rs_ro_ratio - Rs divided by Ro
         gas_id      - target gas type
Output:  ppm of the target gas
Remarks: This function passes different curves to the MQGetPercentage function which
         calculates the ppm (parts per million) of the target gas.
************************************************************************************/

float MQGetGasPercentage(float rs_ro_ratio, int gas_id)
{
    if (gas_id == GAS_LPG)
    {
        return MQGetPercentage(rs_ro_ratio, LPGCurve);
    }
    else if (gas_id == GAS_CO)
    {
        return MQGetPercentage(rs_ro_ratio, COCurve);
    }
    else if (gas_id == GAS_SMOKE)
    {
        return MQGetPercentage(rs_ro_ratio, SmokeCurve);
    }
    return 0;
}

/*****************************  MQGetPercentage **********************************
Input:   rs_ro_ratio - Rs divided by Ro
         pcurve      - pointer to the curve of the target gas
Output:  ppm of the target gas
Remarks: By using the slope and a point of the line. The x(logarithmic value of ppm)
         of the line could be derived if y(rs_ro_ratio) is provided. As it is a
         logarithmic coordinate, power of 10 is used to convert the result to non-logarithmic
         value.
************************************************************************************/
int MQGetPercentage(float rs_ro_ratio, float *pcurve)
{
    return (pow(10, (((log(rs_ro_ratio) - pcurve[1]) / pcurve[2]) + pcurve[0])));
}

/*****************************  readLPG Value --every 10sec interval **********************************/
float readLPG()
{
    if (xthal_get_ccount() < (lastReadTime + 10000) && lpg != 0)
    {
        return lpg;
    }
    else
    {
        return lpg = MQGetGasPercentage(MQRead() / 10, GAS_LPG);
    }
}

/*****************************  readCO Value -- every 10sec interval **********************************/
float readCO()
{
    if (xthal_get_ccount() < (lastReadTime + 10000) && co != 0)
    {
        return co;
    }
    else
    {
        return co = MQGetGasPercentage(MQRead() / 10, GAS_CO);
    }
}
/*****************************  readSmoke Value -- every 10sec interval **********************************/

float readSmoke()
{
    if (xthal_get_ccount() < (lastReadTime + 10000) && smoke != 0)
    {
        return smoke;
    }
    else
    {
        return smoke = MQGetGasPercentage(MQRead() / 10, GAS_SMOKE);
    }
}

static void mq_app_task()
{
    for (;;)
    {
        adc_chars = (esp_adc_cal_characteristics_t *)malloc(sizeof(esp_adc_cal_characteristics_t));

        // lpg = MQGetGasPercentage(MQRead() / Ro, GAS_LPG);
        // smoke = MQGetGasPercentage(MQRead() / Ro, GAS_SMOKE);
        co = MQGetGasPercentage(MQRead() / Ro, GAS_CO);

        // turn warning if co tobe high
        if (co > 50)
        {
            gpio_app_turn_warning(true);
        }

        // printf("LPG::%.2f::ppm CO::%.2f::ppm SMOKE::%.2f::ppm \n", lpg, co, smoke);
        lastReadTime = xthal_get_ccount();

        // Public to mqtt if connect success
        if (g_mqtt_connect_status == MQTT_APP_CONNECT_SUCCESS)
        {
            if (co < 10000)
            {
                // // send topic data 
                // char coBuff[10];
                // sprintf(coBuff, "%d", (int)co);

                int length = snprintf(NULL, 0, "%d", (int) co);
                // Cấp phát bộ nhớ động cho chuỗi
                 char *buffer = (char *)malloc((length + 1) * sizeof(char)); // +1 cho ký tự kết thúc null
                 // Chuyển số nguyên thành chuỗi
                snprintf(buffer, length + 1, "%d", ((int)co));

                mqtt_app_send_message_with(MQTT_APP_MSG_PUBLISHED, MQTT_APP_TOPIC_PUB_CO, strlen(MQTT_APP_TOPIC_PUB_CO), buffer, length);
                free(buffer);
            }
            else
            {
                // send topic data + on warning
                mqtt_app_send_message_with(MQTT_APP_MSG_PUBLISHED, MQTT_APP_TOPIC_PUB_CO, strlen(MQTT_APP_TOPIC_PUB_CO), "10000", 5);
            }

            if(co > 40){
                mqtt_app_send_message(MQTT_APP_MSG_ONWARNING);
            }
        }
        free(adc_chars);
        adc_chars = NULL;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void mq2_app_main()
{
    begin();
    // Start the WIFI Application task
    xTaskCreatePinnedToCore(&mq_app_task, "MQ_APP_TASK",
                            WIFI_APP_TASK_STACK_SIZE, NULL, WIFI_APP_TASK_PRIORITY, NULL,
                            1);
}