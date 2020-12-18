#include <stdio.h>
#include <sqlite3.h>
#include "bme280.h"

sqlite3 *db_init(char *dbname);

int8_t db_update_bme280_data (sqlite3 *db, struct bme280_data *bme280);

int bme280_opendev(const char *iic_dev, struct bme280_dev *dev);

void print_sensor_data(struct bme280_data *comp_data);
