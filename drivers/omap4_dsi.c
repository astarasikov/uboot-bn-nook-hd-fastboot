#include "omap4_dsi.h"
#include <bn_boot.h>

#include <asm/io.h>

#define DSI_REVISION		(0x48044000)
#define DSI_SYSCONFIG           (0x48044010)
#define DSI_SYSSTATUS		(0x48044014)
#define DSI_IRQSTATUS		(0x48044018)
#define DSI_IRQENABLE           (0x4804401c)
#define DSI_CTRL                (0x48044040)
#define DSI_CLK_CTRL		(0x48044054)
#define DSI_PLL_GO		(0x48044308)
#define DSI_PLL_STATUS		(0x48044304)
#define DSI_PLL_CONTROL		(0x48044300)
#define DSI_PLL_CONFIGURATION1	(0x4804430C)
#define DSI_PLL_CONFIGURATION2	(0x48044310)
#define DSI_COMPLEXIO_CFG1	(0x48044048)
#define DSI_COMPLEXIO_CFG2	(0x48044078)
#define DSI_TIMING1		(0x48044058)
#define DSI_TIMING2		(0x4804405C)
#define DSI_VM_TIMING1		(0x48044060)
#define DSI_VM_TIMING2 		(0x48044064)
#define DSI_VM_TIMING3 		(0x48044068)
#define DSI_VM_TIMING4		(0x48044080)
#define DSI_VM_TIMING5		(0x48044088)
#define DSI_VM_TIMING6		(0x4804408C)
#define DSI_STOPCLK_TIMING	(0x48044094)
#define DSI_DSIPHY_CFG0 	(0x48044200)
#define DSI_DSIPHY_CFG1 	(0x48044204)
#define DSI_DSIPHY_CFG2 	(0x48044208)
#define DSI_DSIPHY_CFG5		(0x48044214)
#define DSI_CLK_TIMING 		(0x4804406C)
#define DSI_VM_TIMING7 		(0x48044090)
#define DSI_VC_CTRL_0 		(0x48044100)
#define DSI_VC_IRQSTATUS_0	(0x48044118)
#define DSI_VC_IRQENABLE_0	(0x4804411C)
#define DSI_COMPLEXIO_IRQSTATUS (0x4804404C)
#define DSI_COMPLEXIO_IRQENABLE (0x48044050)
#define DSI_VC_LONG_PACKET_HEADER_0 	(0x48044108)
#define DSI_VC_LONG_PACKET_PAYLOAD_0	(0x4804410C)
#define DSI_VC_SHORT_PACKET_HEADER_0	(0x48044110)
#define DSI_TX_FIFO_VC_SIZE		(0x48044070)
#define DSI_RX_FIFO_VC_SIZE		(0x48044074)
#define DSI_RX_FIFO_VC_FULLNESS		(0x4804407C)
#define DSI_TX_FIFO_VC_EMPTINESS	(0x48044084)

#define DSI_VC_CTRL_1 		(0x48044120)
#define DSI_VC_IRQSTATUS_1	(0x48044138)
#define DSI_VC_IRQENABLE_1	(0x4804413C)
#define DSI_VC_CTRL_2 		(0x48044140)
#define DSI_VC_IRQSTATUS_2	(0x48044158)
#define DSI_VC_IRQENABLE_2	(0x4804415C)
#define DSI_VC_CTRL_3 		(0x48044160)
#define DSI_VC_IRQSTATUS_3	(0x48044178)
#define DSI_VC_IRQENABLE_3	(0x4804417C)

#define DISPC_SYSSTATUS		(0x48041014)
#define DISPC_SYSCONFIG		(0x48041010)
#define DISPC_DIVISOR1		(0x48041070)
#define DISPC_TIMING_H1		(0x48041064)
#define DISPC_TIMING_V1 	(0x48041068)
#define DISPC_SIZE_LCD1		(0x4804107C)
#define DISPC_CONTROL1		(0x48041040)
#define DISPC_POL_FREQ1		(0x4804106C)
#define DISPC_IRQSTATUS		(0x48041018)
#define DISPC_IRQENABLE		(0x4804101C)
#define DISPC_GLOBAL_ALPHA	(0x48041074)
#define DISPC_GFX_ROW_INC	(0x480410AC)
#define DISPC_GFX_PIXEL_INC	(0x480410B0)
#define DISPC_CONFIG1		(0x48041044)
#define DISPC_GFX_ATTRIBUTES	(0x480410A0)
#define DISPC_GFX_BUF_THRESHOLD	(0x480410A4)
#define DISPC_GFX_BA_0		(0x48041080)
#define DISPC_GFX_BA_1		(0x48041084)
#define DISPC_GFX_POSITION	(0x48041088)
#define DISPC_GFX_SIZE		(0x4804108C)
#define DISPC_GFX_PRELOAD	(0x4804122C)
#define DISPC_DEFAULT_COLOR0	(0x4804104C)
#define DISPC_DEFAULT_COLOR1	(0x48041050)

#define DSS_CTRL		(0x48040040)
#define DSS_STATUS		(0x4804005C)

#define CONTROL_DSIPHY		(0x4A100618)

#define FLD_MASK(start, end)	(((1 << ((start) - (end) + 1)) - 1) << (end))
#define FLD_VAL(val, start, end) (((val) << (end)) & FLD_MASK(start, end))
#define FLD_GET(val, start, end) (((val) & FLD_MASK(start, end)) >> (end))
#define FLD_MOD(orig, val, start, end) \
	(((orig) & ~FLD_MASK(start, end)) | FLD_VAL(val, start, end))

