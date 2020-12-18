#include <stdio.h>
#include <libgen.h>
#include <stdlib.h>

#include <sys/types.h>

#include <sqlite3.h>
#include <sys/time.h>
#include <unistd.h>
#include "bme280.h"
#include "tools.h"

// #define DEBUG

//

int main(int argc, char* argv[])
{
  struct bme280_dev dev;
  struct bme280_data comp_data;
  int8_t rslt = BME280_OK;
  int ret;

  sqlite3 *db = NULL;

  struct timeval currtime;
  struct timeval nexttime;
  struct timeval steptime;
  struct timeval difftime;
  
  int opt = 0;

  char *IIC_Dev = "/dev/i2c-0";
  char *dbfname = NULL;
  char *pidfname = NULL;
  int   timeinterval = 5;
  int   daemonflag = 0;
  int   errflag = 0;

  FILE *fl;
  int   nf; 

  FILE *pf;

#define DEF_PID_FILE "/var/run/bme280.pid"

  while ((opt = getopt(argc, argv, "i:f:t:p:d")) != -1) {
    switch(opt) {
      case 'i':
        IIC_Dev = optarg;
        break;
      case 'f':
        dbfname = optarg;
        break;
      case 't':
        timeinterval = atoi(optarg);
        break;
      case 'p':
	pidfname = optarg;
      case 'd':
        daemonflag = 1;
        break;
      case '?':
      default :
        errflag = 1;
        break;
    }
  }

  if (daemonflag) {

    fl = popen("logger -t bme280","w");
    if (fl == NULL) {
        fprintf(stderr, "bme280: Can not redirect STDOUT to logger\n");
        exit(1);
    }
    int nf;
    nf = fileno(fl);
    dup2(nf,STDOUT_FILENO);
    dup2(nf,STDERR_FILENO);
    close(nf);
    fclose(stdin);
    fflush(stdout);
    if (daemon(0, 1)) {
        fprintf(stderr, "bme280: Can not daemonise the process\n");
        exit(1);
    }
  }

  if (errflag) {
    fprintf(stderr, "Usage: %s [-i iic_dev] [-f dbfile] [-t interval] [-d] [-p pidfile]\n", basename(argv[0]));
    exit(1);
  }
  if (daemonflag) {
    fprintf(stderr, "bme280 run in backgroud\n");
    fprintf(stderr, "  argv[0] = %s\n", basename(argv[0]));
    fprintf(stderr, "  IIC_Dev = %s\n", IIC_Dev);
    fprintf(stderr, "  dbfname = %s\n", dbfname);
    fprintf(stderr, "  timeout = %d\n", timeinterval);
    fprintf(stderr, "  daemonize = %s\n\n", (daemonflag == 1) ? "Yes" : "No");

    if (pidfname == NULL)
      pidfname = DEF_PID_FILE;

    if ((pf = fopen(pidfname, "w+")) == NULL) {
      fprintf(stderr, "pid file open error: %s\n", pidfname);
    } else {
      if (fprintf(pf, "%u", getpid()) < 0) {
        fprintf(stderr, "pid file write error: %s\n", pidfname);
      }
      fclose(pf);
    }
  }

  steptime.tv_sec = timeinterval;
  steptime.tv_usec = 0;

  if (dbfname != NULL) {
    if ((db = db_init(dbfname)) == NULL) {
      exit(1);
    }
  }

  if ((ret = bme280_opendev(IIC_Dev, &dev)) != 0) {
    exit(1);
  }
  dev.delay_ms(70);

  if (daemonflag == 0) {
    printf("Temperature           Pressure             Humidity\r\n");
  }

  gettimeofday(&currtime, NULL);

  while (1) {

    timeradd(&currtime, &steptime, &nexttime);

    /* Delay while the sensor completes a measurement */
//    dev.delay_ms(700);
    rslt = bme280_get_sensor_data(BME280_ALL, &comp_data, &dev);
    if (dbfname != NULL) {
      rslt = db_update_bme280_data (db, &comp_data);
    }
    if (daemonflag == 0) {
      print_sensor_data(&comp_data);
    }

    gettimeofday(&currtime, NULL);
    if (timercmp(&nexttime, &currtime, >) ) {
      timersub(&nexttime, &currtime, &difftime);
      sleep(difftime.tv_sec);
      usleep(difftime.tv_usec);
    } else {
      dev.delay_ms(70);
    }
    currtime.tv_sec = nexttime.tv_sec;
    currtime.tv_usec= nexttime.tv_usec;
  }

}
