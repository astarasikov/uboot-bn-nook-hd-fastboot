#ifndef LP855X_H
#define LP855X_H

void lp855x_on(u8 cfg);
void lp855x_set_brightness(u8 brightness);
void lp855x_restore_i2c(void);

#endif /* LP855X_H */