#define REG_GET(reg, start, end) \
	FLD_GET(__raw_readl(reg), start, end)

#define REG_FLD_MOD(reg, val, start, end) __raw_writel(FLD_MOD(__raw_readl(reg), val, start, end), reg)

#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))
#define DSI_DT_GENERIC_SHORT_WRITE	0x03
#define DSI_DT_TURN_ON_PERIPHERAL	0x32

#if 1
void dsi_dump_regs(int line)
{

#define DUMP_REG(reg)  printf(#reg "\t0x%08x\n", __raw_readl(reg))

printf("%d:\n", line);
DUMP_REG(DSI_REVISION);
DUMP_REG(DSI_SYSCONFIG);
DUMP_REG(DSI_SYSSTATUS);
DUMP_REG(DSI_IRQSTATUS);
DUMP_REG(DSI_IRQENABLE);
DUMP_REG(DSI_CTRL);
DUMP_REG(DSI_COMPLEXIO_CFG1);
DUMP_REG(DSI_COMPLEXIO_IRQSTATUS);
DUMP_REG(DSI_COMPLEXIO_IRQENABLE);
DUMP_REG(DSI_CLK_CTRL);
DUMP_REG(DSI_TIMING1);
DUMP_REG(DSI_TIMING2);
DUMP_REG(DSI_VM_TIMING1);
DUMP_REG(DSI_VM_TIMING2);
DUMP_REG(DSI_VM_TIMING3);
DUMP_REG(DSI_CLK_TIMING);
DUMP_REG(DSI_TX_FIFO_VC_SIZE);
DUMP_REG(DSI_RX_FIFO_VC_SIZE);
DUMP_REG(DSI_COMPLEXIO_CFG2);
DUMP_REG(DSI_RX_FIFO_VC_FULLNESS);
DUMP_REG(DSI_VM_TIMING4);
DUMP_REG(DSI_TX_FIFO_VC_EMPTINESS);
DUMP_REG(DSI_VM_TIMING5);
DUMP_REG(DSI_VM_TIMING6);
DUMP_REG(DSI_VM_TIMING7);
DUMP_REG(DSI_STOPCLK_TIMING);

DUMP_REG(DSI_VC_CTRL_0);
DUMP_REG(DSI_VC_LONG_PACKET_HEADER_0);
DUMP_REG(DSI_VC_LONG_PACKET_PAYLOAD_0);
DUMP_REG(DSI_VC_SHORT_PACKET_HEADER_0);
DUMP_REG(DSI_VC_IRQSTATUS_0);
DUMP_REG(DSI_VC_IRQENABLE_0);

DUMP_REG(DSI_VC_CTRL_1);
DUMP_REG(DSI_VC_IRQSTATUS_1);
DUMP_REG(DSI_VC_IRQENABLE_1);
DUMP_REG(DSI_VC_CTRL_2);
DUMP_REG(DSI_VC_IRQSTATUS_2);
DUMP_REG(DSI_VC_IRQENABLE_2);
DUMP_REG(DSI_VC_CTRL_3);
DUMP_REG(DSI_VC_IRQSTATUS_3);
DUMP_REG(DSI_VC_IRQENABLE_3);

DUMP_REG(DSI_DSIPHY_CFG0);
DUMP_REG(DSI_DSIPHY_CFG1);
DUMP_REG(DSI_DSIPHY_CFG2);
DUMP_REG(DSI_DSIPHY_CFG5);

DUMP_REG( DSI_PLL_GO);
DUMP_REG( DSI_PLL_STATUS);
DUMP_REG( DSI_PLL_CONTROL);
DUMP_REG( DSI_PLL_CONFIGURATION1);
DUMP_REG( DSI_PLL_CONFIGURATION2);

DUMP_REG(DISPC_SYSCONFIG);
DUMP_REG(DISPC_SYSSTATUS);
DUMP_REG(DISPC_IRQSTATUS);
DUMP_REG(DISPC_IRQENABLE);
DUMP_REG(DISPC_CONTROL1);
DUMP_REG(DISPC_DIVISOR1);
DUMP_REG(DISPC_TIMING_H1);
DUMP_REG(DISPC_TIMING_V1);
DUMP_REG(DISPC_POL_FREQ1);
DUMP_REG(DISPC_SIZE_LCD1);
DUMP_REG(DISPC_GLOBAL_ALPHA);
DUMP_REG(DISPC_GFX_ROW_INC);
DUMP_REG(DISPC_GFX_PIXEL_INC);
DUMP_REG(DISPC_CONFIG1);
DUMP_REG(DISPC_GFX_ATTRIBUTES);
DUMP_REG(DISPC_GFX_BUF_THRESHOLD);
DUMP_REG(DISPC_SYSSTATUS);	
DUMP_REG(DISPC_GFX_BA_0);	
DUMP_REG(DISPC_GFX_BA_1);
DUMP_REG(DISPC_GFX_PRELOAD);
DUMP_REG(DISPC_GFX_POSITION);
DUMP_REG(DISPC_GFX_SIZE );

DUMP_REG(DSS_CTRL);
DUMP_REG(DSS_STATUS);

DUMP_REG(CONTROL_DSIPHY);
}
#endif

