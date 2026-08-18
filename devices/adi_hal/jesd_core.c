/***************************************************************************//**
* @file jesd_core.c
* @brief Implementation of Jesd Core.
* @author DBogdan (dragos.bogdan@analog.com)
********************************************************************************
* Copyright 2016(c) Analog Devices, Inc.
*
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
* - Redistributions of source code must retain the above copyright
* notice, this list of conditions and the following disclaimer.
* - Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in
* the documentation and/or other materials provided with the
* distribution.
* - Neither the name of Analog Devices, Inc. nor the names of its
* contributors may be used to endorse or promote products derived
* from this software without specific prior written permission.
* - The use of this software may or may not infringe the patent rights
* of one or more patent holders. This license does not release you
* from the requirement that you obtain separate licenses from these
* patent holders to use this software.
* - Use of the software either in source or binary form, must be run
* on or directly connected to an Analog Devices Inc. component.
*
* THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES "AS IS" AND ANY EXPRESS OR
* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL ANALOG DEVICES BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, INTELLECTUAL PROPERTY RIGHTS, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

/******************************************************************************/
/***************************** Include Files **********************************/
/******************************************************************************/
#include "jesd_core.h"
#include <stdio.h>
#define XILINX
/***************************************************************************//**
* @brief jesd_read
*******************************************************************************/
int32_t jesd_read(jesd_core core,
					uint32_t reg_addr,
					uint32_t *reg_data)
{
	*reg_data = ad_reg_read((core.base_address + reg_addr));

	return 0;
}

/***************************************************************************//**
* @brief jesd_write
*******************************************************************************/
int32_t jesd_write(jesd_core core,
					uint32_t reg_addr,
					uint32_t reg_data)
{
	ad_reg_write((core.base_address + reg_addr), reg_data);

	return 0;
}


