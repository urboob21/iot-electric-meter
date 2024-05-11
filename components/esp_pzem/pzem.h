/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  Author: hakanai
 *
 * pzem.h
 *
# ESPZEM Sensor Driver

## Features

    * Read sensor specific sensor values
    * Reset energy counter for a specific address

## Usage

  To minize number of allocations, `espzem` shares `pzem_sensor` structure
  globally, its values could be easily read. The difinition looks like:

  ```
  typedef struct pzem_sensor_t {
    float voltage;
    float current;
    float power;
    float energy;
    float frequency;
    float pf;
    uint16_t alarms;
  } pzem_sensor_t;
  ```

  First of all the driver have to be properly configured using following 
  options:

    * config `PZEM_UART_PORT_NUM` - Number of device's uart port (for an instance: 2)
    * config `PZEM_UART_RXD` and `PZEM_UART_RXD`
    * config `PZEM_DEBUG_ENABLE` enables debug output and embedds additional code.

## API

  ```
  int pzem_sensor_init();                   // Initializes the UART port
  void pzem_sensor_request(uint8_t addr);   // Read values into `pzem_sensor`
  int pzem_reset(uint8_t addr);             // Resets energy counter
  ```
 */

// config
#define CONFIG_PZEM_UART_PORT_NUM 0
#define CONFIG_PZEM_UART_RXD 3
#define CONFIG_PZEM_UART_TXD 1
#define CONFIG_PZEM_DEBUG_ENABLE 0

#define REG_VOLTAGE     0x0000
#define REG_CURRENT_L   0x0001
#define REG_CURRENT_H   0X0002
#define REG_POWER_L     0x0003
#define REG_POWER_H     0x0004
#define REG_ENERGY_L    0x0005
#define REG_ENERGY_H    0x0006
#define REG_FREQUENCY   0x0007
#define REG_PF          0x0008
#define REG_ALARM       0x0009

#define PZEM_DEFAULT_ADDR   0xF8

#define CMD_RHR         0x03
#define CMD_RIR         0X04
#define CMD_WSR         0x06
#define CMD_CAL         0x41
#define CMD_REST        0x42

#define WREG_ALARM_THR   0x0001
#define WREG_ADDR        0x0002

typedef struct pzem_sensor_t {
  float voltage;
  float current;
  float power;
  float energy;
  float frequency;
  float pf;
  uint16_t alarms;
} pzem_sensor_t;

int pzem_sensor_init();
void pzem_sensor_request(pzem_sensor_t *pzem_sensor, uint8_t addr);
int pzem_reset(uint8_t addr);
