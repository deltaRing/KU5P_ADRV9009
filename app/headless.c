/**
 * \file headless.c
 *
 * \brief Contains example code for user integration with their application
 *
 * Copyright 2015-2017 Analog Devices Inc.
 * Released under the AD9378-AD9379 API license, for more information see the "LICENSE.txt" file in this zip file.
 *
 */

/****< Insert User Includes Here >***/
#include <stdio.h>
#include "../devices/talise/talise.h"
#include "../devices/talise/talise_tx.h"
#include "../devices/talise/talise_gpio.h"
#include "../devices/adi_hal/adi_hal.h"
#include "../devices/adi_hal/parameters.h"
#include "../devices/adi_hal/io_control.h"
#include "../devices/adi_hal/spi_ctrl.h"
#include "rf_config.h"
#include "xil_exception.h"
#include "../platform.h"
#include "xil_printf.h"

#ifdef _XPARAMETERS_PS_H_
#include "xscugic.h"
#else
#include "xintc.h"
#endif

/**********************************************************/
/**********************************************************/
/********** Talise Data Structure Initializations ********/
uint32_t axi_lite_addr = XPAR_UP_AXI_0_BASEADDR;
struct spi_desc *spi_desc_AD9528;
extern spi_init_param spi_AD9528_param;
uint32_t bb_freq;
uint32_t rf_freq;
uint32_t dived=1;

/******************************************************************************/
/********************** Macros and Constants Definitions **********************/
/******************************************************************************/
uint32_t jesd_phy0_addr = XPAR_JESD204_PHY_0_BASEADDR;
struct adi_hal hal_rf1;
taliseDevice_t talDev_rf1 = { .devHalInfo = &hal_rf1, .devStateInfo = { 0 } };
jesd_core tx_jesd;
jesd_core rx_jesd;
jesd_core orx_jesd;
jesd_core_array jesd_array;

