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
 * pzem.c
 *
 * PZEM current sensor TTL driver.
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"

#include "pzem.h"

#define TAG "pzem"
#define BUF_SIZE (1024)

// {{{ crc16
static const uint16_t crcTable[] = {
  0X0000, 0XC0C1, 0XC181, 0X0140, 0XC301, 0X03C0, 0X0280, 0XC241,
  0XC601, 0X06C0, 0X0780, 0XC741, 0X0500, 0XC5C1, 0XC481, 0X0440,
  0XCC01, 0X0CC0, 0X0D80, 0XCD41, 0X0F00, 0XCFC1, 0XCE81, 0X0E40,
  0X0A00, 0XCAC1, 0XCB81, 0X0B40, 0XC901, 0X09C0, 0X0880, 0XC841,
  0XD801, 0X18C0, 0X1980, 0XD941, 0X1B00, 0XDBC1, 0XDA81, 0X1A40,
  0X1E00, 0XDEC1, 0XDF81, 0X1F40, 0XDD01, 0X1DC0, 0X1C80, 0XDC41,
  0X1400, 0XD4C1, 0XD581, 0X1540, 0XD701, 0X17C0, 0X1680, 0XD641,
  0XD201, 0X12C0, 0X1380, 0XD341, 0X1100, 0XD1C1, 0XD081, 0X1040,
  0XF001, 0X30C0, 0X3180, 0XF141, 0X3300, 0XF3C1, 0XF281, 0X3240,
  0X3600, 0XF6C1, 0XF781, 0X3740, 0XF501, 0X35C0, 0X3480, 0XF441,
  0X3C00, 0XFCC1, 0XFD81, 0X3D40, 0XFF01, 0X3FC0, 0X3E80, 0XFE41,
  0XFA01, 0X3AC0, 0X3B80, 0XFB41, 0X3900, 0XF9C1, 0XF881, 0X3840,
  0X2800, 0XE8C1, 0XE981, 0X2940, 0XEB01, 0X2BC0, 0X2A80, 0XEA41,
  0XEE01, 0X2EC0, 0X2F80, 0XEF41, 0X2D00, 0XEDC1, 0XEC81, 0X2C40,
  0XE401, 0X24C0, 0X2580, 0XE541, 0X2700, 0XE7C1, 0XE681, 0X2640,
  0X2200, 0XE2C1, 0XE381, 0X2340, 0XE101, 0X21C0, 0X2080, 0XE041,
  0XA001, 0X60C0, 0X6180, 0XA141, 0X6300, 0XA3C1, 0XA281, 0X6240,
  0X6600, 0XA6C1, 0XA781, 0X6740, 0XA501, 0X65C0, 0X6480, 0XA441,
  0X6C00, 0XACC1, 0XAD81, 0X6D40, 0XAF01, 0X6FC0, 0X6E80, 0XAE41,
  0XAA01, 0X6AC0, 0X6B80, 0XAB41, 0X6900, 0XA9C1, 0XA881, 0X6840,
  0X7800, 0XB8C1, 0XB981, 0X7940, 0XBB01, 0X7BC0, 0X7A80, 0XBA41,
  0XBE01, 0X7EC0, 0X7F80, 0XBF41, 0X7D00, 0XBDC1, 0XBC81, 0X7C40,
  0XB401, 0X74C0, 0X7580, 0XB541, 0X7700, 0XB7C1, 0XB681, 0X7640,
  0X7200, 0XB2C1, 0XB381, 0X7340, 0XB101, 0X71C0, 0X7080, 0XB041,
  0X5000, 0X90C1, 0X9181, 0X5140, 0X9301, 0X53C0, 0X5280, 0X9241,
  0X9601, 0X56C0, 0X5780, 0X9741, 0X5500, 0X95C1, 0X9481, 0X5440,
  0X9C01, 0X5CC0, 0X5D80, 0X9D41, 0X5F00, 0X9FC1, 0X9E81, 0X5E40,
  0X5A00, 0X9AC1, 0X9B81, 0X5B40, 0X9901, 0X59C0, 0X5880, 0X9841,
  0X8801, 0X48C0, 0X4980, 0X8941, 0X4B00, 0X8BC1, 0X8A81, 0X4A40,
  0X4E00, 0X8EC1, 0X8F81, 0X4F40, 0X8D01, 0X4DC0, 0X4C80, 0X8C41,
  0X4400, 0X84C1, 0X8581, 0X4540, 0X8701, 0X47C0, 0X4680, 0X8641,
  0X8201, 0X42C0, 0X4380, 0X8341, 0X4100, 0X81C1, 0X8081, 0X4040
};

