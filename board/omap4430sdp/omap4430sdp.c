/*
 * (C) Copyright 2004-2009
 * Texas Instruments, <www.ti.com>
 * Richard Woodruff <r-woodruff2@ti.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <common.h>
#include <asm/arch/mux.h>
#include <asm/io.h>
#include <asm/arch/sys_proto.h>
#include <i2c.h>

/* function protypes */
extern int omap4_mmc_init(void);
extern int select_bus(int, int);
#if defined(OMAP44XX_TABLET_CONFIG)
static int tablet_check_display_boardid(void);
#endif

#define		OMAP44XX_WKUP_CTRL_BASE		0x4A31E000
#if 1
#define M0_SAFE M0
#define M1_SAFE M1
#define M2_SAFE M2
#define M4_SAFE M4
#define M7_SAFE M7
#define M3_SAFE M3
#define M5_SAFE M5
#define M6_SAFE M6
#else
#define M0_SAFE M7
#define M1_SAFE M7
#define M2_SAFE M7
#define M4_SAFE M7
#define M7_SAFE M7
#define M3_SAFE M7
#define M5_SAFE M7
#define M6_SAFE M7
#endif
#define		MV(OFFSET, VALUE)\
			__raw_writew((VALUE), OMAP44XX_CTRL_BASE + (OFFSET));
#define		MV1(OFFSET, VALUE)\
			__raw_writew((VALUE), OMAP44XX_WKUP_CTRL_BASE + (OFFSET));

#define		CP(x)	(CONTROL_PADCONF_##x)
#define		WK(x)	(CONTROL_WKUP_##x)
/*
 * IEN  - Input Enable
 * IDIS - Input Disable
 * PTD  - Pull type Down
 * PTU  - Pull type Up
 * DIS  - Pull type selection is inactive
 * EN   - Pull type selection is active
 * M0   - Mode 0
 * The commented string gives the final mux configuration for that pin
 */


