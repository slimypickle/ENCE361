/*
 * i2c.h
 *
 * I2C1 + its TX DMA channel, used by the SSD1306 OLED driver.
 *
 * The handles themselves are defined in i2c.c. This header exists so
 * that other modules (in particular the IRQ vector file) can reach
 * them via a normal #include instead of redeclaring `extern` locally.
 */

#ifndef __I2C_H__
#define __I2C_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

extern I2C_HandleTypeDef hi2c1;
extern DMA_HandleTypeDef hdma_i2c1_tx;

void MX_I2C1_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* __I2C_H__ */