/*
 * Calculate the CRC16-Modbus for a buffer
 * Based on https://www.modbustools.com/modbus_crc16.html
 */
static uint16_t crc16(const uint8_t *data, uint16_t len) {
  uint8_t nTemp; // CRC table index
  uint16_t crc = 0xFFFF; // Default value

  while (len--) {
    nTemp = *data++ ^ crc;
    crc >>= 8;
    crc ^= (uint16_t)crcTable[nTemp];
  }
  return crc;
}

static bool check_crc(const uint8_t *buf, uint16_t len) {
  if(len <= 2) // Sanity check
    return false;

  uint16_t crc = crc16(buf, len - 2); // Compute CRC of data
  return ((uint16_t)buf[len-2]  | (uint16_t)buf[len-1] << 8) == crc;
}

static void set_crc(uint8_t *buf, uint16_t len){
  if(len <= 2) // Sanity check
    return;

  uint16_t crc = crc16(buf, len - 2); // CRC of data

  // Write high and low byte to last two positions
  buf[len - 2] = crc & 0xFF; // Low byte first
  buf[len - 1] = (crc >> 8) & 0xFF; // High byte second
}
// }}}

// {{{ uart routines
#ifdef CONFIG_PZEM_DEBUG_ENABLE
static void printbuf(uint8_t* buffer, uint16_t len) {
  for(uint16_t i = 0; i < len; i++){
    printf("%.2x ", buffer[i]);
  }
}
#endif

static uint16_t receive(uint8_t *resp, uint16_t len) {
  uint8_t c[1];
  uint8_t index = 0; // Bytes we have read

  while (index < len) {
    if (uart_read_bytes(CONFIG_PZEM_UART_PORT_NUM, c, 1, 100 / portTICK_PERIOD_MS) > 0) {
      resp[index++] = c[0];
    } else {
      break;
    }
  }

#ifdef CONFIG_PZEM_DEBUG_ENABLE
  printf("recv: ");
  printbuf(resp, len);
  printf("\n");
#endif

  if(!check_crc(resp, index))
    return 0;

  return index;
}

static bool sendcmd(uint8_t cmd, uint16_t rAddr, uint16_t val,
    bool check, uint16_t slave_addr) {

  uint8_t sendBuffer[8] = {} ; // Send buffer
  uint8_t respBuffer[8] = {} ; // Response buffer (only used when check is true)

  sendBuffer[0] = slave_addr;                   // Set slave address
  sendBuffer[1] = cmd;                     // Set command

  sendBuffer[2] = (rAddr >> 8) & 0xFF;     // Set high byte of register address
  sendBuffer[3] = (rAddr) & 0xFF;          // Set low byte =//=

  sendBuffer[4] = (val >> 8) & 0xFF;       // Set high byte of register value
  sendBuffer[5] = (val) & 0xFF;            // Set low byte =//=

  set_crc(sendBuffer, 8);                   // Set CRC of frame

  uart_write_bytes(CONFIG_PZEM_UART_PORT_NUM, (const char *) sendBuffer, 8);

#ifdef CONFIG_PZEM_DEBUG_ENABLE
  printf("sent: ");
  printbuf(sendBuffer, 8);
  printf("\n");
#endif

  if (check && !receive(respBuffer, 8)) { // if check enabled, read the response
    return false;
  }

  return true;
}
// }}}

