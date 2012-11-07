#include <asm/types.h>
#include <common.h>
#include <i2c.h>
#include "lp855x.h"

#define LP8556_I2C_ADDR		(0x2C)
#define BRIGHTNESS_CTRL		(0x00)
#define	DEVICE_CTRL		(0x01)
#define CFG5_REG		(0xA5)

extern int select_bus(int, int);

static inline void lp855x_i2c_write(u8 reg, u8 val)
{
	i2c_write(LP8556_I2C_ADDR, reg, sizeof(reg), &val, sizeof(val));
}

static inline u8 lp855x_i2c_read(u8 reg)
{
	u8 val = 0;
	
	i2c_read(LP8556_I2C_ADDR, reg, sizeof(reg), &val, sizeof(val));
	return val;
}

void lp855x_on(u8 cfg)
{
	int r;

	if ((r = select_bus(2, 100)) != 0) {
		printf("select_bus failed: %d\n", r);
		goto out;
	}

	i2c_init(100, LP8556_I2C_ADDR);

	lp855x_i2c_write(BRIGHTNESS_CTRL, 0);
	lp855x_i2c_write(DEVICE_CTRL, 1 | (2 << 1));
	lp855x_i2c_write(CFG5_REG, cfg);

out:
	return r;
}

void lp855x_set_brightness(u8 brightness)
{
	int r;

	if ((r = select_bus(2, 100)) != 0) {
		printf("select_bus failed: %d\n", r);
	}

	i2c_init(100, LP8556_I2C_ADDR);

	lp855x_i2c_write(BRIGHTNESS_CTRL, brightness);
}

void lp855x_restore_i2c(void)
{
	int r;

	if ((r = select_bus(0, CFG_I2C_SPEED)) != 0) {
		printf("select_bus failed: %d\n", r);
	}

	i2c_init(CFG_I2C_SPEED, CFG_I2C_SLAVE);
}