static inline int wait_for_bit_change(u32 reg, int bitnum, int value)
{
	int t = 100000;

	while (REG_GET(reg, bitnum, bitnum) != value) {
		if (--t == 0)
			return !value;
	}

	return value;
}

static void dsi_reset(void)
{
	u32 t = 0;
	REG_FLD_MOD(DSI_SYSCONFIG, 1, 1, 1);
	
	while (REG_GET(DSI_SYSSTATUS, 0, 0) == 0) {
		if (++t > 5) {
			return;
		}

		udelay(1);
	}
}

void dss_init(void)
{
	u32 r;

	r = __raw_readl(CONTROL_DSIPHY);
	r = FLD_MOD(r, 0x1f, 28, 24);
	r = FLD_MOD(r, 0x1f, 23, 19);
	__raw_writel(r, CONTROL_DSIPHY); 	

	r = __raw_readl(CM_DSS_DSS_CLKCTRL);
	r = FLD_MOD(r, 2, 1, 0);
	r = FLD_MOD(r, 1, 8, 8);
	r = FLD_MOD(r, 1, 10, 10);
	__raw_writel(r, CM_DSS_DSS_CLKCTRL);

	REG_FLD_MOD(DSS_CTRL, 0, 0, 0);
}

void dispc_init(void)
{
	u32 r;

	REG_FLD_MOD(DISPC_SYSCONFIG, 1, 1, 1);

	if (wait_for_bit_change(DISPC_SYSSTATUS, 0, 1) != 1) {
		return;
	}	

	__raw_writel(0, DISPC_IRQENABLE);
	__raw_writel(0xffffffff, DISPC_IRQSTATUS);

	r = __raw_readl(DISPC_SYSCONFIG);
	r = FLD_MOD(r, 1, 0, 0);
	r = FLD_MOD(r, 1, 2, 2);
	r = FLD_MOD(r, 2, 4, 3);
	r = FLD_MOD(r, 2, 13, 12);
	__raw_writel(r, DISPC_SYSCONFIG);

	r = __raw_readl(DISPC_DIVISOR1);
	r = FLD_MOD(r, 1, 7, 0);
	r = FLD_MOD(r, 1, 23, 16);
	__raw_writel(r, DISPC_DIVISOR1);

	REG_FLD_MOD(DISPC_CONFIG1, 1, 17, 17);
	REG_FLD_MOD(DISPC_CONFIG1, 2, 2, 1);
}

void dsi_init(void)
{
	REG_FLD_MOD(DSI_CLK_CTRL, 1, 14, 14);
}

// hard coded for now - perhaps not the best approach...
#define SYS_CLK_FREQ 	(38400000)

static inline unsigned ns2ddr(unsigned long ddr_clk, unsigned ns)
{
	return (ns * (ddr_clk / 1000 / 1000) + 999) / 1000;
}

void dsi_if_enable(u8 onoff)
{
	REG_FLD_MOD(DSI_CTRL, onoff, 0, 0);
	
	if (wait_for_bit_change(DSI_CTRL, 0, onoff) != onoff) {
		printf("if failed to enable/disable\n");
	}	
}

void dsi_vc_enable(int vc, u8 onoff)
{
	u32 reg;

	reg = DSI_VC_CTRL_0 + (vc * 0x20);

	REG_FLD_MOD(reg, onoff, 0, 0);	

	if (wait_for_bit_change(reg, 0, onoff) != onoff) {
		printf("vc failed to enable/disable\n");
	}
}

static void dispc_init_panel(struct omap_dsi_panel *panel)
{
	u32 r;

	r = __raw_readl(DISPC_CONTROL1);
	r = FLD_MOD(r, 1, 3, 3);

	if (panel->dsi_data.pxl_fmt == OMAP_PXL_FMT_RGB888) {
		r = FLD_MOD(r, 3, 9, 8);
	} else {
		r = FLD_MOD(r, 2, 9, 8);
		r = FLD_MOD(r, 1, 7, 7);
	}

	r = FLD_MOD(r, 0, 11, 11);
	r = FLD_MOD(r, 1, 15, 15);
	r = FLD_MOD(r, 1, 16, 16);
	__raw_writel(r, DISPC_CONTROL1);

	REG_FLD_MOD(DISPC_CONFIG1, 0, 16, 16);	

	r = FLD_VAL(panel->dispc_data.hsw - 1, 7, 0) |
		FLD_VAL(panel->dispc_data.hfp - 1, 19, 8) |
		FLD_VAL(panel->dispc_data.hbp - 1, 31, 20);
	__raw_writel(r, DISPC_TIMING_H1);

	r = FLD_VAL(panel->dispc_data.vsw - 1, 7, 0) |
		FLD_VAL(panel->dispc_data.vfp, 19, 8) |
 		FLD_VAL(panel->dispc_data.vbp, 31, 20);
	__raw_writel(r, DISPC_TIMING_V1);

	r = FLD_VAL(panel->xres - 1, 10, 0) |
		FLD_VAL(panel->yres - 1, 26, 16);
	__raw_writel(r, DISPC_SIZE_LCD1);
}

static void dsi_pll_init(void)
{
	REG_FLD_MOD(DSI_CLK_CTRL, 1, 14, 14);
	
	if (wait_for_bit_change(DSI_PLL_STATUS, 0, 1) != 1) {
		printf("pll init failed\n");
		return;
	}

	REG_FLD_MOD(DSI_CLK_CTRL, 2, 31, 30);
	udelay(1000);

	if (REG_GET(DSI_CLK_CTRL, 29, 28) != 2) {
		printf("pll init failed\n");
		return;
	}
}

