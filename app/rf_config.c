/*
 * ad9009_init.c
 *
 *  Created on: 2018-08-01
 *      Author: liche
 */
#include "rf_config.h"
#include "../firmware/talise_arm_binary.h"
#include "../firmware/talise_stream_binary.h"
#include "../devices/talise/talise_gpio_types.h"
#include "../devices/talise/talise_gpio.h"
extern uint32_t axi_lite_addr;
extern struct gpio_desc *gpio_rf1_gpio0;
extern struct gpio_desc *gpio_lmk_sync;
extern uint64_t rflo;
/*************************************************************************/
/*****                     ad9009_hard_reset                         *****/
/*************************************************************************/
uint32_t ad9009_hard_reset(taliseDevice_t *talDev, uint32_t rfid) {
	uint32_t talAction = TALACT_NO_ACTION;
	/*Open Talise Hw Device*/
	talAction = TALISE_openHw(talDev,rfid);
	if (talAction != TALACT_NO_ACTION) {
		/*** < User: decide what to do based on Talise recovery action returned > ***/
		printf("error: RF%ld, TALISE_openHw() failed\n",rfid);
		return talAction;
	}

	/* Toggle RESETB pin on Talise device */
	talAction = TALISE_resetDevice(talDev);
	if (talAction != TALACT_NO_ACTION) {
		/*** < User: decide what to do based on Talise recovery action returned > ***/
		printf("error: RF%ld, TALISE_resetDevice() failed\n",rfid);
		return talAction;
	}
	mdelay(100);

	/* TALISE_initialize() loads the Talise device data structure
	 * settings for the Rx/Tx/ORx profiles, FIR filters, digital
	 * filter enables, calibrates the CLKPLL, loads the user provided Rx
	 * gain tables, and configures the JESD204b serializers/framers/deserializers
	 * and deframers.
	 */
	talAction = TALISE_initialize(talDev, &talInit);
	if (talAction != TALACT_NO_ACTION) {
		/*** < User: decide what to do based on Talise recovery action returned > ***/
		printf("error: RF%ld, TALISE_initialize() failed\n",rfid);
		return talAction;
	}
	return talAction;
}
/*************************************************************************/
/*****                           ad9009_init                         *****/
/*************************************************************************/
uint32_t ad9009_init(taliseDevice_t *talDev) {
	uint32_t talAction = TALACT_NO_ACTION;
	uint8_t pllLockStatus = 0;
	uint8_t mcsStatus = 0;
	uint32_t count = sizeof(armBinary);
	taliseArmVersionInfo_t talArmVersionInfo;
	uint32_t api_vers[4];
	uint8_t rev;
	/***** CLKPLL Status Check *****/
	/*******************************/
	talAction = TALISE_getPllsLockStatus(talDev, &pllLockStatus);
	if (talAction != TALACT_NO_ACTION) {
		/*** < User: decide what to do based on Talise recovery action returned > ***/
		printf("TALISE_getPllsLockStatus() failed\n");
		return talAction;
	}
	/* Assert that Talise CLKPLL is locked */
	if ((pllLockStatus & 0x01) == 0) {
		/* <user code - CLKPLL not locked - ensure lock before proceeding */
		printf("error: CLKPLL not locked\n");
		return talAction;
	}
	/*******************************************************/
	/**** Perform MultiChip Sync (MCS) on Talise Device ***/
	/*******************************************************/
//	talAction = TALISE_enableMultichipSync(talDev, 1, &mcsStatus);
//	if (talAction != TALACT_NO_ACTION) {
//		/*** < User: decide what to do based on Talise recovery action returned > ***/
//		printf("error: TALISE_enableMultichipSync() failed\n");
//		return talAction;
//	}
//	/*******************/
//	/**** Verify MCS ***/
//	/*******************/
//	talAction = TALISE_enableMultichipSync(talDev, 0, &mcsStatus);
//	if ((mcsStatus & 0x0B) != 0x0B) {
//		/*< user code - MCS failed - ensure MCS before proceeding*/
//		printf("warning: TALISE_enableMultichipSync() failed\n");
//		return talAction;
//	}
	/*******************************************************/
	/**** Prepare Talise Arm binary and Load Arm and	****/
	/**** Stream processor Binaryes 					****/
	/*******************************************************/
	if (pllLockStatus & 0x01) {
		talAction = TALISE_initArm(talDev, &talInit);
		if (talAction != TALACT_NO_ACTION) {
			/*** < User: decide what to do based on Talise recovery action returned > ***/
			printf("error: TALISE_initArm() failed\n");
			return talAction;
		}
		/*< user code- load Talise stream binary into streamBinary[4096] >*/
		/*< user code- load ARM binary byte array into armBinary[114688] >*/
		talAction = TALISE_loadStreamFromBinary(talDev, &streamBinary[0]);
		if (talAction != TALACT_NO_ACTION) {
			/*** < User: decide what to do based on Talise recovery action returned > ***/
			printf("error: TALISE_loadStreamFromBinary() failed\n");
			return talAction;
		}
		talAction = TALISE_loadArmFromBinary(talDev, &armBinary[0], count);
		if (talAction != TALACT_NO_ACTION) {
			/*** < User: decide what to do based on Talise recovery action returned > ***/
			printf("error: TALISE_loadArmFromBinary() failed\n");
			return talAction;
		}
		/* TALISE_verifyArmChecksum() will timeout after 200ms
		 * if ARM checksum is not computed
		 */
		talAction = TALISE_verifyArmChecksum(talDev);
		if (talAction != TAL_ERR_OK) {
			/*< user code- ARM did not load properly - check armBinary & clock/profile settings >*/
			printf("error: TALISE_verifyArmChecksum() failed\n");
			return talAction;
		}

	} else {
		/*< user code- check settings for proper CLKPLL lock  > ***/
		printf("error: CLKPLL not locked\n");
		return talAction;
	}
	TALISE_getDeviceRev(talDev, &rev);
	TALISE_getArmVersion_v2(talDev, &talArmVersionInfo);
	TALISE_getApiVersion(talDev, &api_vers[0], &api_vers[1], &api_vers[2],
			&api_vers[3]);

	printf(
			"talise: Device Revision %d, Firmware %d.%d.%d, API %lu.%lu.%lu.%lu\n",
			rev, talArmVersionInfo.majorVer, talArmVersionInfo.minorVer,
			talArmVersionInfo.rcVer, api_vers[0], api_vers[1], api_vers[2],
			api_vers[3]);
	return talAction;
}
/*************************************************************************/
/*****                      ad9009_rf_init_setpll                    *****/
/*************************************************************************/
uint32_t ad9009_rf_init_setpll(taliseDevice_t *talDev) {
	uint32_t talAction = TALACT_NO_ACTION;
	uint8_t pllLockStatus = 0;
	/*******************************/
	/**Set RF PLL LO Frequencies ***/
	/*******************************/
	talAction = TALISE_setRfPllFrequency(talDev, TAL_RF_PLL, rflo);
	if (talAction != TALACT_NO_ACTION) {
		/*** < User: decide what to do based on Talise recovery action returned > ***/
		printf("error: TALISE_setRfPllFrequency() failed\n");
		return talAction;
	}
	/*** < wait 200ms for PLLs to lock - user code here > ***/
	talAction = TALISE_getPllsLockStatus(talDev, &pllLockStatus);
	if ((pllLockStatus & 0x07) != 0x07) {
		/*< user code - ensure lock of all PLLs before proceeding>*/
		printf("error: RFPLL not locked\n");
		return talAction;
	}
	return talAction;
}

