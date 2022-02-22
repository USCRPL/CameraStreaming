CC = gcc
EXTRA_CFLAGS := -Wno-pointer-to-int-cast
CFLAGS = $(EXTRA_CFLAGS) -MD -Wall -g -O2 -I../../include -I../lib -I./ -Wno-unused-but-set-variable
LFLAGS = -Wall -g -O2
RM = rm -f

TARGETS = it950x_testkit_rx
OBJS = api.o it950x_testkit_rx.o

ifeq "$(MACHINE)" "x86_64"
EXTRA_CFLAGS += -Wno-pointer-to-int-cast
endif

all: $(OBJS) $(TARGETS) 
	@echo
	@echo "--------------------------------------------------------------------------------"
	@echo " please call ./it950x_testkit_rx for DVB-T Test"
	@echo "--------------------------------------------------------------------------------"
	@echo 

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

.o:
	$(CC) $(LFLAGS) -o $@ api.o it950x_testkit_rx.o

clean:
	$(RM) $(TARGETS) core* *.o *.d .depend

-include $(wildcard *.d) dummy

