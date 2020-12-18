#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h> 
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <string.h>
#include "tools.h"

// #define DEBUG

char * make_sql_update_bme280(struct bme280_data *bme280);

sqlite3 *db_init(char *dbname) {

   sqlite3 *db;
   char *sql;
   char *err_msg = 0;
   int rc;

   if ((rc = sqlite3_open(dbname, &db)) != SQLITE_OK ) {
      fprintf(stderr, "db_init: Can't open database: \'%s\': %s\n", dbname, sqlite3_errmsg(db));

      sqlite3_close(db);
      return(NULL);
   }
#ifdef DEBUG
   fprintf(stderr, "db_init: Opened database successfully: \'%s\'\n", dbname);
#endif

   sql = "DROP TABLE IF EXISTS Env;" 
         "CREATE TABLE Env(id INT NOT NULL, temp REAL, pres REAL, rhum REAL, ts TIMESTAMP NOT NULL, \
                           switch INT, fan INT, state INT);" 
         "INSERT INTO  Env VALUES(1, 0.0, 0.0, 0.0, CURRENT_TIMESTAMP, NULL, NULL, NULL);";

   if ((rc = sqlite3_exec(db, sql, 0, 0, &err_msg)) != SQLITE_OK ) {
     fprintf(stderr, "db_init: Failed to create Env table: %s\n", err_msg);
        
     sqlite3_free(err_msg);        
     sqlite3_close(db);
     return(NULL);
   } 
#ifdef DEBUG
   fprintf(stderr, "db_init: Env table created successfully\n");
#endif

//   sqlite3_close(db);
   return(db);
}

int8_t db_update_bme280_data (sqlite3 *db, struct bme280_data *bme280) {

   char *sql;
   char *err_msg = 0;
   int rc;

   if ((sql = make_sql_update_bme280(bme280)) == NULL) {
//      sqlite3_close(db);
      return(1);
   }
#ifdef DEBUG
   fprintf(stderr, "db_update_bme280_data: SQL Update string: \'%s\'\n", sql);
#endif

   if ((rc = sqlite3_exec(db, sql, 0, 0, &err_msg)) != SQLITE_OK ) {
     fprintf(stderr, "db_update_bme280_data. Failed to update Env table: %s\n", err_msg);
     fprintf(stderr, "db_update_bme280_data. SQL = \'%s\'\n", sql);

     free(sql);
     sqlite3_free(err_msg);
 //    sqlite3_close(db);
     return(1);
   }

#ifdef DEBUG
   fprintf(stderr, "db_update_bme280_data: Updated database successfully\n");
#endif

   free(sql);
//   sqlite3_close(db);
   return(0);

}

/* {} */

char * make_sql_update_bme280(struct bme280_data *bme280) {

  int size = 0;
  char *p = NULL;

  const char *fmt = "UPDATE Env SET temp=%0.2f, pres=%0.2f, rhum=%0.2f, ts=CURRENT_TIMESTAMP  where id=1;";

           /* Determine required size */

  if ((size = snprintf(p, size, fmt, bme280->temperature, bme280->pressure/100, bme280->humidity)) < 0)
    return NULL;

               /* ++ For '\0' */
  if ((p = malloc(++size)) == NULL) {
    fprintf(stderr, "make_sql_update_bme280: malloc() error\n");
    return NULL;
  }

  if ((size = snprintf(p, size, fmt, bme280->temperature, bme280->pressure/100, bme280->humidity)) < 0) {
    free(p);
    return NULL;
  }
  return p;
}


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static void user_delay_ms(uint32_t period)
{
  usleep(period*1000);
}

static int8_t user_i2c_read(int fd, uint8_t reg_addr, uint8_t *data, uint16_t len)
{
  write(fd, &reg_addr,1);
  read(fd, data, len);
  return 0;
}

static int8_t user_i2c_write(int fd, uint8_t reg_addr, uint8_t *data, uint16_t len)
{
  int8_t *buf;
  buf = malloc(len +1);
  buf[0] = reg_addr;
  memcpy(buf +1, data, len);
  write(fd, buf, len +1);
  free(buf);
  return 0;
}

void print_sensor_data(struct bme280_data *comp_data)
{
        printf("temperature:%0.2f*C   pressure:%0.2fhPa   humidity:%0.2f%%\r\n",comp_data->temperature, comp_data->pressure/100, comp_data->humidity);
}


int bme280_opendev(const char *iic_dev, struct bme280_dev *dev) {

  int fd;
  int ret;
  int8_t rslt;
  uint8_t settings_sel;

  if ((fd = open(iic_dev, O_RDWR)) < 0) {
    fprintf(stderr, "bme280_init: Failed to open the i2c bus %s", iic_dev);
    return -1;
  }
  if ((ret = ioctl(fd, I2C_SLAVE, BME280_I2C_ADDR_PRIM)) < 0) {
    fprintf(stderr, "bme280_init: Failed to acquire bus access and/or talk to slave. ret=%d\n", ret);
    return -1;
  }
  dev->iic_fd = fd;
  dev->dev_id = BME280_I2C_ADDR_PRIM;//0x76
  //dev->dev_id = BME280_I2C_ADDR_SEC; //0x77
  dev->intf = BME280_I2C_INTF;
  dev->read = user_i2c_read;
  dev->write = user_i2c_write;
  dev->delay_ms = user_delay_ms;

  if ((rslt = bme280_init(dev)) != BME280_OK) {
    fprintf(stderr,"bme280_init: BME280 Init Failed: %d\n",rslt);
    dev->iic_fd = -1;
    close(fd);
    return -1;
  }
#ifdef DEBUG
  fprintf(stderr,"bme280_init: BME280 Init success\n");
#endif

  /* Recommended mode of operation: Indoor navigation */
  dev->settings.osr_h = BME280_OVERSAMPLING_1X;
  dev->settings.osr_p = BME280_OVERSAMPLING_16X;
  dev->settings.osr_t = BME280_OVERSAMPLING_2X;
  dev->settings.filter = BME280_FILTER_COEFF_16;
  dev->settings.standby_time = BME280_STANDBY_TIME_62_5_MS;
        
  settings_sel = BME280_OSR_PRESS_SEL;
  settings_sel |= BME280_OSR_TEMP_SEL;
  settings_sel |= BME280_OSR_HUM_SEL;
  settings_sel |= BME280_STANDBY_SEL;
  settings_sel |= BME280_FILTER_SEL;
  rslt = bme280_set_sensor_settings(settings_sel, dev);
  rslt = bme280_set_sensor_mode(BME280_NORMAL_MODE, dev);

}

