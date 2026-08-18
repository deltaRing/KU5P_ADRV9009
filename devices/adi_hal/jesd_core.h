/***************************************************************************//**
 * @file jesd_core.h
 * @brief Header file of Jesd Core.
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
#ifndef JESD_CORE_H_
#define JESD_CORE_H_

#include "platform_drivers.h"

/******************************************************************************/
/**************** AMD/Xilinx JESD204C v4.x (8B10B) registers ****************/
/******************************************************************************/
/* PG242 register map.  This is not the legacy ADI/Xilinx JESD204B map. */
#define JESD204_REG_TRX_VERSION                         0x000
#define JESD204_REG_TRX_CONFIG                          0x004
#define JESD204_REG_TRX_RESET                           0x020
#define JESD204_REG_TRX_TX_SYNC                         0x028
#define JESD204_REG_TRX_SUBCLASS_MODE                   0x034
#define JESD204_REG_TRX_8B10B_CFG                       0x03c
#define JESD204_REG_TRX_LANES_IN_USE                    0x040
#define JESD204_REG_RX_BUFFER_ADVANCE                   0x044
#define JESD204_REG_TRX_TEST_MODES                      0x048
#define JESD204_REG_TRX_SYSREF_HANDLING                 0x050
#define JESD204_REG_RX_LINK_ERROR_STATUS                0x058
#define JESD204_REG_RX_DEBUG_STATUS                     0x05c
#define JESD204_REG_TRX_SYNC_STATUS                     0x060
#define JESD204_REG_TX_ILA_CFG0                         0x070
#define JESD204_REG_TX_ILA_CFG1                         0x074
#define JESD204_REG_TX_ILA_CFG2                         0x078
#define JESD204_REG_TX_ILA_CFG3                         0x07c
#define JESD204_REG_TX_ILA_CFG4                         0x080
#define JESD204_REG_TX_ILA_LID(lane)                    (0x404 + ((lane) * 0x80))

/* JESD204_REG_TRX_CONFIG */
#define JESD204_TRX_CONFIG_TX                           (1U << 16)
#define JESD204_TRX_CONFIG_LANES(x)                     ((x) & 0x0fU)

/* JESD204_REG_TRX_RESET */
#define JESD204_TRX_GT_RESET_BUSY                       (1U << 7)
#define JESD204_TRX_RESET_REG_STATE                     (1U << 5)
#define JESD204_TRX_EXTERNAL_RESET_STATE                (1U << 4)
#define JESD204_TRX_RESET                               (1U << 0)

/* JESD204_REG_TRX_8B10B_CFG */
#define JESD204_TRX_ILA_MULTIFRAMES(x)                  ((((uint32_t)(x) - 1U) & 0xffU) << 24)
#define JESD204_TRX_LINK_ERROR_COUNTER_EN               (1U << 19)
#define JESD204_TRX_ERROR_REPORTING_EN                  (1U << 18)
#define JESD204_TRX_ILA_EN                              (1U << 17)
#define JESD204_TRX_SCR_EN                              (1U << 16)
#define JESD204_TRX_FRAMES_PER_MULTIFRAME(x)            ((((uint32_t)(x) - 1U) & 0x1fU) << 8)
#define JESD204_TRX_OCTETS_PER_FRAME(x)                 (((uint32_t)(x) - 1U) & 0xffU)

/* JESD204_REG_TRX_LANES_IN_USE */
#define JESD204_TRX_LANE_MASK(x)                        ((1U << (x)) - 1U)

/* JESD204_REG_TRX_SUBCLASS_MODE */
#define JESD204_TRX_SUBCLASS_MODE(x)                    ((x) & 0x3U)

/* JESD204_REG_TRX_SYSREF_HANDLING */
#define JESD204_TRX_SYSREF_ALWAYSON                     (1U << 0)
#define JESD204_TRX_SYSREF_ONRESYNC                     (1U << 1)
#define JESD204_TRX_SYSREF_TOLERANCE(x)                 (((x) & 0x7U) << 8)
#define JESD204_TRX_SYSREF_DELAY(x)                     (((x) & 0xfU) << 16)

/* JESD204_REG_RX_LINK_ERROR_STATUS: four bits per lane, clear on read. */
#define JESD204_RX_LINK_K_CH_ERR(lane)                  (1U << (2U + (4U * (lane))))
#define JESD204_RX_LINK_DISP_ERR(lane)                  (1U << (1U + (4U * (lane))))
#define JESD204_RX_LINK_NOT_IN_TBL_ERR(lane)            (1U << (4U * (lane)))

/* JESD204_REG_TRX_SYNC_STATUS */
#define JESD204_TRX_ALIGNMENT_ERROR                     (1U << 15)
#define JESD204_TRX_RX_DATA_STARTED                     (1U << 14)
#define JESD204_TRX_CGS_ACHIEVED                        (1U << 13)
#define JESD204_TRX_SYNC_ACHIEVED                       (1U << 12)
#define JESD204_TRX_BUFFER_OVERFLOW                     (1U << 10)
#define JESD204_TRX_SYSREF_ERROR                        (1U << 2)
#define JESD204_TRX_SYSREF_CAPTURED                     (1U << 1)

/* TX ILA fields */
#define JESD204_TX_ILA_CFG0(bid, did)                   ((((bid) & 0xfU) << 8) | ((did) & 0xffU))
#define JESD204_TX_ILA_CFG1(cs, np, n, m)               ((((cs) & 0x3U) << 24) | \
								 (((uint32_t)(np) - 1U) << 16) | \
								 (((uint32_t)(n) - 1U) << 8) | \
								 ((uint32_t)(m) - 1U))
#define JESD204_TX_ILA_CFG2(cf, hd, s)                  ((((cf) & 0x1fU) << 24) | \
								 (((hd) & 0x1U) << 16) | \
								 (((uint32_t)(s) - 1U) << 8))
#define JESD204_TX_ILA_LID(l, lid)                      ((((uint32_t)(l) - 1U) << 16) | ((lid) & 0x1fU))

/******************************************************************************/
/*************************** Types Declarations *******************************/
/******************************************************************************/

typedef enum {
	EXTERN,
	INTERN
} sys_ref_type;

typedef struct {
  uint32_t		base_address;
  uint8_t		rx_tx_n;
  uint8_t		scramble_enable;
  uint8_t		octets_per_frame;
  uint8_t		frames_per_multiframe;
  uint8_t		subclass_mode;
  uint8_t		lanes;
  uint8_t		converters_per_device;
  uint8_t		converter_resolution;
  uint8_t		bits_per_sample;
  uint8_t		samples_per_frame;
  uint8_t		control_bits_per_sample;
  uint8_t		control_words_per_frame;
  uint8_t		high_density;
  uint8_t		bank_id;
  uint8_t		device_id;
  uint8_t		lane0_id;
  uint8_t		ila_multiframes;
  uint8_t		sysref_always;
  uint8_t		sysref_required;
  sys_ref_type		sysref_type;
  uint32_t		sysref_gpio_pin;
} jesd_core;

/******************************************************************************/
/************************ Functions Declarations ******************************/
/******************************************************************************/

int32_t jesd_read(jesd_core core, uint32_t reg_addr, uint32_t *reg_data);
int32_t jesd_write(jesd_core core, uint32_t reg_addr, uint32_t reg_data);

int32_t jesd_setup(jesd_core core);
int32_t jesd_wait_reset_done(jesd_core core, uint32_t timeout_ms);
int32_t jesd_status(jesd_core core);
int32_t jesd_sysref_control(jesd_core core, uint32_t enable);

#endif