extern int intr_irq_proc();
uint64_t rflo=5500e6;
struct gpio_desc *gpio_trx_sw;
struct gpio_desc *gpio_fddtdd_sw1;
struct gpio_desc *gpio_fddtdd_sw2;
struct gpio_desc *gpio_lmk_sync;
struct gpio_desc *gpio_ref_select0;
struct gpio_desc *gpio_ref_select1;
uint32_t talAction = TALACT_NO_ACTION;
uint32_t val_out;
/**********************************************************/
/**********************************************************/
int main(void)
{
	tx_jesd.base_address = XPAR_JESD204C_TX_BASEADDR;
	rx_jesd.base_address = XPAR_JESD204C_RX_BASEADDR;
	tx_jesd.rx_tx_n = 0;
	tx_jesd.scramble_enable = 1;
	tx_jesd.octets_per_frame = 2;
	tx_jesd.frames_per_multiframe = 32;
	tx_jesd.subclass_mode = 1;
	tx_jesd.lanes = 4;
	tx_jesd.converters_per_device = 4;
	tx_jesd.converter_resolution = 16;
	tx_jesd.bits_per_sample = 16;
	tx_jesd.samples_per_frame = 1;
	tx_jesd.control_bits_per_sample = 0;
	tx_jesd.control_words_per_frame = 0;
	tx_jesd.high_density = 0;
	tx_jesd.bank_id = 0;
	tx_jesd.device_id = 0;
	tx_jesd.lane0_id = 0;
	tx_jesd.ila_multiframes = 4;
	tx_jesd.sysref_always = 0;
	tx_jesd.sysref_required = 0;
	rx_jesd.rx_tx_n = 1;
	rx_jesd.scramble_enable = 1;
	rx_jesd.octets_per_frame = 4;
	rx_jesd.frames_per_multiframe = 32;
	rx_jesd.subclass_mode = 1;
	rx_jesd.lanes = 2;
	rx_jesd.converters_per_device = 4;
	rx_jesd.converter_resolution = 16;
	rx_jesd.bits_per_sample = 16;
	rx_jesd.samples_per_frame = 1;
	rx_jesd.control_bits_per_sample = 0;
	rx_jesd.control_words_per_frame = 0;
	rx_jesd.high_density = 0;
	rx_jesd.bank_id = 0;
	rx_jesd.device_id = 0;
	rx_jesd.lane0_id = 0;
	rx_jesd.ila_multiframes = 4;
	rx_jesd.sysref_always = 0;
	rx_jesd.sysref_required = 0;
	jesd_array.tx_jesd = tx_jesd;
	jesd_array.rx_jesd = rx_jesd;
	jesd_array.orx_jesd = rx_jesd;
    jesd_array.orx_jesd.base_address=XPAR_JESD204C_ORX_BASEADDR;
	int32_t status;
	print("**************************************************\n\r");
	print("**********FMC_ADRV9009/9379 TEST begin ************\n\r");
	print("**************************************************\n\r");
	printf("Please wait...\n");
	/**********************************************************/
	/**********************************************************/
	/************ Talise Initialization Sequence *************/
	/**********************************************************/
	/**********************************************************/

	/** < Insert User System Clock(s) Initialization Code Here >
	 * System Clock should provide a device clock and SYSREF signal
	 * to the Talise device.
	 **/
	struct gpio_desc *gpio_adrv_resetb;
	struct gpio_desc *gpio_adrv_tx1_enable;
	struct gpio_desc *gpio_adrv_tx2_enable;
	struct gpio_desc *gpio_adrv_rx1_enable;
	struct gpio_desc *gpio_adrv_rx2_enable;
	struct gpio_desc *gpio_adrv_test;
	struct gpio_desc *gpio_lmk_reset;

	status = gpio_get(&gpio_adrv_test, adrv9009_test); printf("adrv9009_test: %d %d\n", status, adrv9009_test);
	status = gpio_get(&gpio_adrv_tx1_enable, adrv9009_tx1_enable); printf("adrv9009_tx1_enable: %d %d\n", status, adrv9009_tx1_enable);
	status = gpio_get(&gpio_adrv_tx2_enable, adrv9009_tx2_enable); printf("adrv9009_tx2_enable: %d %d\n", status, adrv9009_tx2_enable);
	status = gpio_get(&gpio_adrv_rx1_enable, adrv9009_rx1_enable); printf("adrv9009_rx1_enable: %d %d\n", status, adrv9009_rx1_enable);
	status = gpio_get(&gpio_adrv_rx2_enable, adrv9009_rx2_enable); printf("adrv9009_rx2_enable: %d %d\n", status, adrv9009_rx2_enable);
	status = gpio_get(&gpio_lmk_reset, ad9528_reset_b); printf("ad9528_reset_b: %d %d\n", status, ad9528_reset_b);
	status = gpio_get(&gpio_lmk_sync, ad9528_sysref_req); printf("ad9528_sysref_req: %d %d\n", status, ad9528_sysref_req);
	status = gpio_get(&gpio_adrv_resetb, adrv9009_reset_b); printf("adrv9009_reset_b: %d %d\n", status, adrv9009_reset_b);
	mdelay(100);
	gpio_direction_output(gpio_lmk_reset, 0);
	mdelay(500);
	gpio_direction_output(gpio_lmk_reset, 1);
	gpio_direction_output(gpio_adrv_test, 0);
	gpio_direction_output(gpio_adrv_tx1_enable, 1);
	gpio_direction_output(gpio_adrv_tx2_enable, 1);
	gpio_direction_output(gpio_adrv_rx1_enable, 1);
	gpio_direction_output(gpio_adrv_rx2_enable, 1);

	gpio_direction_output(gpio_lmk_sync, 0);

	AXI_REG_WRITE(axi_lite_addr, 73 * 4, 0x7FFFE);	//rf1_gpio_t
	AXI_REG_WRITE(axi_lite_addr, 75 * 4, 0);		//rf1_gpio_o

	/**************************************************************************/
	/*****      configure system clock					                  *****/
	/**************************************************************************/
	spi_init_clk(&spi_desc_AD9528,spi_AD9528_param);
	AD9528_init(spi_desc_AD9528);					//printf 0x03

	int16_t lock_pll1 = -1;
	int16_t lock_pll2 = -1;
	int cyc=0;
	while ((lock_pll1 != 0) || lock_pll2 != 1 || cyc<100) {
		lock_pll1 = AD9528_spi_read(spi_desc_AD9528, 0X508) >> 0 & 0x1;
		lock_pll2 = AD9528_spi_read(spi_desc_AD9528, 0X508) >> 1 & 0x1;
//		printf("read from rf1_AD9528 pll1_lock=0x%x\n", lock_pll1);
//		printf("read from rf1_AD9528 pll2_lock=0x%x\n", lock_pll2);
		cyc++;

		if (lock_pll2 == 1 && (lock_pll1 == 0)) {
			lock_pll2 = 1;
			lock_pll1 = 0;
			cyc++;
		} else {
			lock_pll2 = 0;
			lock_pll1 = 0;
			cyc = 0;
		}
		mdelay(10);
	}
	printf("read from rf1_AD9528 pll1_lock=%d\r\n", lock_pll1);
	printf("read from rf1_AD9528 pll2_lock=%d\r\n", lock_pll2);
	gpio_direction_output(gpio_lmk_sync, 1);
	mdelay(500);
	AXI_REG_WRITE(axi_lite_addr, 2 * 4, 1);//jesd204b_reset=0;
	mdelay(500);
	AXI_REG_WRITE(axi_lite_addr, 2 * 4, 0);//jesd204b_reset=0;
	mdelay(100);
//	gpio_direction_output(gpio_lmk_sync, 0);
	printf("generate sysref \n");

	/**************************************************************************/
	/*****      configure adrv9009   					                  *****/
	/**************************************************************************/
	talAction = ad9009_initial_rf1(&talDev_rf1, jesd_array,jesd_phy0_addr);
	mdelay(2000);
	gpio_direction_output(gpio_lmk_sync, 0);
	TALISE_setTxAttenuation(&talDev_rf1, TAL_TX1, 0);
	TALISE_setTxAttenuation(&talDev_rf1, TAL_TX2, 0);
	TALISE_setRxManualGain(&talDev_rf1, TAL_RX1, 10+100);
	TALISE_setRxManualGain(&talDev_rf1, TAL_RX2, 10+100);
	TALISE_getRfPllFrequency(&talDev_rf1, TAL_RF_PLL, &rflo);
	printf("RF1,rf_lo_freq=%lluHz\n", rflo);
	if (talAction != TALACT_NO_ACTION) {
		/*** < User: decide what to do based on Talise recovery action returned > ***/
		printf("error: ad9009_initial() failed\n");
	} else
		printf("adrv9009 config successful!\n");

	printf("The lasted build at %s %s\n", __TIME__, __DATE__);
	rf_freq = talDev_rf1.devStateInfo.rxOutputRate_kHz * 1000;
	bb_freq = rf_freq / dived;
	printf("The sample rate is %ld Hz\n", bb_freq);

	while (1) mdelay(100);
}
