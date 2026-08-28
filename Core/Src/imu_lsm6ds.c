/*
 * imu_lsm6ds_spi.c
 *
 *  Created on: Nov 28, 2024
 *      Author: fsy13
 *
 * SPI2 byte driver for the LSM6DS3TR-C.
 *
 *   Each register access on the LSM6 must look like:
 *       CS HIGH  →  CS LOW  →  16 SCK pulses  →  CS HIGH
 *
 *   With STM32 hardware-NSS-output and NSSP disabled, NSS only goes
 *   HIGH when SPE = 0.  HAL_SPI_Transmit / TransmitReceive enable SPI
 *   at the start of a call but never disable it at the end, so NSS
 *   stays LOW across consecutive calls and the LSM6 reads them as
 *   one long multi-byte transfer (with auto-increment via IF_INC).
 *
 *   We explicitly disable SPI after every byte transaction so NSS
 *   rises before the next call — that's what gives each register
 *   access a clean transaction boundary.
 */

#include "imu_lsm6ds.h"
#include "spi.h"

// Hardware configuration
#define spi_hal_handler hspi2

void imu_lsm6ds_write_byte(imu_register_t register_address, uint8_t value)
{
	uint8_t write_buff[2] = {0};
	write_buff[0] = value;
	write_buff[1] = register_address;

	// Send one word = 16 bits, MSB first
	HAL_SPI_Transmit(&spi_hal_handler, write_buff, 1, HAL_MAX_DELAY);

	// Force NSS HIGH so the next call begins a fresh LSM6 transaction
	__HAL_SPI_DISABLE(&spi_hal_handler);
}

uint8_t imu_lsm6ds_read_byte(imu_register_t register_address)
{
	// 16 bit transmission:
	// First byte is the register address on MOSI, with the read bit enabled.
	// Second byte is the data from slave on MISO.
	// Indexing in reverse order due to MSB first.

	uint8_t tx[2] = {0};
	uint8_t rx[2] = {0};

	tx[1] = register_address |= (1 << 7); // Set "Read" bit

	HAL_SPI_TransmitReceive(&spi_hal_handler, tx, rx, 1, HAL_MAX_DELAY);

	// Force NSS HIGH so the next call begins a fresh LSM6 transaction
	__HAL_SPI_DISABLE(&spi_hal_handler);

	return rx[0];
}
