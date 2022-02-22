CC = gcc
EXTRA_CFLAGS := -Wno-pointer-to-int-cast
CFLAGS = -Wno-unused-but-set-variable $(EXTRA_CFLAGS) -MD -Wall -g -O2 -I../../include -I../lib -I./ 
LFLAGS = -Wall -g -O2 -lc
RM = rm -f

TARGETS = it950x_testkit_tx
OBJS = api.o it950x_testkit_tx.o

ifeq "$(MACHINE)" "x86_64"
EXTRA_CFLAGS += -Wno-pointer-to-int-cast
endif

all: $(OBJS) $(TARGETS)
	@echo 
	@echo "--------------------------------------------------------------------------------"
	@echo " please call ./it950x_testkit_tx for DVB-T Test"
	@echo "--------------------------------------------------------------------------------"
	@echo
 
.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

.o:
	$(CC) $(LFLAGS) -o $@ api.o it950x_testkit_tx.o

clean:
	$(RM) $(TARGETS) core* *.o *.d .depend

-include $(wildcard *.d) dummy