static void dsi_pll_set_clock_div(struct omap_dsi_panel *panel)
{
	u32 r;

	REG_FLD_MOD(DSI_PLL_CONTROL, 0, 0, 0);
	REG_FLD_MOD(DSI_PLL_CONTROL, 1, 1, 1);
	REG_FLD_MOD(DSI_PLL_CONTROL, 1, 2, 2);

	r = __raw_readl(DSI_PLL_CONFIGURATION1);
	r = FLD_MOD(r, 1, 0, 0);
	r = FLD_MOD(r, panel->dsi_data.regn - 1, 8, 1);
	r = FLD_MOD(r, panel->dsi_data.regm, 20, 9);
	r = FLD_MOD(r, panel->dsi_data.regm_dispc - 1, 25, 21);
	r = FLD_MOD(r, panel->dsi_data.regm_dsi - 1, 30, 26);
	__raw_writel(r, DSI_PLL_CONFIGURATION1);

	r = __raw_readl(DSI_PLL_CONFIGURATION2);
	r = FLD_MOD(r, 1, 13, 13);
	r = FLD_MOD(r, 0, 14, 14);
	r = FLD_MOD(r, 1, 20, 20);
	r = FLD_MOD(r, 3, 22, 21);
	__raw_writel(r, DSI_PLL_CONFIGURATION2);

	REG_FLD_MOD(DSI_PLL_GO, 1, 0, 0);
	
	if (wait_for_bit_change(DSI_PLL_GO, 0, 0) != 0) {
		return;
	}

	if (wait_for_bit_change(DSI_PLL_STATUS, 1, 1) != 1) {
		return;
	}

	if (wait_for_bit_change(DSI_IRQSTATUS, 7, 1) != 1) {
		return;
	}

	REG_FLD_MOD(DSI_IRQSTATUS, 1, 7, 7);

	r = __raw_readl(DSI_PLL_CONFIGURATION2);
	r = FLD_MOD(r, 0, 0, 0);	/* DSI_PLL_IDLE */
	r = FLD_MOD(r, 0, 5, 5);	/* DSI_PLL_PLLLPMODE */
	r = FLD_MOD(r, 0, 6, 6);	/* DSI_PLL_LOWCURRSTBY */
	r = FLD_MOD(r, 0, 7, 7);	/* DSI_PLL_TIGHTPHASELOCK */
	r = FLD_MOD(r, 0, 8, 8);	/* DSI_PLL_DRIFTGUARDEN */
	r = FLD_MOD(r, 0, 10, 9);	/* DSI_PLL_LOCKSEL */
	r = FLD_MOD(r, 1, 13, 13);	/* DSI_PLL_REFEN */
	r = FLD_MOD(r, 1, 14, 14);	/* DSIPHY_CLKINEN */
	r = FLD_MOD(r, 0, 15, 15);	/* DSI_BYPASSEN */
	r = FLD_MOD(r, 1, 16, 16);	/* DSS_CLOCK_EN */
	r = FLD_MOD(r, 0, 17, 17);	/* DSS_CLOCK_PWDN */
	r = FLD_MOD(r, 1, 18, 18);	/* DSI_PROTO_CLOCK_EN */
	r = FLD_MOD(r, 0, 19, 19);	/* DSI_PROTO_CLOCK_PWDN */
	r = FLD_MOD(r, 0, 20, 20);	/* DSI_HSDIVBYPASS */
	__raw_writel(r, DSI_PLL_CONFIGURATION2);
}

static void dispc_set_clock_div(struct omap_dsi_panel *panel)
{
	u32 r;

	r = FLD_VAL(1, 14, 14) |
		FLD_VAL(panel->dispc_data.acbi, 11, 8) |
		FLD_VAL(panel->dispc_data.acb, 7, 0);
	__raw_writel(r, DISPC_POL_FREQ1);
	
	r = FLD_VAL(panel->dispc_data.lcd, 23, 16) | 
		FLD_VAL(panel->dispc_data.pcd, 7, 0);
	__raw_writel(r, DISPC_DIVISOR1);
}

static void dsi_cio_timings(struct omap_dsi_panel *panel)
{
	u32 r;

	/* program timings */
	r = __raw_readl(DSI_DSIPHY_CFG0);
	r = FLD_MOD(r, panel->dsi_data.ths_prepare, 31, 24);
	r = FLD_MOD(r, panel->dsi_data.ths_prepare + panel->dsi_data.ths_zero, 23, 16);
	r = FLD_MOD(r, panel->dsi_data.ths_trail, 15, 8);
	r = FLD_MOD(r, panel->dsi_data.ths_exit, 7, 0);
	__raw_writel(r, DSI_DSIPHY_CFG0);

	r = __raw_readl(DSI_DSIPHY_CFG1);
	r = FLD_MOD(r, panel->dsi_data.tlpx / 2, 22, 16);
	r = FLD_MOD(r, panel->dsi_data.tclk_trail, 15, 8);
	r = FLD_MOD(r, panel->dsi_data.tclk_zero, 7, 0);
	__raw_writel(r, DSI_DSIPHY_CFG1);

	r = __raw_readl(DSI_DSIPHY_CFG2);
	r = FLD_MOD(r, panel->dsi_data.tclk_prepare, 7, 0);
	__raw_writel(r, DSI_DSIPHY_CFG2);
}

