#ifndef MAIN_MQ2_AP_H
#define MAIN_MQ2_AP_H

#define MQ2_PIN_CHANEL              3          //PIN 25       

/************************Hardware Related Macros************************************/
#define         RL_VALUE                     5     //define the load resistance on the board, in kilo ohms
#define         RO_CLEAN_AIR_FACTOR          9.83  //RO_CLEAR_AIR_FACTOR=(Sensor resistance in clean air)/RO,

/***********************Software Related Macros************************************/
#define         CALIBARAION_SAMPLE_TIMES     50   //define how many samples you are going to take in the calibration phase
#define         CALIBRATION_SAMPLE_INTERVAL  500   //define the time interal(in milisecond) between each samples in the
                                                     //cablibration phase
#define         READ_SAMPLE_INTERVAL         50    //define how many samples you are going to take in normal operation
#define         READ_SAMPLE_TIMES            5     //define the time interal(in milisecond) between each samples in 
                                                     //normal operation

/**********************Application Related Macros**********************************/
#define         GAS_LPG                      0
#define         GAS_CO                       1
#define         GAS_SMOKE                    2



/**********************************************function-define-----------------------------*/
float* read(bool print);

float readLPG();
float readCO();
float readSmoke();
void mq2_app_main();
float getCop();

float MQRead();
float MQGetGasPercentage(float rs_ro_ratio, int gas_id);
int MQGetPercentage(float rs_ro_ratio, float *pcurve);
float MQCalibration();
float MQResistanceCalculation(int raw_adc);



#endif