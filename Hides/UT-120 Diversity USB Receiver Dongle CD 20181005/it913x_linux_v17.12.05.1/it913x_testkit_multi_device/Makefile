CC = gcc
CURRENT = $(shell uname -r)

CFLAGS = -MD -Wall -g -O2 -I../../include -I../lib

VERSION_MAJOR = $(shell uname -r | awk -F"[.-]" '{print $$1}')
VERSION_MINOR = $(shell uname -r | awk -F"[.-]" '{print $$2}')

#kernel 3.7.0 path change
VERSION_FLAG := $(shell if [ $(VERSION_MAJOR) -gt 2 ] ; then if [ $(VERSION_MINOR) -gt 6 -o $(VERSION_MAJOR) -gt 3 ] ; then echo -I/lib/modules/$(shell uname -r)/build/include/generated/uapi/ ; fi ; fi)

#if not set VERSION_FLAG
ifeq (,$(findstring lib,$(VERSION_FLAG)))
VERSION_FLAG := -I/lib/modules/$(shell uname -r)/build/include/
endif

CFLAGS += $(VERSION_FLAG)

LFLAGS = -Wall -g -O2 -pthread
RM = rm -f

TARGETS = it913x_testkit_multi_device
OBJS = it913x_testkit_multi_device.o

all: $(OBJS) $(TARGETS)
	@echo
	@echo "--------------------------------------------------------------------------------"
	@echo " please call ./it913x_testkit_multi_device for DVB-T Test"
	@echo "--------------------------------------------------------------------------------"
	@echo

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

.o:
	$(CC) $(LFLAGS) -o $@ $<

clean:
	$(RM) $(TARGETS) core* *.o *.d .depend

-include $(wildcard *.d) dummy