static void dsi_cio_init(struct omap_dsi_panel *panel)
{
	u32 r;
	REG_FLD_MOD(DSI_CLK_CTRL, 0, 13, 13);
	REG_FLD_MOD(DSI_CLK_CTRL, 1, 18, 18);

	r = __raw_readl(DSI_DSIPHY_CFG5);

	if (wait_for_bit_change(DSI_DSIPHY_CFG5, 30, 1) != 1) {
		return;
	}
	
	// hard code lane config for now
	r = __raw_readl(DSI_COMPLEXIO_CFG1);
	r = FLD_MOD(r, 1, 2, 0);
	r = FLD_MOD(r, 2, 6, 4);
	r = FLD_MOD(r, 3, 10, 8);
	r = FLD_MOD(r, 4, 14, 12);
	r = FLD_MOD(r, 5, 18, 16);
	__raw_writel(r, DSI_COMPLEXIO_CFG1);	

	r = __raw_readl(DSI_TIMING1);	
	r = FLD_MOD(r, 1, 15, 15);	/* FORCE_TX_STOP_MODE_IO */
	r = FLD_MOD(r, 1, 14, 14);	/* STOP_STATE_X16_IO */
	r = FLD_MOD(r, 1, 13, 13);	/* STOP_STATE_X4_IO */
	r = FLD_MOD(r, 0x1fff, 12, 0);	/* STOP_STATE_COUNTER_IO */
	__raw_writel(r, DSI_TIMING1);

	REG_FLD_MOD(DSI_COMPLEXIO_CFG1, 1, 28, 27);
	REG_FLD_MOD(DSI_COMPLEXIO_CFG1, 1, 30, 30);
	
	udelay(1000);
	
	if (REG_GET(DSI_COMPLEXIO_CFG1, 26, 25) != 1) {
		return;
	}

	if (wait_for_bit_change(DSI_COMPLEXIO_CFG1, 29, 1) != 1) {
		return;
	}

	dsi_if_enable(1);
	dsi_if_enable(0);
	REG_FLD_MOD(DSI_CLK_CTRL, 1, 20, 20);

	r = 0;
	while (REG_GET(DSI_DSIPHY_CFG5, 26, 24) != 0x7) {
		if (++r > 100000) {
			printf("%08x\n", __raw_readl(DSI_DSIPHY_CFG5));
			return;
		}
	}

	REG_FLD_MOD(DSI_TIMING1, 0, 15, 15);
	
	dsi_cio_timings(panel);
}

static void dsi_proto_timings(struct omap_dsi_panel *panel)
{
	u32 tlpx, tclk_zero, tclk_prepare, tclk_trail;
	u32 tclk_pre, tclk_post;
	u32 ths_prepare, ths_prepare_ths_zero, ths_zero;
	u32 ths_trail, ths_exit;
	u32 ddr_clk_pre, ddr_clk_post;
	u32 enter_hs_mode_lat, exit_hs_mode_lat;
	u32 ths_eot;
	u32 r;

	unsigned long ddr_clk;

	ddr_clk = (panel->dsi_data.regm * (SYS_CLK_FREQ / panel->dsi_data.regn)) / 2;

	r = __raw_readl(DSI_DSIPHY_CFG0);
	ths_prepare = FLD_GET(r, 31, 24);
	ths_prepare_ths_zero = FLD_GET(r, 23, 16);
	ths_zero = ths_prepare_ths_zero - ths_prepare;
	ths_trail = FLD_GET(r, 15, 8);
	ths_exit = FLD_GET(r, 7, 0);

	r = __raw_readl(DSI_DSIPHY_CFG1);
	tlpx = FLD_GET(r, 22, 16) * 2;
	tclk_trail = FLD_GET(r, 15, 8);
	tclk_zero = FLD_GET(r, 7, 0);

	r = __raw_readl(DSI_DSIPHY_CFG2);
	tclk_prepare = FLD_GET(r, 7, 0);

	/* min 8*UI */
	tclk_pre = 20;
	/* min 60ns + 52*UI */
	tclk_post = ns2ddr(ddr_clk, 60) + 26;

	ths_eot = DIV_ROUND_UP(4, 4);

	ddr_clk_pre = DIV_ROUND_UP(tclk_pre + tlpx + tclk_zero + tclk_prepare,
			4);
	ddr_clk_post = DIV_ROUND_UP(tclk_post + ths_trail, 4) + ths_eot;

	r = __raw_readl(DSI_CLK_TIMING);
	r = FLD_MOD(r, ddr_clk_pre, 15, 8);
	r = FLD_MOD(r, ddr_clk_post, 7, 0);
	__raw_writel(r, DSI_CLK_TIMING);

	enter_hs_mode_lat = 1 + DIV_ROUND_UP(tlpx, 4) +
		DIV_ROUND_UP(ths_prepare, 4) +
		DIV_ROUND_UP(ths_zero + 3, 4);

	exit_hs_mode_lat = DIV_ROUND_UP(ths_trail + ths_exit, 4) + 1 + ths_eot;

	r = FLD_VAL(enter_hs_mode_lat, 31, 16) |
		FLD_VAL(exit_hs_mode_lat, 15, 0);
	__raw_writel(r, DSI_VM_TIMING7);
}

