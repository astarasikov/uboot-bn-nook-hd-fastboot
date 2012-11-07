#ifndef OMAP4_DSI_H
#define OMAP4_DSI_H

#include <common.h>

enum omap_dsi_pxl_format {
	OMAP_PXL_FMT_RGB888,
	OMAP_PXL_FMT_RGB666_PACKED,
};

struct omap_dsi_data {
	enum omap_dsi_pxl_format pxl_fmt;
	
	// DSI specific settings
	u16 hsa;
	u16 hbp;
	u16 hfp;

	u16 vsa;
	u16 vbp;
	u16 vfp;

	u8 window_sync;

	u16 regm;
	u16 regn;
	u16 regm_dsi;
	u16 regm_dispc;
	u16 lp_div;
	u16 tl;
	u16 vact;
	u8 line_bufs;
	u8 bus_width;

	u8 hsa_hs_int;
	u8 hfp_hs_int;
	u8 hbp_hs_int;

	u16 enter_lat;
	u16 exit_lat;

	u8 hsa_lp_int;
	u8 hfp_lp_int;
	u8 hbp_lp_int;

	u16 bl_hs_int;
	u16 bl_lp_int;

	u16 ths_prepare;
	u16 ths_zero;
	u16 ths_trail;
	u16 ths_exit;
	u16 tlpx;
	u16 tclk_trail;
	u16 tclk_zero;
	u16 tclk_prepare;
};

struct omap_dispc_data {
	u16 hsw;
	u16 hbp;
	u16 hfp;
	
	u16 vsw;
	u16 vbp;
	u16 vfp;

	u8 lcd;
	u8 pcd;

	u8 acbi;
	u8 acb;

	u16 row_inc;
};

struct omap_dsi_panel {
	u16 xres;
	u16 yres;

	struct omap_dispc_data dispc_data;
	struct omap_dsi_data dsi_data;
};

enum omap_dispc_format {
	OMAP_RGB565_FMT = 0x6,
	OMAP_XRGB888_FMT = 0x8,	
};

void dss_init(void);
void dsi_init(void);
void dispc_init(void);
void dispc_config(struct omap_dsi_panel *panel, void *fb_addr, enum omap_dispc_format fmt);
void dsi_enable_panel(struct omap_dsi_panel *panel);
void dispc_go(void);
void dsi_turn_on_peripheral(void);
void dsi_enable_videomode(struct omap_dsi_panel *panel);
void dsi_send_null(void);
void dsi_enable_hs(void);
void dsi_dump_regs(int line);
void dsi_gen_short_write(u8 *data, size_t len);
void dsi_turn_on_peripheral(void);

#endif /* OMAP4_DSI_H */