/***************************************************************************//**
* @brief Configure an AMD/Xilinx JESD204C v4.x core in 8B10B mode.
*
* The legacy driver used a different JESD204B register map.  In particular,
* it wrote F to address 0x20, which is the RESET register in JESD204C v4.x.
*******************************************************************************/
int32_t jesd_setup(jesd_core core)
{
	uint32_t cfg;
	uint32_t ip_cfg;
	uint32_t lane;
	uint32_t sysref_cfg;

	if ((core.lanes == 0U) || (core.lanes > 8U) ||
	    (core.octets_per_frame == 0U) ||
	    (core.frames_per_multiframe == 0U) ||
	    (core.frames_per_multiframe > 32U) ||
	    (core.subclass_mode > 2U) ||
	    (core.ila_multiframes < 4U)) {
		printf("%s: invalid JESD link parameters at 0x%08lx\n",
		       __func__, (unsigned long)core.base_address);
		return -1;
	}

	if ((core.rx_tx_n == 0U) &&
	    ((core.converters_per_device == 0U) ||
	     (core.converter_resolution == 0U) ||
	     (core.bits_per_sample == 0U) ||
	     (core.samples_per_frame == 0U))) {
		printf("%s: incomplete TX ILA parameters at 0x%08lx\n",
		       __func__, (unsigned long)core.base_address);
		return -1;
	}

	jesd_read(core, JESD204_REG_TRX_CONFIG, &ip_cfg);
	if ((ip_cfg & (1U << 17)) != 0U) {
		printf("%s: core at 0x%08lx is 64B66B, expected 8B10B\n",
		       __func__, (unsigned long)core.base_address);
		return -1;
	}
	if ((((ip_cfg & JESD204_TRX_CONFIG_TX) != 0U) &&
	     (core.rx_tx_n != 0U)) ||
	    (((ip_cfg & JESD204_TRX_CONFIG_TX) == 0U) &&
	     (core.rx_tx_n == 0U))) {
		printf("%s: TX/RX direction mismatch, IP_CONFIG=0x%08lx\n",
		       __func__, (unsigned long)ip_cfg);
		return -1;
	}
	if (JESD204_TRX_CONFIG_LANES(ip_cfg) != core.lanes) {
		printf("%s: lane mismatch, IP=%lu software=%u\n", __func__,
		       (unsigned long)JESD204_TRX_CONFIG_LANES(ip_cfg), core.lanes);
		return -1;
	}

	/* Hold the core in reset while changing the framing parameters. */
	jesd_write(core, JESD204_REG_TRX_RESET, JESD204_TRX_RESET);
	jesd_write(core, JESD204_REG_TRX_SUBCLASS_MODE,
		   JESD204_TRX_SUBCLASS_MODE(core.subclass_mode));

	cfg = JESD204_TRX_ILA_EN |
	      JESD204_TRX_FRAMES_PER_MULTIFRAME(core.frames_per_multiframe) |
	      JESD204_TRX_OCTETS_PER_FRAME(core.octets_per_frame);
	if (core.scramble_enable != 0U)
		cfg |= JESD204_TRX_SCR_EN;
	if (core.rx_tx_n == 0U)
		cfg |= JESD204_TRX_ILA_MULTIFRAMES(core.ila_multiframes);
	jesd_write(core, JESD204_REG_TRX_8B10B_CFG, cfg);
	jesd_write(core, JESD204_REG_TRX_LANES_IN_USE,
		   JESD204_TRX_LANE_MASK(core.lanes));

	sysref_cfg = 0U;
	if (core.sysref_always != 0U)
		sysref_cfg |= JESD204_TRX_SYSREF_ALWAYSON;
	if (core.sysref_required != 0U)
		sysref_cfg |= JESD204_TRX_SYSREF_ONRESYNC;
	jesd_write(core, JESD204_REG_TRX_SYSREF_HANDLING, sysref_cfg);

	if (core.rx_tx_n == 0U) {
		jesd_write(core, JESD204_REG_TX_ILA_CFG0,
			   JESD204_TX_ILA_CFG0(core.bank_id, core.device_id));
		jesd_write(core, JESD204_REG_TX_ILA_CFG1,
			   JESD204_TX_ILA_CFG1(core.control_bits_per_sample,
						 core.bits_per_sample,
						 core.converter_resolution,
						 core.converters_per_device));
		jesd_write(core, JESD204_REG_TX_ILA_CFG2,
			   JESD204_TX_ILA_CFG2(core.control_words_per_frame,
						 core.high_density,
						 core.samples_per_frame));
		jesd_write(core, JESD204_REG_TX_ILA_CFG3, 0U);
		jesd_write(core, JESD204_REG_TX_ILA_CFG4, 0U);
		for (lane = 0U; lane < core.lanes; lane++) {
			jesd_write(core, JESD204_REG_TX_ILA_LID(lane),
				   JESD204_TX_ILA_LID(core.lanes,
							core.lane0_id + lane));
		}
	}

	/* Release the software reset. Bit 0 reads back high until GT reset ends. */
	jesd_write(core, JESD204_REG_TRX_RESET, 0U);
	return 0;
}

/***************************************************************************//**
* @brief Wait for the JESD204C core and JESD204 PHY reset sequence.
*******************************************************************************/
int32_t jesd_wait_reset_done(jesd_core core, uint32_t timeout_ms)
{
	uint32_t reset_status = 0U;

	for (;;) {
		jesd_read(core, JESD204_REG_TRX_RESET, &reset_status);
		if ((reset_status & JESD204_TRX_RESET) == 0U)
			return 0;
		if (timeout_ms == 0U)
			break;
		timeout_ms--;
		mdelay(1);
	}

	printf("%s: timeout at 0x%08lx, RESET=0x%08lx "
	       "(gt_busy=%lu ext_reset=%lu reg_reset=%lu)\n",
	       __func__, (unsigned long)core.base_address,
	       (unsigned long)reset_status,
	       (unsigned long)((reset_status >> 7) & 1U),
	       (unsigned long)((reset_status >> 4) & 1U),
	       (unsigned long)((reset_status >> 5) & 1U));
	return -1;
}