static void dsi_vc_init(int vc)
{
	u32 r;

	r = __raw_readl(DSI_VC_CTRL_0 + (vc * 0x20));
	r = FLD_MOD(r, 0, 1, 1); /* SOURCE, 0 = L4 */
	r = FLD_MOD(r, 0, 2, 2); /* BTA_SHORT_EN  */
	r = FLD_MOD(r, 0, 3, 3); /* BTA_LONG_EN */
	r = FLD_MOD(r, 0, 4, 4); /* MODE, 0 = command */
	r = FLD_MOD(r, 1, 7, 7); /* CS_TX_EN */
	r = FLD_MOD(r, 1, 8, 8); /* ECC_TX_EN */
	r = FLD_MOD(r, 0, 9, 9); /* MODE_SPEED, high speed on/off */

	if (vc == 0) {
		r = FLD_MOD(r, 1, 11, 10); /* OCP_WIDTH = 24 bit */
	} else {
		r = FLD_MOD(r, 3, 11, 10);
	}

	r = FLD_MOD(r, 4, 29, 27); /* DMA_RX_REQ_NB = no dma */
	r = FLD_MOD(r, 4, 23, 21); /* DMA_TX_REQ_NB = no dma */
	__raw_writel(r, DSI_VC_CTRL_0 + (vc * 0x20));
}

static void dsi_video_proto_timings(struct omap_dsi_panel *panel)
{
	u32 r;

	r = FLD_VAL(0, 2, 0) | FLD_VAL(1, 7, 4) |
		FLD_VAL(1, 10, 8) | FLD_VAL(1, 15, 12) |
		FLD_VAL(2, 18, 16) | FLD_VAL(1, 23, 20) |
		FLD_VAL(3, 26, 24) | FLD_VAL(1, 31, 28);
	__raw_writel(r, DSI_TX_FIFO_VC_SIZE);
	__raw_writel(r, DSI_RX_FIFO_VC_SIZE);

	r = __raw_readl(DSI_TIMING1);
	r = FLD_MOD(r, 0, 15, 15);	/* FORCE_TX_STOP_MODE_IO */
	r = FLD_MOD(r, 1, 14, 14);	/* STOP_STATE_X16_IO */
	r = FLD_MOD(r, 1, 13, 13);	/* STOP_STATE_X4_IO */
	r = FLD_MOD(r, 0x1fff, 12, 0);	/* STOP_STATE_COUNTER_IO */
	__raw_writel(r, DSI_TIMING1);

	r = __raw_readl(DSI_TIMING1);
	r = FLD_MOD(r, 0, 31, 31);	/* TA_TO */
	r = FLD_MOD(r, 1, 30, 30);	/* TA_TO_X16 */
	r = FLD_MOD(r, 1, 29, 29);	/* TA_TO_X8 */
	r = FLD_MOD(r, 0x1fff, 28, 16);	/* TA_TO_COUNTER */
	__raw_writel(r, DSI_TIMING1);

	r = __raw_readl(DSI_TIMING2);
	r = FLD_MOD(r, 0, 15, 15);	/* LP_RX_TO */
	r = FLD_MOD(r, 1, 14, 14);	/* LP_RX_TO_X16 */
	r = FLD_MOD(r, 1, 13, 13);	/* LP_RX_TO_X4 */
	r = FLD_MOD(r, 0x1fff, 12, 0);	/* LP_RX_COUNTER */
	__raw_writel(r, DSI_TIMING2);

	__raw_readl(DSI_TIMING2);
	r = FLD_MOD(r, 1, 31, 31);	/* HS_TX_TO */
	r = FLD_MOD(r, 1, 30, 30);	/* HS_TX_TO_X16 */
	r = FLD_MOD(r, 1, 29, 29);	/* HS_TX_TO_X8 (4 really) */
	r = FLD_MOD(r, 0x1fff, 28, 16);	/* HS_TX_TO_COUNTER */
	__raw_writel(r, DSI_TIMING2);

	r = __raw_readl(DSI_CTRL);
	r = FLD_MOD(r, 1, 3, 3);	/* TX_FIFO_ARBITRATION */
	r = FLD_MOD(r, 1, 4, 4);	/* VP_CLK_RATIO, always 1, see errata*/
	r = FLD_MOD(r, panel->dsi_data.bus_width, 7, 6);		/* VP_DATA_BUS_WIDTH */
	r = FLD_MOD(r, 0, 8, 8);	/* VP_CLK_POL */
	r = FLD_MOD(r, 1, 9, 9);	/* VP_DE_POL */
	r = FLD_MOD(r, 0, 10, 10);	/* VP_HSYNC_POL */
	r = FLD_MOD(r, 1, 11, 11);	/* VP_VSYNC_POL */
	r = FLD_MOD(r, panel->dsi_data.line_bufs, 13, 12);	/* LINE_BUFFER */
	r = FLD_MOD(r, 1, 14, 14);	/* TRIGGER_RESET_MODE */
	r = FLD_MOD(r, 1, 15, 15);	/* VP_VSYNC_START */
	r = FLD_MOD(r, 1, 17, 17);	/* VP_HSYNC_START */
	r = FLD_MOD(r, 1, 19, 19);	/* EOT_ENABLE */
	r = FLD_MOD(r, 1, 21, 21);	/* HFP_BLANKING */
	r = FLD_MOD(r, 1, 22, 22);	/* HBP_BLANKING */
	r = FLD_MOD(r, 1, 23, 23);	/* HSA_BLANKING */
	__raw_writel(r, DSI_CTRL);
	
	dsi_vc_init(0);
	dsi_vc_init(1);
	dsi_vc_init(2);
	dsi_vc_init(3);

	r = __raw_readl(DSI_VM_TIMING1);
	r = FLD_MOD(r, panel->dsi_data.hbp, 11, 0);   /* HBP */
	r = FLD_MOD(r, panel->dsi_data.hfp, 23, 12);  /* HFP */
	r = FLD_MOD(r, panel->dsi_data.hsa, 31, 24);  /* HSA */
	__raw_writel(r, DSI_VM_TIMING1);

	r = __raw_readl(DSI_VM_TIMING2);
	r = FLD_MOD(r, panel->dsi_data.vbp, 7, 0);    /* VBP */
	r = FLD_MOD(r, panel->dsi_data.vfp, 15, 8);   /* VFP */
	r = FLD_MOD(r, panel->dsi_data.vsa, 23, 16);  /* VSA */
	r = FLD_MOD(r, 4, 27, 24);	/* WINDOW_SYNC */
	__raw_writel(r, DSI_VM_TIMING2);

	r = __raw_readl(DSI_VM_TIMING3);
	r = FLD_MOD(r, panel->dsi_data.vact, 14, 0);
	r = FLD_MOD(r, panel->dsi_data.tl, 31, 16);
	__raw_writel(r, DSI_VM_TIMING3);

	r = FLD_VAL(panel->dsi_data.hsa_hs_int, 23, 16) |      /* HSA_HS_INTERLEAVING */
		FLD_VAL(panel->dsi_data.hfp_hs_int, 15, 8) |   /* HFP_HS_INTERLEAVING */
		FLD_VAL(panel->dsi_data.hbp_hs_int, 7, 0);     /* HBP_HS_INTERLEAVING */
	__raw_writel(r, DSI_VM_TIMING4);

	r = FLD_VAL(panel->dsi_data.hsa_lp_int, 23, 16) |	/* HSA_LP_INTERLEAVING */
		FLD_VAL(panel->dsi_data.hfp_lp_int, 15, 8) |	/* HFP_LP_INTERLEAVING */
		FLD_VAL(panel->dsi_data.hbp_lp_int, 7, 0);	/* HBP_LP_INTERLEAVING */
	__raw_writel(r, DSI_VM_TIMING5);

	r = FLD_VAL(panel->dsi_data.bl_hs_int, 31, 16) |	/* BL_HS_INTERLEAVING */
		FLD_VAL(panel->dsi_data.bl_lp_int, 15, 0);	/* BL_LP_INTERLEAVING */
	__raw_writel(r, DSI_VM_TIMING6);

	r = FLD_VAL(panel->dsi_data.enter_lat, 31, 16) |	/* ENTER_HS_MODE_LATENCY */
		FLD_VAL(panel->dsi_data.exit_lat, 15, 0);	/* EXIT_HS_MODE_LATENCY */
	__raw_writel(r, DSI_VM_TIMING7);
}

