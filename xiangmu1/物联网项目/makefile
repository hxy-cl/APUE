CC = gcc
CFLAGS = -Wall -Wextra -O2 -D_GNU_SOURCE
OBJS = main.o daemon.o log.o config.o pool.o process_pool.o \
       alarm_scheduler.o ipc_modules.o password_auth.o \
       io_multiplexing.o queue.o tbf.o cloud_api.o base64.o

LDFLAGS = -lpthread -lrt -lcrypt  -lcjson 
all: iot_gateway

iot_gateway: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o iot_gateway

rund: iot_gateway
	./iot_gateway ./iot_gateway.conf -d

stop:
	killall iot_gateway || true

status:
	pgrep -a iot_gateway

restart: stop rund

.PHONY: clean rund stop status restart