/*************************************************************************/
/*****                           ad9009_mcps                         *****/
/*************************************************************************/
uint32_t ad9009_mcps(taliseDevice_t *talDev) {
	uint32_t talAction = TALACT_NO_ACTION;
	talAction = TALISE_enableMultichipRfLOPhaseSync(talDev, 1);
	if (talAction != TALACT_NO_ACTION) {
		/*** < User: decide what to do based on Talise recovery action returned > ***/
		printf("error: enableMultichipRfLOPhaseSync() failed\n");
		return talAction;
	}
	//gpio_direction_output(gpio_lmk_sync, 0);
	mdelay(1);
	//gpio_direction_output(gpio_lmk_sync, 1);
	//mdelay(1);
	//gpio_direction_output(gpio_lmk_sync, 0);
	talAction=TALISE_enableMultichipRfLOPhaseSync(talDev, 0);
	mdelay(1);
	if (talAction != TALACT_NO_ACTION) {
		/*** < User: decide what to do based on Talise recovery action returned > ***/
		printf("error: enableMultichipRfLOPhaseSync() failed\n");
		return talAction;
	}
//	talAction = TALISE_serializerReset(talDev);
//	if (talAction != TALACT_NO_ACTION) {
//		/*** < User: decide what to do based on Talise recovery action returned > ***/
//		printf("error:  TALISE_serializerReset() failed\n",
//				);
//		return talAction;
//	}
	return talAction;
}

