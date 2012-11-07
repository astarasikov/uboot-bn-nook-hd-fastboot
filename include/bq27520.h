#ifndef _BQ27520_H_
#define _BQ27520_H_

#define ENOBAT			125	/* No battery attached error */

int bq27520_init(int retries);

int bq27520_get_control_control_status(u16 *val);
int bq27520_get_control_device_type(u16 *val);

int bq27520_get_temperature(u16 *val);
int bq27520_get_voltage(u16 *val);
int bq27520_get_soc(u16 *val);
int bq27520_get_flags(u16 *val);
int bq27520_battery_detect(u8 *flag);

#endif /* _BQ27520_H_ */