int pzem_sensor_init() {
  uart_config_t uart_config = {
    .baud_rate = 9600,
    .data_bits = UART_DATA_8_BITS,
    .parity    = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_DEFAULT,
  };

  int intr_alloc_flags = 0;

#if CONFIG_UART_ISR_IN_IRAM
  intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif

  ESP_ERROR_CHECK(uart_driver_install(
    CONFIG_PZEM_UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));

  ESP_ERROR_CHECK(uart_param_config(CONFIG_PZEM_UART_PORT_NUM, &uart_config));

  ESP_ERROR_CHECK(uart_set_pin(CONFIG_PZEM_UART_PORT_NUM,
        CONFIG_PZEM_UART_TXD, CONFIG_PZEM_UART_RXD, -1, -1));

  return ESP_OK;
}

void pzem_sensor_request(pzem_sensor_t *pzem_sensor, uint8_t addr) {
  static uint8_t response[25];

  sendcmd(CMD_RIR, 0x00, 0x0A, false, addr);

  if(receive(response, 25) != 25){ // Something went wrong
    printf("Error.\n");
    return;
  }

  // Update the current values
  pzem_sensor->voltage = ((uint32_t)response[3] << 8 | // Raw voltage in 0.1V
                              (uint32_t)response[4])/10.0;

  pzem_sensor->current = ((uint32_t)response[5] << 8 | // Raw current in 0.001A
                              (uint32_t)response[6] |
                              (uint32_t)response[7] << 24 |
                              (uint32_t)response[8] << 16) / 1000.0;

  pzem_sensor->power =   ((uint32_t)response[9] << 8 | // Raw power in 0.1W
                              (uint32_t)response[10] |
                              (uint32_t)response[11] << 24 |
                              (uint32_t)response[12] << 16) / 10.0;

  pzem_sensor->energy =  ((uint32_t)response[13] << 8 | // Raw Energy in 1Wh
                              (uint32_t)response[14] |
                              (uint32_t)response[15] << 24 |
                              (uint32_t)response[16] << 16) / 1000.0;

  pzem_sensor->frequency=((uint32_t)response[17] << 8 | // Raw Frequency in 0.1Hz
                              (uint32_t)response[18]) / 10.0;

  pzem_sensor->pf =      ((uint32_t)response[19] << 8 | // Raw pf in 0.01
                              (uint32_t)response[20])/100.0;

  pzem_sensor->alarms =  ((uint32_t)response[21] << 8 | // Raw alarm value
                              (uint32_t)response[22]);

#ifdef CONFIG_PZEM_DEBUG_ENABLE
  printf("\n");
  printf("Voltage:\t%fV\n", pzem_sensor->voltage);
  printf("Current:\t%fA\n", pzem_sensor->current);
  printf("Frequency:\t%fHz\n", pzem_sensor->frequency);
  printf("Power:\t\t%fW\n", pzem_sensor->power);
  printf("Energy:\t\t%fWh\n", pzem_sensor->energy);
  printf("pf:\t\t%f\n", pzem_sensor->pf);
  printf("alarms:\t\t%u\n", pzem_sensor->alarms);
  printf("\n");
#endif
}

int pzem_reset(uint8_t addr) {
  uint8_t buffer[] = {0x00, CMD_REST, 0x00, 0x00};
  uint16_t length = 0;
  uint8_t reply[5];

  buffer[0] = addr;
  set_crc(buffer, 4);
  uart_write_bytes(CONFIG_PZEM_UART_PORT_NUM, (const char *) buffer, 4);
  printf("reset sent\n");
  length = receive(reply, 5);

  if(length == 0 || length == 5){
    ESP_LOGE(TAG, "Reset energy for 0x%02x failed\n", addr);
    return ESP_FAIL;
  }

  return ESP_OK;
}