/*************************************************************************/
/*****                           ad9009_rf_init_cal                      *****/
/*************************************************************************/
uint32_t ad9009_rf_init_cal(taliseDevice_t *talDev) {
	uint32_t talAction = TALACT_NO_ACTION;
	uint32_t initCalMask = TAL_TX_BB_FILTER
			| TAL_ADC_TUNER | TAL_TIA_3DB_CORNER
			| TAL_DC_OFFSET | TAL_TX_ATTENUATION_DELAY | TAL_RX_GAIN_DELAY
			| TAL_FLASH_CAL | TAL_PATH_DELAY | TAL_TX_LO_LEAKAGE_INTERNAL
			| TAL_TX_QEC_INIT | TAL_LOOPBACK_RX_LO_DELAY
			| TAL_LOOPBACK_RX_RX_QEC_INIT | TAL_RX_LO_DELAY | TAL_RX_QEC_INIT
			| TAL_RX_PHASE_CORRECTION | TAL_ORX_LO_DELAY | TAL_TX_DAC
			| TAL_ADC_STITCHING | TAL_ORX_QEC_INIT;
	uint8_t errorFlag = 0;

	/****************************************************/
	/**** Run Talise ARM Initialization Calibrations ***/
	/****************************************************/
	talAction = TALISE_runInitCals(talDev, initCalMask);
	if (talAction != TALACT_NO_ACTION) {
		/*** < User: decide what to do based on Talise recovery action returned > ***/
		printf("error: TALISE_runInitCals() failed\n");
		return talAction;
	}

	talAction = TALISE_waitInitCals(talDev, 20000, &errorFlag);
	if (talAction != TALACT_NO_ACTION) {
		/*** < User: decide what to do based on Talise recovery action returned > ***/
		printf("error: TALISE_waitInitCals() failed\n");
		return talAction;
	}

	if (errorFlag) {
		/*< user code - Check error flag to determine ARM  error> */
		printf("error: Calibrations not completed\n");
		return talAction;
	} else {
		/*< user code - Calibrations completed successfully > */
		printf("talise: Calibrations completed successfully\n");
	}
//	taliseAuxDac_t auxDac = {
//	0x10, /*!< Aux DAC enable bit for each DAC */
//	{ 0, 0, 0, 0, 1, 0, 0, 0, 0, 0 }, /*!< Aux DAC voltage reference value for each of the 10-bit DACs */
//	{ 0, 0, 0, 0, 2, 0, 0, 0, 0, 0 }, /*!< Aux DAC slope (resolution of voltage change per AuxDAC code) */
//	{ 0, 0, 0, 0, 373, 0, 0, 0, 0, 0, 0, 0 } /*!< Aux DAC values for each 10-bit DAC */
//	};
//	TALISE_setGpio3v3Oe(talDev, 0, 0);
//	talAction = TALISE_setupAuxDacs(talDev, &auxDac);
//	if (talAction != TALACT_NO_ACTION) {
//		/*** < User: decide what to do based on Talise recovery action returned > ***/
//		printf("error:  TALISE_setupAuxDacs() failed\n");
//		return talAction;
//	}
	talAction = TALISE_serializerReset(talDev);
	if (talAction != TALACT_NO_ACTION) {
		/*** < User: decide what to do based on Talise recovery action returned > ***/
		printf("error:  TALISE_serializerReset() failed\n");
		return talAction;
	}
	return talAction;
}

/*************************************************************************/
/*****                    ad9009_jesd204b_init                       *****/
/*************************************************************************/
uint32_t ad9009_jesd204b_init(taliseDevice_t *talDev) {
	uint32_t talAction = TALACT_NO_ACTION;
	/***************************************************/
	/**** Enable  Talise JESD204B Framer ***/
	/***************************************************/
	talAction = TALISE_enableFramerLink(talDev, TAL_FRAMER_A, 0);
	if (talAction != TALACT_NO_ACTION) {
		/*** < User: decide what to do based on Talise recovery action returned > ***/
		printf("error: TALISE_enableFramerLink() failed\n");
		return talAction;
	}
	talAction |= TALISE_enableFramerLink(talDev, TAL_FRAMER_A, 1);
	if (talAction != TALACT_NO_ACTION) {
		/*** < User: decide what to do based on Talise recovery action returned > ***/
		printf("error: TALISE_enableFramerLink() failed\n");
		return talAction;
	}
	/*************************************************/
	/**** Enable SYSREF to Talise JESD204B Framer ***/
	/*************************************************/
	/*** < User: Make sure SYSREF is stopped/disabled > ***/
	talAction = TALISE_enableSysrefToFramer(talDev, TAL_FRAMER_A, 1);
	if (talAction != TALACT_NO_ACTION) {
		/*** < User: decide what to do based on Talise recovery action returned > ***/
		printf("error: TALISE_enableSysrefToFramer() failed\n");
		return talAction;
	}
	/***************************************************/
	/**** Enable  Talise JESD204B Framer ***/
	/***************************************************/
	talAction = TALISE_enableFramerLink(talDev, TAL_FRAMER_B, 0);
	if (talAction != TALACT_NO_ACTION) {
		/*** < User: decide what to do based on Talise recovery action returned > ***/
		printf("error: TALISE_enableFramerLink() failed\n");
		return talAction;
	}
	talAction |= TALISE_enableFramerLink(talDev, TAL_FRAMER_B, 1);
	if (talAction != TALACT_NO_ACTION) {
		/*** < User: decide what to do based on Talise recovery action returned > ***/
		printf("error: TALISE_enableFramerLink() failed\n");
		return talAction;
	}
	/*************************************************/
	/**** Enable SYSREF to Talise JESD204B Framer ***/
	/*************************************************/
	/*** < User: Make sure SYSREF is stopped/disabled > ***/
	talAction = TALISE_enableSysrefToFramer(talDev, TAL_FRAMER_B, 1);
	if (talAction != TALACT_NO_ACTION) {
		/*** < User: decide what to do based on Talise recovery action returned > ***/
		printf("error: TALISE_enableSysrefToFramer() failed\n");
		return talAction;
	}
	/***************************************************/
	/**** Enable  Talise JESD204B Deframer ***/
	/***************************************************/
	talAction = TALISE_enableDeframerLink(talDev, TAL_DEFRAMER_A, 0);
	if (talAction != TALACT_NO_ACTION) {
		/*** < User: decide what to do based on Talise recovery action returned > ***/
		printf("error: TALISE_enableDeframerLink() failed\n");
		return talAction;
	}
	talAction |= TALISE_enableDeframerLink(talDev, TAL_DEFRAMER_A, 1);
	if (talAction != TALACT_NO_ACTION) {
		/*** < User: decide what to do based on Talise recovery action returned > ***/
		printf("error: TALISE_enableDeframerLink() failed\n");
		return talAction;
	}
	/***************************************************/
	/**** Enable SYSREF to Talise JESD204B Deframer ***/
	/***************************************************/
	talAction = TALISE_enableSysrefToDeframer(talDev, TAL_DEFRAMER_A, 1);
	if (talAction != TALACT_NO_ACTION) {
		/*** < User: decide what to do based on Talise recovery action returned > ***/
		printf("error: TALISE_enableDeframerLink() failed\n");
		return talAction;
	}
	return talAction;
}
/*************************************************************************/
/*****                    fpga_jesd204b_init                         *****/
/*************************************************************************/
uint32_t fpga_jesd204b_init(jesd_core_array jesdarray, uint32_t jesd_phy0_addr) {
	uint32_t status=0;
	status = jesd_phy_settings_singlephy(jesd_phy0_addr);
	if (status != 0U) {
		printf("error: jesd_phy_settings_singlephy() failed: 0x%08lx\n",
		       (unsigned long)status);
		return status;
	}
	/* Initialize JESDs for Device rf1 */
	status = jesd_setup(jesdarray.tx_jesd);
	if (status != 0U)
		return status;
	status = jesd_setup(jesdarray.rx_jesd);
	if (status != 0U)
		return status;
	status = jesd_setup(jesdarray.orx_jesd);
	if (status != 0U)
		return status;
	mdelay(100);
	return 0U;
}