#define MUX_DEFAULT_OMAP4_ALL() \
	MV(CP(GPMC_AD0),	(IEN | M1)) /* sdmmc2_dat0 */ \
	MV(CP(GPMC_AD1),	(IEN | M1)) /* sdmmc2_dat1 */ \
	MV(CP(GPMC_AD2),	(IEN | M1)) /* sdmmc2_dat2 */ \
	MV(CP(GPMC_AD3),	(IEN | M1)) /* sdmmc2_dat3 */ \
	MV(CP(GPMC_AD4),	(IEN | M1)) /* sdmmc2_dat4 */ \
	MV(CP(GPMC_AD5),	(IEN | M1)) /* sdmmc2_dat5 */ \
	MV(CP(GPMC_AD6),	(IEN | M1)) /* sdmmc2_dat6 */ \
	MV(CP(GPMC_AD7),	(IEN | M1)) /* sdmmc2_dat7 */ \
	MV(CP(GPMC_AD8),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M3)) /* Key-home gpio_32 */ \
	MV(CP(GPMC_AD9),	(IEN | DIS | M3)) /* gpio_33 */ \
	MV(CP(GPMC_AD10),	(IEN | DIS | M3)) /* gpio_34 */ \
	MV(CP(GPMC_AD11),	(IEN | DIS | M3)) /* gpio_35 */ \
	MV(CP(GPMC_AD12),	(PTU | IEN | EN | M3_SAFE)) /* LCD-PWR-EN gpio_36 */ \
	MV(CP(GPMC_AD13),	(IEN | DIS | M3)) /* TP-nINT-IN  gpio_37 */ \
	MV(CP(GPMC_AD14),	(PTD | IDIS | OFF_EN | OFF_PD | OFF_OUT_PTD | M3)) /* BL-PWR_nEN gpio_38 */ \
	MV(CP(GPMC_AD15),	(PTD | IDIS | M3)) /* TP-RESET gpio_39 */ \
	MV(CP(GPMC_A16),	(IEN | DIS | M3)) /* gpio_40 */ \
	MV(CP(GPMC_A17),	(IEN | DIS | M3)) /* gpio_41 */ \
	MV(CP(GPMC_A18),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M1)) /* kpd_row6 */ \
	MV(CP(GPMC_A19),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M1)) /* kpd_row7 */ \
	MV(CP(GPMC_A20),	(M3_SAFE)) /*lcd-cab0 gpio_44 */ \
	MV(CP(GPMC_A21),	(M3_SAFE)) /*lcd-cab1 gpio_45 */ \
	MV(CP(GPMC_A22),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M1)) /* kpd_col6 */ \
	MV(CP(GPMC_A23),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M1)) /* kpd_col7 */ \
	MV(CP(GPMC_A24),	(IEN | DIS | M3)) /* gpio_48 */ \
	MV(CP(GPMC_A25),	(IEN | DIS | M3)) /* gpio_49 */ \
	MV(CP(GPMC_NCS0),	(IEN | DIS | M3)) /* gpio_50 */ \
	MV(CP(GPMC_NCS1),	(IEN | DIS | M3)) /* gpio_51 */ \
	MV(CP(GPMC_NCS2),	(M3_SAFE)) /* gpio_52 */ \
	MV(CP(GPMC_NCS3),	(IEN | M3)) /* gpio_53 */ \
	MV(CP(GPMC_NWP),	(M3_SAFE)) /* gpio_54 */ \
	MV(CP(GPMC_CLK),	(IEN | M3_SAFE)) /* gpio_55 */ \
	MV(CP(GPMC_NADV_ALE),	(M3_SAFE)) /* gpio_56 */ \
	MV(CP(GPMC_NOE),	(OFF_EN | OFF_OUT_PTD | M1)) /* sdmmc2_clk */ \
	MV(CP(GPMC_NWE),	(IEN | M1)) /* sdmmc2_cmd */ \
	MV(CP(GPMC_NBE0_CLE),	(M3_SAFE)) /* gpio_59 */ \
	MV(CP(GPMC_NBE1),	(M3)) /* gpio_60 */ \
	MV(CP(GPMC_WAIT0),	(M3_SAFE)) /* gpio_61 */ \
	MV(CP(GPMC_WAIT1),	(IEN | M3)) /* gpio_62 */ \
	MV(CP(C2C_DATA11),	(IEN | M3 )) /* gpio_100 */ \
	MV(CP(C2C_DATA12),	(M3)) /* gpio_101 */ \
	MV(CP(C2C_DATA13),	(IEN | M3)) /* gpio_102 */ \
	MV(CP(C2C_DATA14),	(IEN | M3)) /* gpio_103 */ \
	MV(CP(C2C_DATA15),	(M3_SAFE)) /* gpio_104 */ \
	MV(CP(HDMI_HPD),	(M3)) /* gpio_63 */ \
	MV(CP(HDMI_CEC),	(M3)) /* gpio_64 */ \
	MV(CP(HDMI_DDC_SCL),	(IEN | M3)) /* gpio_65 */ \
	MV(CP(HDMI_DDC_SDA),	(IEN | M3)) /*gpio_66 */ \
	MV(CP(CSI21_DX0),	(M0_SAFE)) /* csi21_dx0 */ \
	MV(CP(CSI21_DY0),	(M0_SAFE)) /* csi21_dy0 */ \
	MV(CP(CSI21_DX1),	(M0_SAFE)) /* csi21_dx1 */ \
	MV(CP(CSI21_DY1),	(M0_SAFE)) /* csi21_dy1 */ \
	MV(CP(CSI21_DX2),	(M0_SAFE)) /* csi21_dx2 */ \
	MV(CP(CSI21_DY2),	(M0_SAFE)) /* csi21_dy2 */ \
	MV(CP(CSI21_DX3),	(M7_SAFE)) /* csi21_dx3 */ \
	MV(CP(CSI21_DY3),	(M7_SAFE)) /* csi21_dy3 */ \
	MV(CP(CSI21_DX4),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M7)) /* gpi_75 */ \
	MV(CP(CSI21_DY4),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M7)) /* gpi_76 */ \
	MV(CP(CSI22_DX0),	(M0_SAFE)) /* csi22_dx0 */ \
	MV(CP(CSI22_DY0),	(M0_SAFE)) /* csi22_dy0 */ \
	MV(CP(CSI22_DX1),	(M0_SAFE)) /* csi22_dx1 */ \
	MV(CP(CSI22_DY1),	(M0_SAFE)) /* csi22_dy1 */ \
	MV(CP(CAM_SHUTTER),	(IEN | M3)) /* gpio_81 */ \
	MV(CP(CAM_STROBE),	(IEN | M3)) /* gpio_82 */ \
	MV(CP(CAM_GLOBALRESET),	(M3)) /* gpio_83 */ \
	MV(CP(USBB1_ULPITLL_CLK),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* usbb1_ulpiphy_clk */ \
	MV(CP(USBB1_ULPITLL_STP),	(PTU | OFF_EN | OFF_OUT_PTD | OFF_OUT | M0)) /* usbb1_ulpiphy_stp */ \
	MV(CP(USBB1_ULPITLL_DIR),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* usbb1_ulpiphy_dir */ \
	MV(CP(USBB1_ULPITLL_NXT),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* usbb1_ulpiphy_nxt */ \
	MV(CP(USBB1_ULPITLL_DAT0),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* usbb1_ulpiphy_dat0 */ \
	MV(CP(USBB1_ULPITLL_DAT1),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* usbb1_ulpiphy_dat1 */ \
	MV(CP(USBB1_ULPITLL_DAT2),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* usbb1_ulpiphy_dat2 */ \
	MV(CP(USBB1_ULPITLL_DAT3),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* usbb1_ulpiphy_dat3 */ \
	MV(CP(USBB1_ULPITLL_DAT4),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* usbb1_ulpiphy_dat4 */ \
	MV(CP(USBB1_ULPITLL_DAT5),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* usbb1_ulpiphy_dat5 */ \
	MV(CP(USBB1_ULPITLL_DAT6),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* usbb1_ulpiphy_dat6 */ \
	MV(CP(USBB1_ULPITLL_DAT7),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* usbb1_ulpiphy_dat7 */ \
	MV(CP(USBB1_HSIC_DATA),	(M3)) /* gpio_96 */ \
	MV(CP(USBB1_HSIC_STROBE),(M3)) /* gpio_97 */ \
	MV(CP(USBC1_ICUSB_DP),	( IEN | M3)) /* gpio_98 */ \
	MV(CP(USBC1_ICUSB_DM),	(IEN |M3)) /* gpio_99 */ \
	MV(CP(SDMMC1_CLK),	(OFF_EN | OFF_OUT_PTD | M0)) /* sdmmc1_clk */ \
	MV(CP(SDMMC1_CMD),	(M0)) /* sdmmc1_cmd */ \
	MV(CP(SDMMC1_DAT0),	(IEN | M0)) /* sdmmc1_dat0 */ \
	MV(CP(SDMMC1_DAT1),	(IEN | M0)) /* sdmmc1_dat1 */ \
	MV(CP(SDMMC1_DAT2),	(IEN | M0)) /* sdmmc1_dat2 */ \
	MV(CP(SDMMC1_DAT3),	(IEN | M0)) /* sdmmc1_dat3 */ \
	MV(CP(SDMMC1_DAT4),	(M3)) /* gpio_106 */ \
	MV(CP(SDMMC1_DAT5),	(M3)) /* gpio_107 */ \
	MV(CP(SDMMC1_DAT6),	(M3)) /* gpio_108 */ \
	MV(CP(SDMMC1_DAT7),	(M3)) /* gpio_109 */ \
	MV(CP(ABE_MCBSP2_CLKX),	(IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* abe_mcbsp2_clkx */ \
	MV(CP(ABE_MCBSP2_DR),	(IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* abe_mcbsp2_dr */ \
	MV(CP(ABE_MCBSP2_DX),	(OFF_EN | OFF_OUT_PTD | M0)) /* abe_mcbsp2_dx */ \
	MV(CP(ABE_MCBSP2_FSX),	(IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* abe_mcbsp2_fsx */ \
	MV(CP(ABE_MCBSP1_CLKX),	(OFF_EN | OFF_PD | OFF_IN | M3))  /* gpio_114 */ \
	MV(CP(ABE_MCBSP1_DR),	(IEN | OFF_EN | OFF_OUT_PTD | OFF_OUT | M3))  /* gpio_115 */ \
	MV(CP(ABE_MCBSP1_DX),	(IEN | OFF_EN | OFF_PD | OFF_IN | M1))  /* sdmmc3_dat2 */ \
	MV(CP(ABE_MCBSP1_FSX),	(IEN | OFF_EN | OFF_PD | OFF_IN | M1))  /* sdmmc3_dat3 */ \
	MV(CP(ABE_PDM_UL_DATA),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M0_SAFE)) /* abe_pdm_ul_data */ \
	MV(CP(ABE_PDM_DL_DATA),	(PTD | OFF_EN | OFF_PD | OFF_IN | M0_SAFE)) /* abe_pdm_dl_data */ \
	MV(CP(ABE_PDM_FRAME),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M0_SAFE)) /* abe_pdm_frame */ \
	MV(CP(ABE_PDM_LB_CLK),	(OFF_EN | OFF_PD | OFF_IN | M0_SAFE)) /* abe_pdm_lb_clk */ \
	MV(CP(ABE_CLKS),	(OFF_EN | OFF_PD | OFF_IN | M3))  /* gpio_118 */ \
	MV(CP(ABE_DMIC_CLK1),	(M0_SAFE)) /* abe_dmic_clk1 */ \
	MV(CP(ABE_DMIC_DIN1),	(M0_SAFE)) /* abe_dmic_din1 */ \
	MV(CP(ABE_DMIC_DIN2),	(M5_SAFE)) /* abe_dmic_din2 */ \
	MV(CP(ABE_DMIC_DIN3),	(M0_SAFE)) /* abe_dmic_din3 */ \
	MV(CP(UART2_CTS),	(OFF_PD | OFF_EN | OFF_IN | M1))  /* sdmmc3_clk */ \
	MV(CP(UART2_RTS),	(OFF_EN | OFF_PD | OFF_IN | M1))  /* sdmmc3_cmd */ \
	MV(CP(UART2_RX),	(IEN | OFF_EN | OFF_PD | OFF_IN | M1))  /* sdmmc3_dat0 */ \
	MV(CP(UART2_TX),	(IEN | OFF_EN | OFF_PD | OFF_IN | M1))  /* sdmmc3_dat1 */ \
	MV(CP(HDQ_SIO),	(M3_SAFE)) /* gpio_127 */ \
	MV(CP(I2C1_SCL),	(PTU | IEN | M0)) /* i2c1_scl */ \
	MV(CP(I2C1_SDA),	(PTU | IEN | M0)) /* i2c1_sda */ \
	MV(CP(I2C2_SCL),	(PTU | IEN | M0)) /* i2c2_scl */ \
	MV(CP(I2C2_SDA),	(PTU | IEN | M0)) /* i2c2_sda */ \
	MV(CP(I2C3_SCL),	(PTU | IEN | M0)) /* i2c3_scl */ \
	MV(CP(I2C3_SDA),	(M3)) /* gpio_131 */ \
	MV(CP(I2C4_SCL),	(PTU | IEN | M0)) /* i2c4_scl */ \
	MV(CP(I2C4_SDA),	(PTU | IEN | M0)) /* i2c4_sda */ \
	MV(CP(MCSPI1_CLK),	(M3)) /* gpio_134 */ \
	MV(CP(MCSPI1_SOMI),	(IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* mcspi1_somi */ \
	MV(CP(MCSPI1_SIMO),	(IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* mcspi1_simo */ \
	MV(CP(MCSPI1_CS0),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* mcspi1_cs0 */ \
	MV(CP(MCSPI1_CS1),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M1)) /* uart1_rx */ \
	MV(CP(MCSPI1_CS2),	(OFF_EN | OFF_OUT_PTU | M1)) /* uart1_cts */ \
	MV(CP(MCSPI1_CS3),	(M1)) /* uart1_rts */ \
	MV(CP(UART3_CTS_RCTX),	(M1)) /* uart1_tx */ \
	MV(CP(UART3_RTS_SD),	(M3)) /* uart3_rts_sd */ \
	MV(CP(UART3_RX_IRRX),	(M3)) /* gpio_143*/ \
	MV(CP(UART3_TX_IRTX),	(M4)) /* gpio_144 */ \
	MV(CP(SDMMC5_CLK),	(M3)) /* sdmmc5_clk */ \
	MV(CP(SDMMC5_CMD),	(PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* sdmmc5_cmd */ \
	MV(CP(SDMMC5_DAT0),	(PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* sdmmc5_dat0 */ \
	MV(CP(SDMMC5_DAT1),	(PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* sdmmc5_dat1 */ \
	MV(CP(SDMMC5_DAT2),	(PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* sdmmc5_dat2 */ \
	MV(CP(SDMMC5_DAT3),	(PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* sdmmc5_dat3 */ \
	MV(CP(MCSPI4_CLK),	(IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* mcspi4_clk */ \
	MV(CP(MCSPI4_SIMO),	(IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* mcspi4_simo */ \
	MV(CP(MCSPI4_SOMI),	(IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* mcspi4_somi */ \
	MV(CP(MCSPI4_CS0),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* mcspi4_cs0 */ \
	MV(CP(UART4_RX),	(IEN | M0)) /* uart4_rx */ \
	MV(CP(UART4_TX),	(M0)) /* uart4_tx */ \
	MV(CP(USBB2_ULPITLL_CLK),	(PTU | IEN | M3)) /* gpio_157 */ \
	MV(CP(USBB2_ULPITLL_STP),	(M5)) /* dispc2_data23 */ \
	MV(CP(USBB2_ULPITLL_DIR),	(M5)) /* dispc2_data22 */ \
	MV(CP(USBB2_ULPITLL_NXT),	(M5)) /* dispc2_data21 */ \
	MV(CP(USBB2_ULPITLL_DAT0),	(M5)) /* dispc2_data20 */ \
	MV(CP(USBB2_ULPITLL_DAT1),	(M5)) /* dispc2_data19 */ \
	MV(CP(USBB2_ULPITLL_DAT2),	(M5)) /* dispc2_data18 */ \
	MV(CP(USBB2_ULPITLL_DAT3),	(M5)) /* dispc2_data15 */ \
	MV(CP(USBB2_ULPITLL_DAT4),	(M5)) /* dispc2_data14 */ \
	MV(CP(USBB2_ULPITLL_DAT5),	(M5)) /* dispc2_data13 */ \
	MV(CP(USBB2_ULPITLL_DAT6),	(M5)) /* dispc2_data12 */ \
	MV(CP(USBB2_ULPITLL_DAT7),	(M5)) /* dispc2_data11 */ \
	MV(CP(USBB2_HSIC_DATA),	(M3)) /* gpio_169 */ \
	MV(CP(USBB2_HSIC_STROBE),	(M3)) /* gpio_170 */ \
	MV(CP(UNIPRO_TY1),	(OFF_EN | OFF_PD | OFF_IN | M0)) /* kpd_col0 */ \
	MV(CP(UNIPRO_TY2),	(M3)) /* kpd_col2 */ \
	MV(CP(UNIPRO_RY1),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* kpd_row0 */ \
	MV(CP(UNIPRO_RX2),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* kpd_row1 */ \
	MV(CP(USBA0_OTG_CE),	(PTU | OFF_EN | OFF_OUT |OFF_OUT_PTD | M0)) /* usba0_otg_ce */ \
	MV(CP(USBA0_OTG_DP),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* usba0_otg_dp */ \
	MV(CP(USBA0_OTG_DM),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* usba0_otg_dm */ \
	MV(CP(FREF_CLK1_OUT),	(M0_SAFE)) /* fref_clk1_out */ \
	MV(CP(FREF_CLK2_OUT),	(M0_SAFE)) /* fref_clk2_out */ \
	MV(CP(SYS_NIRQ1),	(PTU | IEN | M0)) /* sys_nirq1 */ \
	MV(CP(SYS_NIRQ2),	(PTU | IEN | M0)) /* sys_nirq2 */ \
	MV(CP(SYS_BOOT0),	(M0)) /* SYSBOOT0 */ \
	MV(CP(SYS_BOOT1),	(M0)) /* SYSBOOT1 */ \
	MV(CP(SYS_BOOT2),	(M0)) /* SYSBOOT2 */ \
	MV(CP(SYS_BOOT3),	(M0)) /* SYSBOOT3 */ \
	MV(CP(SYS_BOOT4),	(M0)) /* SYSBOOT4 */ \
	MV(CP(SYS_BOOT5),	(M0)) /* SYSBOOT5 */ \
	MV(CP(DPM_EMU0),	(M0_SAFE)) /* dpm_emu0 */ \
	MV(CP(DPM_EMU1),	(M0_SAFE)) /* dpm_emu1 */ \
	MV(CP(DPM_EMU2),	(M0_SAFE)) /* dpm_emu2 */ \
	MV(CP(DPM_EMU3),	(M5)) /* dispc2_data10 */ \
	MV(CP(DPM_EMU4),	(M5)) /* dispc2_data9 */ \
	MV(CP(DPM_EMU5),	(M5)) /* dispc2_data16 */ \
	MV(CP(DPM_EMU6),	(M5)) /* dispc2_data17 */ \
	MV(CP(DPM_EMU7),	(M5)) /* dispc2_hsync */ \
	MV(CP(DPM_EMU8),	(M5)) /* dispc2_pclk */ \
	MV(CP(DPM_EMU9),	(M5)) /* dispc2_vsync */ \
	MV(CP(DPM_EMU10),	(M5)) /* dispc2_de */ \
	MV(CP(DPM_EMU11),	(M5)) /* dispc2_data8 */ \
	MV(CP(DPM_EMU12),	(M5)) /* dispc2_data7 */ \
	MV(CP(DPM_EMU13),	(M5)) /* dispc2_data6 */ \
	MV(CP(DPM_EMU14),	(M5)) /* dispc2_data5 */ \
	MV(CP(DPM_EMU15),	(M5)) /* dispc2_data4 */ \
	MV(CP(DPM_EMU16),	(M5)) /* dispc2_data3/dmtimer8_pwm_evt */ \
	MV(CP(DPM_EMU17),	(M5)) /* dispc2_data2 */ \
	MV(CP(DPM_EMU18),	(M5)) /* dispc2_data1 */ \
	MV(CP(DPM_EMU19),	(M5)) /* dispc2_data0 */ \
	MV1(WK(PAD0_SIM_IO),	(M0_SAFE)) /* sim_io */ \
	MV1(WK(PAD1_SIM_CLK),	(M0_SAFE)) /* sim_clk */ \
	MV1(WK(PAD0_SIM_RESET),	(M0_SAFE)) /* sim_reset */ \
	MV1(WK(PAD1_SIM_CD),	(M0_SAFE)) /* sim_cd */ \
	MV1(WK(PAD0_SIM_PWRCTRL),	(M0_SAFE)) /* sim_pwrctrl */ \
	MV1(WK(PAD1_SR_SCL),	(PTU | IEN | M0)) /* sr_scl */ \
	MV1(WK(PAD0_SR_SDA),	(PTU | IEN | M0)) /* sr_sda */ \
	MV1(WK(PAD1_FREF_XTAL_IN),	(M0_SAFE)) /* # */ \
	MV1(WK(PAD0_FREF_SLICER_IN),	(M0_SAFE)) /* fref_slicer_in */ \
	MV1(WK(PAD1_FREF_CLK_IOREQ),	(M0_SAFE)) /* fref_clk_ioreq */ \
	MV1(WK(PAD0_FREF_CLK0_OUT),	(M3_SAFE)) /* sys_drm_msecure */ \
	MV1(WK(PAD1_FREF_CLK3_REQ),	(M0_SAFE)) /* # */ \
	MV1(WK(PAD0_FREF_CLK3_OUT),	(M0_SAFE)) /* fref_clk3_out */ \
	MV1(WK(PAD1_FREF_CLK4_REQ),	(M0_SAFE)) /* # */ \
	MV1(WK(PAD0_FREF_CLK4_OUT),	(M0_SAFE)) /* # */ \
	MV1(WK(PAD1_SYS_32K),	(IEN | M0_SAFE)) /* sys_32k */ \
	MV1(WK(PAD0_SYS_NRESPWRON),	(IEN | M0_SAFE)) /* sys_nrespwron */ \
	MV1(WK(PAD1_SYS_NRESWARM),	(IEN | M0_SAFE)) /* sys_nreswarm */ \
	MV1(WK(PAD0_SYS_PWR_REQ),	(M0_SAFE)) /* sys_pwr_req */ \
	MV1(WK(PAD1_SYS_PWRON_RESET),	(IEN | M3)) /* gpio_wk29 */ \
	MV1(WK(PAD0_SYS_BOOT6),	(M0)) /* SYSBOOT6 */ \
	MV1(WK(PAD1_SYS_BOOT7),	(M0)) /* SYSBOOT7 */ \
	MV1(WK(PAD1_JTAG_TCK),	(IEN | M0)) /* jtag_tck */ \
	MV1(WK(PAD0_JTAG_RTCK),	(M0)) /* jtag_rtck */ \
	MV1(WK(PAD1_JTAG_TMS_TMSC),	(IEN | M0)) /* jtag_tms_tmsc */ \
	MV1(WK(PAD0_JTAG_TDI),	(IEN | M0)) /* jtag_tdi */ \
	MV1(WK(PAD1_JTAG_TDO),	(M0)) 		  /* jtag_tdo */

/**********************************************************
 * Routine: set_muxconf_regs
 * Description: Setting up the configuration Mux registers
 *              specific to the hardware. Many pins need
 *              to be moved from protect to primary mode.
 *********************************************************/
void set_muxconf_regs(void)
{
	MUX_DEFAULT_OMAP4_ALL();
	return;
}

/******************************************************************************
 * Routine: update_mux()
 * Description:Update balls which are different between boards.  All should be
 *             updated to match functionality.  However, I'm only updating ones
 *             which I'll be using for now.  When power comes into play they
 *             all need updating.
 *****************************************************************************/
void update_mux(u32 btype, u32 mtype)
{
	/* REVISIT  */
	return;

}

#ifdef CONFIG_BOARD_REVISION

#define GPIO_HW_ID5	49
#define GPIO_HW_ID6	50
#define GPIO_HW_ID7 51

static inline ulong identify_board_revision(void)
{
	u8 hw_id5, hw_id6, hw_id7;

	hw_id5 = gpio_read(GPIO_HW_ID5);
	hw_id6 = gpio_read(GPIO_HW_ID6);
	hw_id7 = gpio_read(GPIO_HW_ID7);

	return ((hw_id5 << 2) + (hw_id6 << 1) + (hw_id7));
}
#endif

/*****************************************
 * Routine: board_init
 * Description: Early hardware init.
 *****************************************/
int board_init(void)
{
	DECLARE_GLOBAL_DATA_PTR;

	gpmc_init();

#if 0 /* No eMMC env partition for now */
	/* Intializing env functional pointers with eMMC */
	boot_env_get_char_spec = mmc_env_get_char_spec;
	boot_env_init = mmc_env_init;
	boot_saveenv = mmc_saveenv;
	boot_env_relocate_spec = mmc_env_relocate_spec;
	env_ptr = (env_t *) (CFG_FLASH_BASE + CFG_ENV_OFFSET);
	env_name_spec = mmc_env_name_spec;
#endif


	/* board id for Linux */
#if defined(OMAP44XX_TABLET_CONFIG)
	gd->bd->bi_arch_number = MACH_TYPE_OMAP_BLAZE;
#else
	gd->bd->bi_arch_number = MACH_TYPE_OMAP4_ACCLAIM;
#endif
	gd->bd->bi_boot_params = (0x80000000 + 0x100); /* boot param addr */
	gd->bd->bi_board_revision = identify_board_revision();

	return 0;
}


/*****************************************
 * Routine: board_late_init
 * Description: Late hardware init.
 *****************************************/
int board_late_init(void)
{
	int status;

	/* mmc */
	if( (status = omap4_mmc_init()) != 0) {
		return status;
	}

#if defined(OMAP44XX_TABLET_CONFIG)
	/* Query display board EEPROM information */
	status = tablet_check_display_boardid();
#endif

	return status;
}

/****************************************************************************
 * check_fpga_revision number: the rev number should be a or b
 ***************************************************************************/
inline u16 check_fpga_rev(void)
{
	return __raw_readw(FPGA_REV_REGISTER);
}

const char* board_rev_string(ulong btype)
{
    switch (btype) {
        case ACCLAIM_EVT1A:
            return "EVT1A";
        case ACCLAIM_EVT1B:
            return "EVT1B";
        case ACCLAIM_EVT2:
            return "EVT2";
        case ACCLAIM_DVT:
            return "DVT";
        default:
            return "Unknown";
    }
}

ulong get_board_revision(void)
{
	DECLARE_GLOBAL_DATA_PTR;
	return gd->bd->bi_board_revision;
}

/****************************************************************************
 * check_uieeprom_avail: Check FPGA Availability
 * OnBoard DEBUG FPGA registers need to be ready for us to proceed
 * Required to retrieve the bootmode also.
 ***************************************************************************/
int check_uieeprom_avail(void)
{
	volatile unsigned short *ui_brd_name =
	    (volatile unsigned short *)EEPROM_UI_BRD + 8;
	int count = 1000;

	/* Check if UI revision Name is already updated.
	 * if this is not done, we wait a bit to give a chance
	 * to update. This is nice to do as the Main board FPGA
	 * gets a chance to know off all it's components and we can continue
	 * to work normally
	 * Currently taking 269* udelay(1000) to update this on poweron
	 * from flash!
	 */
	while ((*ui_brd_name == 0x00) && count) {
		udelay(200);
		count--;
	}
	/* Timed out count will be 0? */
	return count;
}

/***********************************************************************
 * get_board_type() - get board type based on current production stats.
 *  - NOTE-1-: 2 I2C EEPROMs will someday be populated with proper info.
 *    when they are available we can get info from there.  This should
 *    be correct of all known boards up until today.
 *  - NOTE-2- EEPROMs are populated but they are updated very slowly.  To
 *    avoid waiting on them we will use ES version of the chip to get info.
 *    A later version of the FPGA migth solve their speed issue.
 ************************************************************************/
u32 get_board_type(void)
{
	return SDP_4430_V1;
}

#if defined(OMAP44XX_TABLET_CONFIG)
/***********************************************************************
 * tablet_pass_boardid()
 *  Pass display board id to kernel via setting environment variables.
 ************************************************************************/
static void tablet_pass_boardid(char *sPanel_id)
{
	char buf[256] = { 0 };
	char *cmdline;
	int len;

#if defined(CONFIG_OMAP4_ANDROID_CMD_LINE)
	cmdline = getenv("android.bootargs.extra");
#else
	cmdline = getenv("bootargs");
#endif

	/* existing cmdline? */
	if (cmdline)
		strncpy(buf, cmdline, sizeof(buf) - 1);

	/* does it fit? */
	len = strlen(buf) + strlen(sPanel_id) + 18;
	if (sizeof(buf) < len)
		return;

	strcat(buf, " omapdss.board_id=");
	strcat(buf, sPanel_id);

#if defined(CONFIG_OMAP4_ANDROID_CMD_LINE)
	setenv("android.bootargs.extra", buf);
#else
	setenv("bootargs", buf);
#endif
	return;
}
/***********************************************************************
 * tablet_read_i2c()
 *  Selects required i2c bus
 *  Reads data from i2c device.
 *  Restores default i2c settings
 ************************************************************************/
static int tablet_read_i2c(int bus, uchar chip, int off, char *sBuf, int len)
{
	int status;

	/* configure I2C bus */
	if ((status = select_bus(bus, CFG_I2C_SPEED)) != 0 ) {
		printf("Setting bus[%d]: FAILED", bus);
		return status;
	}

	/* is present? */
	if ((status = i2c_probe(chip)) !=0 ) {
		debug("Probing %x failed\n", (int) chip);
		return status;
	}

	/* read buffer */
	status = i2c_read(chip, off, 1, (uchar *)sBuf, len);

	/* restore default settings */
	if (select_bus(CFG_I2C_BUS, CFG_I2C_SPEED) != 0)
		printf("Setting i2c defaults: FAILED\n");

	return status;
}
/***********************************************************************
 * tablet_check_display_boardid() - check display board id.
 *  Reads display board EEPROM information and modifies bootargs to pass
 *  board information to kernel as "omapdss.board_id".
 ************************************************************************/
static int tablet_check_display_boardid(void)
{
	char sBuf[10] = { 0 };
	int status;

	/* Read display board eeprom:
	 *  12-19  board information (XXXX-XXX)
	 */
	status = tablet_read_i2c(CFG_DISP_I2C_BUS,
				 CFG_DISP_EEPROM_I2C_ADDR, 12, sBuf, 8);
	/* pass information to kernel */
	if (!status) {
		debug("board_id: %s\n", sBuf);
		tablet_pass_boardid((char *)sBuf);
	}

	return 0;
}
#endif

