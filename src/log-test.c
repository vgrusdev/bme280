#include <stdio.h>
#include <unistd.h>
#include <libgen.h>
#include <stdlib.h>

int main (int argc, char *argv[]) {
int opt = 0;

char *IIC_Dev = "/dev/i2c-0";
char *dbfname = NULL;
int   timeinterval = 5;
int   daemonflag = 0;

#define DEBUG

  while ((opt = getopt(argc, argv, "i:f:t:d")) != -1) {
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
      case 'd':
	daemonflag = 1;
	break;
      case '?':
      default :
	fprintf(stderr, "Usage: %s [-i iic_dev] [-f dbfile] [-t interval] [-d]\n", basename(argv[0]));
	exit(1);
        break;
    }
  }

#ifdef DEBUG
  fprintf(stderr, "Input options:\n");
  fprintf(stderr, "  argv[0] = %s\n", basename(argv[0]));
  fprintf(stderr, "  IIC_Dev = %s\n", IIC_Dev);
  fprintf(stderr, "  dbfname = %s\n", dbfname);
  fprintf(stderr, "  timeout = %d\n", timeinterval);
  fprintf(stderr, "  daemonize = %s\n\n", (daemonflag == 1) ? "Yes" : "No");
#endif


  printf("\n");
  return 0;

    FILE *fl;
    fl = popen("logger -t log-test","w");
    if(fl == NULL)
        return 1;
//    fprintf(fl,"logger test new");//this goes to /var/log/messages
    int nf;
    nf = fileno(fl);
    dup2(nf,STDOUT_FILENO);
    dup2(nf,STDERR_FILENO);
    close(nf);
    fprintf(stdout,"Wriiten in stdout-1\n");
    fprintf(stderr,"Wriiten in stderr-1\n");
    fflush(stdout);
    fflush(stderr);
}