static void print_jesd_ilas(const char *name,
		const taliseJesd204bLane0Config_t *ilas)
{
	printf("%s: DID=%u BID=%u LID0=%u L=%u SCR=%u F=%u K=%u "
	       "M=%u N=%u CS=%u NP=%u S=%u CF=%u HD=%u "
	       "FCHK=%02X/%02X/%02X/%02X\n",
	       name,
	       ilas->DID, ilas->BID, ilas->LID0, ilas->L,
	       ilas->SCR, ilas->F, ilas->K, ilas->M,
	       ilas->N, ilas->CS, ilas->NP, ilas->S,
	       ilas->CF, ilas->HD,
	       ilas->FCHK0, ilas->FCHK1, ilas->FCHK2, ilas->FCHK3);
}

static void print_tx_ila_readback(jesd_core tx)
{
	uint32_t cfg8b10b = 0U;
	uint32_t cfg0 = 0U;
	uint32_t cfg1 = 0U;
	uint32_t cfg2 = 0U;
	uint32_t lane0 = 0U;
	uint32_t lane1 = 0U;
	uint32_t lane2 = 0U;
	uint32_t lane3 = 0U;

	jesd_read(tx, JESD204_REG_TRX_8B10B_CFG, &cfg8b10b);
	jesd_read(tx, JESD204_REG_TX_ILA_CFG0, &cfg0);
	jesd_read(tx, JESD204_REG_TX_ILA_CFG1, &cfg1);
	jesd_read(tx, JESD204_REG_TX_ILA_CFG2, &cfg2);
	jesd_read(tx, JESD204_REG_TX_ILA_LID(0U), &lane0);
	jesd_read(tx, JESD204_REG_TX_ILA_LID(1U), &lane1);
	jesd_read(tx, JESD204_REG_TX_ILA_LID(2U), &lane2);
	jesd_read(tx, JESD204_REG_TX_ILA_LID(3U), &lane3);

	printf("TX ILA readback: 03C=%08lX 070=%08lX 074=%08lX "
	       "078=%08lX 404=%08lX 484=%08lX 504=%08lX 584=%08lX\n",
	       (unsigned long)cfg8b10b, (unsigned long)cfg0,
	       (unsigned long)cfg1, (unsigned long)cfg2,
	       (unsigned long)lane0, (unsigned long)lane1,
	       (unsigned long)lane2, (unsigned long)lane3);
}