static void dsi_force_tx_stop_mode_io(void)
{
	REG_FLD_MOD(DSI_TIMING1, 1, 15, 15);
	
	if (wait_for_bit_change(DSI_TIMING1, 15, 0) != 0) {
		printf("force stop failed\n");
	}
}

static void dsi_init_panel(struct omap_dsi_panel *panel)
{
	dsi_pll_init();
	dsi_pll_set_clock_div(panel);

	if (wait_for_bit_change(DSI_PLL_STATUS, 7, 1) != 1) {
		return;
	}

	if (wait_for_bit_change(DSI_PLL_STATUS, 8, 1) != 1) {
		return;
	}

	REG_FLD_MOD(DSS_CTRL, 1, 1, 1);
	REG_FLD_MOD(DSS_CTRL, 1, 0, 0);

	dispc_set_clock_div(panel);

	dsi_cio_init(panel);
	udelay(1000);
	dsi_proto_timings(panel);
	
	REG_FLD_MOD(DSI_CLK_CTRL, panel->dsi_data.lp_div, 12, 0);
	REG_FLD_MOD(DSI_CLK_CTRL, 1, 21, 21); // assume dsi fck is > 30kHz

	dsi_video_proto_timings(panel);
	
	dsi_vc_enable(0, 1);
	dsi_vc_enable(1, 1);
	dsi_vc_enable(2, 1);
	dsi_vc_enable(3, 1);
	
	dsi_if_enable(1);
	dsi_force_tx_stop_mode_io();
}

void dsi_enable_panel(struct omap_dsi_panel *panel)
{
	dsi_reset();

	// ENWAKEUP
	REG_FLD_MOD(DSI_SYSCONFIG, 1, 2, 2);
	REG_FLD_MOD(DSI_SYSCONFIG, 2, 4, 3);

	__raw_writel(0, DSI_IRQENABLE);
	__raw_writel(0, DSI_COMPLEXIO_IRQENABLE);
	__raw_writel(0, DSI_VC_IRQENABLE_0);
	__raw_writel(0, DSI_VC_IRQENABLE_1);
	__raw_writel(0, DSI_VC_IRQENABLE_2);
	__raw_writel(0, DSI_VC_IRQENABLE_3);

	// clear IRQs
	__raw_writel(0xffffffff, DSI_IRQSTATUS);
	__raw_writel(0xffffffff, DSI_COMPLEXIO_IRQSTATUS);
	__raw_writel(0xffffffff, DSI_VC_IRQSTATUS_0);	
	__raw_writel(0xffffffff, DSI_VC_IRQSTATUS_1);	
	__raw_writel(0xffffffff, DSI_VC_IRQSTATUS_2);	
	__raw_writel(0xffffffff, DSI_VC_IRQSTATUS_3);	

	REG_FLD_MOD(DSI_CLK_CTRL, 1, 14, 14);

	dispc_init_panel(panel);
	dsi_init_panel(panel);
}

