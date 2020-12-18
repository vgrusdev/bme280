# build helloworld executable when user executes "make"
bme280:main.o bme280.o tools.o sql-select.o log-test.o
	$(CC) $(LDFLAGS) -lsqlite3 main.o bme280.o tools.o -o bme280
	$(CC) $(LDFLAGS) -lsqlite3 sql-select.o -o sql-select
	$(CC) $(LDFLAGS) log-test.o -o log-test
main.o: main.c bme280.h bme280_defs.h tools.h
	$(CC) $(CFLAGS) -c main.c
bme280.o: bme280.c bme280.h bme280_defs.h
	$(CC) $(CFLAGS) -c bme280.c
tools.o: tools.c bme280.h bme280_defs.h tools.h
	$(CC) $(CFLAGS) -c tools.c
sql-select.o: sql-select.c
	$(CC) $(CFLAGS) -c sql-select.c
log-test.o: log-test.c
	$(CC) $(CLFAGS) -c log-test.c

# remove object files and executable when user executes "make clean"
clean:
	rm main.o bme280.o bme280 tools.o
	rm sql-select.o sql-select
	rm log-test.o log-test