uint32_t jesd204b_check(taliseDevice_t *talDev, jesd_core_array jesd_array) {
	uint32_t talAction = TALACT_NO_ACTION;
	uint32_t ilasAction = TALACT_NO_ACTION;
	uint32_t ilasMismatch = 0U;
	uint16_t deframerStatus = 0;
	uint8_t framerStatus = 0;
	taliseJesd204bLane0Config_t dfrmCfg = {0};
	taliseJesd204bLane0Config_t dfrmIlas = {0};

	/* Check the FPGA side first so a failed reset/SYSREF/SYNC is reported. */
	if (jesd_status(jesd_array.tx_jesd) != 0) {
		printf("warning: FPGA JESD204C TX link is not ready\n");
		return 255;
	}
	if (jesd_status(jesd_array.rx_jesd) != 0) {
		printf("warning: FPGA JESD204C RX link is not ready\n");
		return 255;
	}
	if (jesd_status(jesd_array.orx_jesd) != 0)
		printf("warning: FPGA JESD204C ORX link is not ready\n");

	/**************************************/
	/**** Check Talise Deframer Status ***/
	/**************************************/
	talAction = TALISE_readDeframerStatus(talDev, TAL_DEFRAMER_A,
			&deframerStatus);
	if (talAction != TALACT_NO_ACTION) {
		/*** < User: decide what to do based on Talise recovery action returned > ***/
		printf("error: TALISE_readDeframerStatus() failed\n");
		return talAction;
	}
	printf("TAL_DEFRAMER_A status 0x%X\n", deframerStatus);

	/*
	 * Bits 2 and 1 mean user data valid and SYSREF received.  Bits 6, 5,
	 * 4 and 0 are real link errors; bit 8 means an invalid configuration.
	 * Bit 7 is checked separately so status 0x06 can report the exact ILAS
	 * field/checksum mismatch instead of immediately tearing the link down.
	 */
	if ((deframerStatus & 0x0177U) != 0x0006U) {
		printf("warning: TAL_DEFRAMER_A status 0x%X\n", deframerStatus);
		return 255;
	}

	if ((deframerStatus & 0x0080U) == 0U) {
		ilasAction = TALISE_getDfrmIlasMismatch(talDev, TAL_DEFRAMER_A,
				&ilasMismatch, &dfrmCfg, &dfrmIlas);
		printf("warning: TAL_DEFRAMER_A status 0x%X: "
		       "user data valid, but ILAS valid-checksum bit is 0\n",
		       deframerStatus);
		if (ilasAction != TALACT_NO_ACTION) {
			printf("warning: TALISE_getDfrmIlasMismatch() failed: 0x%08lX\n",
			       (unsigned long)ilasAction);
			return 255;
		}

		printf("TAL_DEFRAMER_A ILAS_MISMATCH=0x%08lX\n",
		       (unsigned long)ilasMismatch);
		print_jesd_ilas("Deframer CFG ", &dfrmCfg);
		print_jesd_ilas("Received ILAS", &dfrmIlas);
		if (ilasMismatch != 0U)
			return 255;

		printf("warning: ILAS fields match; accepting status 0x06 "
		       "because SYSREF and user data are valid\n");
	}
	/************************************/
	/**** Check Talise Framer Status ***/
	/************************************/
	talAction = TALISE_readFramerStatus(talDev, TAL_FRAMER_A, &framerStatus);
	if (talAction != TALACT_NO_ACTION) {
		/*** < User: decide what to do based on Talise recovery action returned > ***/
		printf("error: TALISE_readFramerStatus() failed\n");
		return talAction;
	}
	if ((framerStatus & 0x07) != 0x05) {
		printf("warning: TAL_FRAMER_A status 0x%X\n", framerStatus);
		return talAction = 255;
	}
	/************************************/
	/**** Check Talise Framer Status ***/
	/************************************/
	talAction = TALISE_readFramerStatus(talDev, TAL_FRAMER_B, &framerStatus);
	if (talAction != TALACT_NO_ACTION) {
		/*** < User: decide what to do based on Talise recovery action returned > ***/
		printf("error: TALISE_readFramerStatus() failed\n");
		return talAction;
	}
	if ((framerStatus & 0x07) != 0x05) {
		printf("warning: TAL_FRAMER_B status 0x%X\n", framerStatus);
		//return talAction = 255;
	}
//	/*** < User: When links have been verified, proceed > ***/
//	uint32_t status = jesd_status(jesd_array.tx_jesd);
//	if (status != -1)
//		printf("tx_jesd is locking!\n");
//	else {
//		printf("tx_jesd is error!\n");
//		return talAction = 255;
//	}
//	status = jesd_status(jesd_array.rx_jesd);
//	if (status != -1)
//		printf("rx_jesd is locking!\n");
//	else {
//		printf("rx_jesd is error!\n");
//		return talAction = 255;
//	}
//	status = jesd_status(jesd_array.orx_jesd);
//	if (status != -1)
//		printf("orx_jesd is locking!\n");
//	else {
//		printf("orx_jesd is error!\n");
//		//return talAction = 255;
//	}
	return talAction;
}
/*************************************************************************/
/*****                           ad9009_radio_on                     *****/
/*************************************************************************/
uint32_t ad9009_radio_on(taliseDevice_t *talDev) {
	uint32_t talAction = TALACT_NO_ACTION;
	uint32_t trackingCalMask = TAL_TRACK_RX1_QEC | TAL_TRACK_RX2_QEC
			|TAL_TRACK_ORX1_QEC | TAL_TRACK_ORX2_QEC
			|TAL_TRACK_TX1_LOL | TAL_TRACK_TX2_LOL
			| TAL_TRACK_TX1_QEC | TAL_TRACK_TX2_QEC
			|TAL_TRACK_RX1_HD2 |TAL_TRACK_RX2_HD2;
	/***********************************************
	 * Allow Rx1/2 QEC tracking and Tx1/2 QEC	   *
	 * tracking to run when in the radioOn state	*
	 * Tx calibrations will only run if radioOn and *
	 * the obsRx path is set to OBS_INTERNAL_CALS   *
	 * **********************************************/
	talAction = TALISE_enableTrackingCals(talDev, trackingCalMask);
	if (talAction != TALACT_NO_ACTION) {
		/*** < User: decide what to do based on Talise recovery action returned > ***/
		printf("error: TALISE_enableTrackingCals() failed\n");
		return talAction;
	}
	taliseFhmConfig_t taliseFhmConfig;
	taliseFhmConfig.fhmGpioPin = TAL_GPIO_00;
	taliseFhmConfig.fhmMinFreq_MHz = 123;
	taliseFhmConfig.fhmMaxFreq_MHz = 6000;
	talAction = TALISE_setFhmConfig(talDev, &taliseFhmConfig);
	if (talAction != TALACT_NO_ACTION) {
		/*** < User: decide what to do based on Talise recovery action returned > ***/
		printf("error: TALISE_setFhmConfig() failed\n");
		return talAction;
	}
	/* Function to turn radio on, Enables transmitters and receivers */
	/* that were setup during TALISE_initialize() */
	talAction = TALISE_radioOn(talDev);
	if (talAction != TALACT_NO_ACTION) {
		/*** < User: decide what to do based on Talise recovery action returned > ***/
		printf("error: TALISE_radioOn() failed\n");
		return talAction;
	}
	taliseFhmMode_t fhmMode;
	fhmMode.fhmEnable = 1;
	fhmMode.enableMcsSync = 1;
	fhmMode.fhmTriggerMode = TAL_FHM_NON_GPIO_MODE; //TAL_FHM_GPIO_MODE;
	fhmMode.fhmExitMode = TAL_FHM_FULL_EXIT;
	fhmMode.fhmInitFrequency_Hz = 2500e6;//RFPllLoFrequency_Hz;
	talAction = TALISE_setFhmMode(talDev, &fhmMode);
	if (talAction != TALACT_NO_ACTION) {
		/*** < User: decide what to do based on Talise recovery action returned > ***/
		printf("error: TALISE_setFhmMode() failed\n");
		return talAction;
	}
	talAction = TALISE_setRxTxEnable(talDev, TAL_RX1RX2, TAL_TX1TX2);
	if (talAction != TALACT_NO_ACTION) {
		/*** < User: decide what to do based on Talise recovery action returned > ***/
		printf("error: TALISE_setRxTxEnable() failed\n");
		return talAction;
	}
	TALISE_enableFramerTestData(talDev, TAL_FRAMER_A_AND_B, TAL_FTD_ADC_DATA, //TAL_FTD_RAMP,TAL_FTD_ADC_DATA
			TAL_FTD_FRAMERINPUT);

	return talAction;
}
uint32_t ad9009_rx_dc_offset_cal(taliseDevice_t *device, uint8_t rank_matrix){
	uint8_t rank_matrix_ori = 0;
	uint32_t talAction = TALACT_NO_ACTION;
	TALISE_getDigDcOffsetMShift(device, TAL_DC_OFFSET_RX_CHN, &rank_matrix_ori);
	//	printf("rank_matrix_ori is %x\n", rank_matrix_ori);
	//calculate the rank of correlation matrix for RX notch filter according to RF situation, frequency range and sample rate,
	talAction = TALISE_setDigDcOffsetMShift(device, TAL_DC_OFFSET_RX_CHN, rank_matrix);
	return talAction;
}