void dsi_enable_hs(void)
{
	dsi_vc_enable(0, 0);
	dsi_if_enable(0);

	REG_FLD_MOD(DSI_VC_CTRL_0, 1, 9, 9);
	
	dsi_vc_enable(0, 1);
	dsi_if_enable(1);

	dsi_force_tx_stop_mode_io();
}

void dsi_send_null(void)
{
	__raw_writel(0, DSI_VC_SHORT_PACKET_HEADER_0);
	udelay(1000);
}

void dsi_enable_videomode(struct omap_dsi_panel *panel)
{
	u32 r;
	u16 word_count;
	u32 header;
	u8 data_type;
	u8 bits_per_pixel;

	dsi_if_enable(0);
	dsi_vc_enable(1, 0);
	dsi_vc_enable(0, 0);

	if (wait_for_bit_change(DSI_PLL_STATUS, 15, 0) != 0) {
		return;
	}

	r = __raw_readl(DSI_VC_CTRL_0);
	r = FLD_MOD(r, 1, 4, 4);
	r = FLD_MOD(r, 1, 9, 9);
	__raw_writel(r, DSI_VC_CTRL_0);
	__raw_writel(0x20800790, DSI_VC_CTRL_0);
	
	switch (panel->dsi_data.pxl_fmt) {
	case OMAP_PXL_FMT_RGB666_PACKED:
		data_type = 0x1e;
		bits_per_pixel = 18;
		break;
	case OMAP_PXL_FMT_RGB888:
	default:
		data_type = 0x3e;
		bits_per_pixel = 24;
		break;
	}

	word_count = (panel->xres * bits_per_pixel) / 8;

	header = FLD_VAL(0, 31, 24) |
		FLD_VAL(word_count, 23, 8) |
		FLD_VAL(0, 7, 6) |
		FLD_VAL(data_type, 5, 0);
	__raw_writel(header, DSI_VC_LONG_PACKET_HEADER_0);

	dsi_vc_enable(0, 1);
	dsi_if_enable(1);

	udelay(2000);
	REG_FLD_MOD(DSI_TIMING1, 0, 15, 15);
	udelay(2000);
}

void dispc_config(struct omap_dsi_panel *panel, void *framebuffer, enum omap_dispc_format fmt)
{
	u32 r;

	__raw_writel((u32) framebuffer, DISPC_GFX_BA_0);
	__raw_writel((u32) framebuffer, DISPC_GFX_BA_1);

	r = __raw_readl(DISPC_GFX_ATTRIBUTES);
	r = FLD_MOD(r, fmt, 4, 1);
	r = FLD_MOD(r, 1, 5, 5);
	r = FLD_MOD(r, 2, 7, 6);
	r = FLD_MOD(r, 1, 14, 14);
	r = FLD_MOD(r, 1, 25, 25);
	r = FLD_MOD(r, 3, 27, 26);
	__raw_writel(r, DISPC_GFX_ATTRIBUTES);

	__raw_writel(1, DISPC_GFX_ROW_INC);
	__raw_writel(1, DISPC_GFX_PIXEL_INC);
	
	r = FLD_VAL(0x8ff, 31, 16) | FLD_VAL(0x3eb, 15, 0);
	__raw_writel(r, DISPC_GFX_BUF_THRESHOLD);
	__raw_writel(0x8ff, DISPC_GFX_PRELOAD);

	r = FLD_VAL(0, 10, 0) |
	    FLD_VAL(0, 26, 16);
	__raw_writel(r , DISPC_GFX_POSITION);

	r = FLD_VAL(panel->xres - 1 , 10, 0) |
		FLD_VAL(panel->yres - 1 , 26, 16);
	__raw_writel(r, DISPC_GFX_SIZE);

	__raw_writel(0, DISPC_DEFAULT_COLOR0);
}

void dispc_go(void)
{
	u32 r;

	r = __raw_readl(DISPC_CONTROL1);
	r = FLD_MOD(r, 0, 1, 1);
	r = FLD_MOD(r, 1, 0, 0);
	__raw_writel(r, DISPC_CONTROL1);

	REG_FLD_MOD(DISPC_CONTROL1, 1, 5, 5);
	REG_FLD_MOD(DISPC_GFX_ATTRIBUTES, 1, 0, 0);
}

static void dsi_send_short(u8 data_type, u16 data, u8 ecc)
{
	u32 r;

	r = (data_type << 0) | (data << 8) | (ecc << 24);

	__raw_writel(r, DSI_VC_SHORT_PACKET_HEADER_0);	
}

void dsi_gen_short_write(u8 *data, size_t len)
{
	u16 buf = 0;
	
	if (len > 0) {
		buf = data[0];
	} 

	if (len > 1) {
		buf |= (data[1] << 8);
	}

	dsi_send_short(DSI_DT_GENERIC_SHORT_WRITE | (len << 4), buf, 0);

}

void dsi_turn_on_peripheral(void)
{
	dsi_send_short(DSI_DT_TURN_ON_PERIPHERAL, 
		(DSI_DT_TURN_ON_PERIPHERAL << 8) | DSI_DT_TURN_ON_PERIPHERAL, 0);
}