/***************************************************************************//**
* @brief Check JESD204C v4.x 8B10B link status.
*******************************************************************************/
int32_t jesd_status(jesd_core core)
{
	uint32_t status = 0U;
	uint32_t error_status = 0U;
	uint32_t timeout;
	uint8_t lane;
	int32_t ret = 0;

	if (jesd_wait_reset_done(core, 1000U) != 0)
		return -1;

	if (core.subclass_mode >= 1U) {
		for (timeout = 1000U; timeout > 0U; timeout--) {
			jesd_read(core, JESD204_REG_TRX_SYNC_STATUS, &status);
			if ((status & JESD204_TRX_SYSREF_CAPTURED) != 0U)
				break;
			mdelay(1);
		}
		if ((status & JESD204_TRX_SYSREF_CAPTURED) == 0U) {
			printf("%s: missing SYSREF, STAT_STATUS=0x%08lx\n",
			       __func__, (unsigned long)status);
			return -1;
		}
	}

	for (timeout = 1000U; timeout > 0U; timeout--) {
		jesd_read(core, JESD204_REG_TRX_SYNC_STATUS, &status);
		if ((status & JESD204_TRX_SYNC_ACHIEVED) != 0U)
			break;
		mdelay(1);
	}
	if ((status & JESD204_TRX_SYNC_ACHIEVED) == 0U) {
		printf("%s: 8B10B SYNC not achieved, STAT_STATUS=0x%08lx\n",
		       __func__, (unsigned long)status);
		return -1;
	}

	if (core.rx_tx_n != 0U) {
		for (timeout = 1000U; timeout > 0U; timeout--) {
			jesd_read(core, JESD204_REG_TRX_SYNC_STATUS, &status);
			if ((status & JESD204_TRX_RX_DATA_STARTED) != 0U)
				break;
			mdelay(1);
		}
		if ((status & JESD204_TRX_RX_DATA_STARTED) == 0U) {
			printf("%s: RX data not started, STAT_STATUS=0x%08lx\n",
			       __func__, (unsigned long)status);
			ret = -1;
		}

		jesd_read(core, JESD204_REG_RX_LINK_ERROR_STATUS, &error_status);
		for (lane = 0U; lane < core.lanes; lane++) {
			if ((error_status & JESD204_RX_LINK_K_CH_ERR(lane)) != 0U) {
				printf("%s lane %u: unexpected K character\n", __func__, lane);
				ret = -1;
			}
			if ((error_status & JESD204_RX_LINK_DISP_ERR(lane)) != 0U) {
				printf("%s lane %u: disparity error\n", __func__, lane);
				ret = -1;
			}
			if ((error_status & JESD204_RX_LINK_NOT_IN_TBL_ERR(lane)) != 0U) {
				printf("%s lane %u: not-in-table error\n", __func__, lane);
				ret = -1;
			}
		}
	}

	jesd_read(core, JESD204_REG_TRX_SYNC_STATUS, &status);
	if ((status & JESD204_TRX_ALIGNMENT_ERROR) != 0U) {
		printf("%s: 8B10B alignment error\n", __func__);
		ret = -1;
	}
	if ((status & JESD204_TRX_BUFFER_OVERFLOW) != 0U) {
		printf("%s: receive buffer overflow\n", __func__);
		ret = -1;
	}
	if ((status & JESD204_TRX_SYSREF_ERROR) != 0U) {
		printf("%s: SYSREF alignment error\n", __func__);
		ret = -1;
	}

	printf("%s: base=0x%08lx STAT_STATUS=0x%08lx RX_ERR=0x%08lx\n",
	       __func__, (unsigned long)core.base_address,
	       (unsigned long)status, (unsigned long)error_status);
	return ret;
}

/***************************************************************************//**
* @brief Enable or disable SYSREF-always handling in the JESD204C core.
*******************************************************************************/
int32_t jesd_sysref_control(jesd_core core, uint32_t enable)
{
	uint32_t value;

	jesd_read(core, JESD204_REG_TRX_SYSREF_HANDLING, &value);
	if (enable != 0U)
		value |= JESD204_TRX_SYSREF_ALWAYSON;
	else
		value &= ~JESD204_TRX_SYSREF_ALWAYSON;
	jesd_write(core, JESD204_REG_TRX_SYSREF_HANDLING, value);
	return 0;
}