static void jesd204c_dump_rx(uint32_t base, const char *name)
{
    uint32_t ip_ver;
    uint32_t ip_cfg;
    uint32_t reset;
    uint32_t cfg_8b10b;
    uint32_t lane_ena;
    uint32_t sysref;
    uint32_t rx_err;
    uint32_t rx_dbg;
    uint32_t status;

    ip_ver    = AXI_REG_READ(base, 0x000);
    ip_cfg    = AXI_REG_READ(base, 0x004);
    reset     = AXI_REG_READ(base, 0x020);
    cfg_8b10b = AXI_REG_READ(base, 0x03C);
    lane_ena  = AXI_REG_READ(base, 0x040);
    sysref    = AXI_REG_READ(base, 0x050);

    /*
     * 注意：
     * 0x058 STAT_RX_ERR 是 read-to-clear。
     * 所以一次打印之后，错误位会被清掉。
     */
    rx_err    = AXI_REG_READ(base, 0x058);

    /*
     * 0x05C 的 Start-of-ILA / Start-of-Data 位
     * 也包含锁存/读清行为。
     */
    rx_dbg    = AXI_REG_READ(base, 0x05C);

    status    = AXI_REG_READ(base, 0x060);

    printf("\r\n========== %s ==========\r\n", name);

    printf("IP_VERSION   = 0x%08lx\r\n", ip_ver);
    printf("IP_CONFIG    = 0x%08lx\r\n", ip_cfg);
    printf("RESET        = 0x%08lx\r\n", reset);
    printf("8B10B_CFG    = 0x%08lx\r\n", cfg_8b10b);
    printf("LANE_ENA     = 0x%08lx\r\n", lane_ena);
    printf("SYSREF_CTRL  = 0x%08lx\r\n", sysref);
    printf("RX_ERR       = 0x%08lx\r\n", rx_err);
    printf("RX_DEBUG     = 0x%08lx\r\n", rx_dbg);
    printf("STATUS       = 0x%08lx\r\n", status);

    printf("\r\n-- RESET --\r\n");
    printf("GT reset busy       = %lu\r\n",
           (reset >> 7) & 0x1);

    printf("core reset register = %lu\r\n",
           (reset >> 5) & 0x1);

    printf("external core reset = %lu\r\n",
           (reset >> 4) & 0x1);

    printf("reset request       = %lu\r\n",
           reset & 0x1);

    printf("\r\n-- 8B10B STATUS --\r\n");
    printf("Alignment error     = %lu\r\n",
           (status >> 15) & 0x1);

    printf("RX data started     = %lu\r\n",
           (status >> 14) & 0x1);

    printf("CGS achieved        = %lu\r\n",
           (status >> 13) & 0x1);

    printf("SYNC achieved       = %lu\r\n",
           (status >> 12) & 0x1);

    printf("SYSREF error        = %lu\r\n",
           (status >> 2) & 0x1);

    printf("SYSREF captured     = %lu\r\n",
           (status >> 1) & 0x1);

    printf("\r\n-- PER LANE DEBUG --\r\n");

    for (int lane = 0; lane < 4; lane++) {
        uint32_t d = (rx_dbg >> (lane * 4)) & 0xF;
        uint32_t e = (rx_err >> (lane * 4)) & 0xF;

        printf("Lane %d:\r\n", lane);

        printf("  K28.5 receiving = %lu\r\n",
               (d >> 0) & 0x1);

        printf("  CGS             = %lu\r\n",
               (d >> 1) & 0x1);

        printf("  ILA detected    = %lu\r\n",
               (d >> 2) & 0x1);

        printf("  DATA detected   = %lu\r\n",
               (d >> 3) & 0x1);

        printf("  Not-in-table    = %lu\r\n",
               (e >> 0) & 0x1);

        printf("  Disparity error = %lu\r\n",
               (e >> 1) & 0x1);

        printf("  Unexpected K    = %lu\r\n",
               (e >> 2) & 0x1);
    }
}
static void jesd204c_dump_tx(uint32_t base)
{
    uint32_t ip_ver    = AXI_REG_READ(base, 0x000);
    uint32_t ip_cfg    = AXI_REG_READ(base, 0x004);
    uint32_t reset     = AXI_REG_READ(base, 0x020);
    uint32_t tx_sync   = AXI_REG_READ(base, 0x028);
    uint32_t subclass  = AXI_REG_READ(base, 0x034);
    uint32_t cfg       = AXI_REG_READ(base, 0x03C);
    uint32_t lanes     = AXI_REG_READ(base, 0x040);
    uint32_t sysref    = AXI_REG_READ(base, 0x050);
    uint32_t status    = AXI_REG_READ(base, 0x060);

    printf("\r\n========== JESD204C TX ==========\r\n");

    printf("IP_VERSION = 0x%08lx\r\n", ip_ver);
    printf("IP_CONFIG  = 0x%08lx\r\n", ip_cfg);
    printf("RESET      = 0x%08lx\r\n", reset);
    printf("TX_SYNC    = 0x%08lx\r\n", tx_sync);
    printf("SUBCLASS   = 0x%08lx\r\n", subclass);
    printf("8B10B_CFG  = 0x%08lx\r\n", cfg);
    printf("LANE_ENA   = 0x%08lx\r\n", lanes);
    printf("SYSREF     = 0x%08lx\r\n", sysref);
    printf("STATUS     = 0x%08lx\r\n", status);

    printf("GT reset busy       = %lu\r\n",
           (reset >> 7) & 1);

    printf("Core reset reg      = %lu\r\n",
           (reset >> 5) & 1);

    printf("External core reset = %lu\r\n",
           (reset >> 4) & 1);

    printf("Reset request       = %lu\r\n",
           reset & 1);

    printf("SYSREF captured     = %lu\r\n",
           (status >> 1) & 1);

    printf("SYSREF error        = %lu\r\n",
           (status >> 2) & 1);
}

uint32_t ad9009_initial_rf1(taliseDevice_t *talDev_rf1,
		jesd_core_array jesd_array_rf1,
		uint32_t jesd_phy0_addr) {
	uint32_t talAction = TALACT_NO_ACTION;
	uint32_t fpgaStatus = 0U;
	uint32_t resetReadback = 0U;
	uint32_t txStatus = 0U;
	uint32_t rxStatus = 0U;
	uint32_t orxStatus = 0U;
	uint32_t sysrefAttempt = 0U;
	talAction = ad9009_hard_reset(talDev_rf1, 1);
	if (talAction != TALACT_NO_ACTION) {
		printf("error: rf1_ad9009_hard_reset() failed\n");
		return talAction;
	}
	talAction = ad9009_init(talDev_rf1);
	if (talAction != TALACT_NO_ACTION) {
		printf("error: rf1_ad9009_init() failed\n");
		return talAction;
	}
	talAction = ad9009_rf_init_setpll(talDev_rf1);
	if (talAction != TALACT_NO_ACTION) {
		printf("error: ad9009_rf_init_setpll() failed\n");
		return talAction;
	}

	talAction = ad9009_mcps(talDev_rf1);
	if (talAction != TALACT_NO_ACTION) {
		printf("error: ad9009_mcps() failed\n");
		return talAction;
	}

	talAction = ad9009_rf_init_cal(talDev_rf1);
	if (talAction != TALACT_NO_ACTION) {
		printf("error: ad9009_rf_init_cal() failed\n");
		return talAction;
	}

	mdelay(1000);
	/*
	 * AD9528 is configured for SYSREF request-by-pin, continuous pattern.
	 * Keep the request low while Talise and all three FPGA JESD cores are
	 * enabled.  A fresh request edge is generated only after reset_done.
	 */
	gpio_direction_output(gpio_lmk_sync, 0);
	mdelay(10);
	talAction = 255;
	while (talAction == 255) {
		gpio_direction_output(gpio_lmk_sync, 0);
		AXI_REG_WRITE(axi_lite_addr, 2 * 4, 1);
		talAction = ad9009_jesd204b_init(talDev_rf1);
		fpgaStatus = 0U;
		if (talAction == TALACT_NO_ACTION)
			fpgaStatus = fpga_jesd204b_init(jesd_array_rf1,
						       jesd_phy0_addr);
		AXI_REG_WRITE(axi_lite_addr, 2 * 4, 0);
//				gpio_direction_output(gpio_lmk_sync, 0);
//				mdelay(1);
//				gpio_direction_output(gpio_lmk_sync, 1);
//				mdelay(1);
//				gpio_direction_output(gpio_lmk_sync, 0);
		resetReadback = AXI_REG_READ(axi_lite_addr, 2 * 4);
		if ((resetReadback & 1U) != 0U) {
			printf("error: external JESD reset did not deassert, readback=0x%08lx\n",
			       (unsigned long)resetReadback);
			talAction = 255;
			continue;
		}
		if (talAction != TALACT_NO_ACTION) {
			printf("error: ad9009_jesd204b_init() failed: 0x%08lx\n",
			       (unsigned long)talAction);
			talAction = 255;
			continue;
		}
		if (fpgaStatus != 0U) {
			printf("error: fpga_jesd204b_init() failed: 0x%08lx\n",
			       (unsigned long)fpgaStatus);
			talAction = 255;
			continue;
		}

		if ((jesd_wait_reset_done(jesd_array_rf1.tx_jesd, 1000U) != 0) ||
		    (jesd_wait_reset_done(jesd_array_rf1.rx_jesd, 1000U) != 0) ||
		    (jesd_wait_reset_done(jesd_array_rf1.orx_jesd, 1000U) != 0)) {
			printf("error: JESD204C reset sequence did not complete\n");
			talAction = 255;
			continue;
		}
		print_tx_ila_readback(jesd_array_rf1.tx_jesd);

		/*
		 * All SYSREF consumers are armed now.  Keep the same initialized link
		 * alive while issuing up to ten request windows.  AD9528 request-by-pin
		 * latency/phase can otherwise make a single short window intermittent.
		 */
		for (sysrefAttempt = 1U; sysrefAttempt <= 10U; sysrefAttempt++) {
			gpio_direction_output(gpio_lmk_sync, 1);
			mdelay(10);
			gpio_direction_output(gpio_lmk_sync, 0);
			mdelay(10);

			jesd_read(jesd_array_rf1.tx_jesd,
				  JESD204_REG_TRX_SYNC_STATUS, &txStatus);
			jesd_read(jesd_array_rf1.rx_jesd,
				  JESD204_REG_TRX_SYNC_STATUS, &rxStatus);
			jesd_read(jesd_array_rf1.orx_jesd,
				  JESD204_REG_TRX_SYNC_STATUS, &orxStatus);
			printf("SYSREF attempt %lu: TX=%08lX RX=%08lX ORX=%08lX\n",
			       (unsigned long)sysrefAttempt,
			       (unsigned long)txStatus,
			       (unsigned long)rxStatus,
			       (unsigned long)orxStatus);
			if (((txStatus & JESD204_TRX_SYSREF_CAPTURED) != 0U) &&
			    ((rxStatus & JESD204_TRX_SYSREF_CAPTURED) != 0U) &&
			    ((orxStatus & JESD204_TRX_SYSREF_CAPTURED) != 0U))
				break;
		}
		if (((txStatus & JESD204_TRX_SYSREF_CAPTURED) == 0U) ||
		    ((rxStatus & JESD204_TRX_SYSREF_CAPTURED) == 0U) ||
		    ((orxStatus & JESD204_TRX_SYSREF_CAPTURED) == 0U)) {
			printf("error: SYSREF was not captured by all three FPGA cores\n");
			talAction = 255;
			continue;
		}
		mdelay(100);

		fpgaStatus = AXI_REG_READ(jesd_phy0_addr, 0x080);
		printf("PHY STATUS after reset release/SYSREF = 0x%08lX "
		       "(tx_reset=%lu rx_reset=%lu qpll0_locked=%lu)\n",
		       (unsigned long)fpgaStatus,
		       (unsigned long)((fpgaStatus >> 4) & 1U),
		       (unsigned long)((fpgaStatus >> 3) & 1U),
		       (unsigned long)(((fpgaStatus >> 1) & 1U) == 0U));
		talAction = jesd204b_check(talDev_rf1, jesd_array_rf1);

		/*jesd204c_dump_rx(XPAR_JESD204C_RX_BASEADDR,
		                 "JESD204C RX");

		jesd204c_dump_rx(XPAR_JESD204C_ORX_BASEADDR,
		                 "JESD204C ORX");

		jesd204c_dump_tx(XPAR_JESD204C_TX_BASEADDR);*/

	}
	mdelay(500);
	talAction = ad9009_radio_on(talDev_rf1);
	if (talAction != TALACT_NO_ACTION) {
		printf("error: rf1_ad9009_radio_on() failed\n");
		return talAction;
	}
	AXI_REG_WRITE(axi_lite_addr, 75 * 4, 0);//rf1_gpio_o
	AXI_REG_WRITE(axi_lite_addr, 75 * 4, 1);//rf1_gpio_o
	AXI_REG_WRITE(axi_lite_addr, 75* 4, 0);//rf1_gpio_o

	return talAction;
}
