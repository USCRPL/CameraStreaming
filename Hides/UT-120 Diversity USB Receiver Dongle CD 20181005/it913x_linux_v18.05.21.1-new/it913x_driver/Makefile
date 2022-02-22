module = dvb-usb-it913x
EXTRA_CFLAGS = -DEXPORT_SYMTAB

# passing dynamic URB_BUFSIZE to driver
export URB_BUFSIZE
ALIGNSIZE = 188 

CURRENT = $(shell uname -r)
VERSION_MAJOR = $(shell uname -r | awk -F"[.-]" '{print $$1}')
VERSION_MINOR = $(shell uname -r | awk -F"[.-]" '{print $$2}')
VERSION_REVISION = $(shell uname -r | awk -F"[.-]" '{print $$3}')

#for kernel 3.x.x
V4LPATH := $(shell if [ $(VERSION_MAJOR) -gt 2 ] ; then echo v4l/kernel-$(VERSION_MAJOR).$(VERSION_MINOR).0/*; fi)

#if not set V4LPATH
ifeq (,$(findstring kernel,$(V4LPATH)))
V4LPATH := v4l/kernel-$(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_REVISION)/*
endif

KDIR = /lib/modules/$(CURRENT)/build
PWD = $(shell pwd)

KDIR_NATIVE := /lib/modules/$(CURRENT)/kernel/drivers/media
KDIR26 := /lib/modules/$(CURRENT)/kernel/drivers/media/dvb/dvb-usb
LMDIR26 := /lib/firmware

# SYSTEM DVB setting
DVB_FLAGS += -I$(KSRC)/drivers/media/dvb/dvb-core/
DVB_FLAGS += -I$(KSRC)/drivers/media/dvb/dvb-usb/
DVB_FLAGS += -I$(KSRC)/drivers/media/dvb/frontends/
EXTRA_CFLAGS += $(DVB_FLAGS)

EXTRA_CFLAGS += -Wno-unused-value -Wno-unused-variable -Wno-unused-parameter \
		-Wno-switch

dvb-usb-it913x-objs := \
		it913x-core.o it913x-devices.o it913x-drv.o it913x-fe.o it913x-tuner.o it913x-dc.o ADRF6655.o RFFC2072.o \
		demodulator.o omega.o standard.o user.o Afatech_OMEGA.o \
		cmd.o demodulatorextend.o usb2impl.o iocontrol.o
obj-m += dvb-usb-it913x.o

ifneq (,$(findstring 2.6.01,$(CURRENT)))
dvb-usb-objs := dvb-usb-firmware.o dvb-usb-init.o dvb-usb-urb.o \
		  dvb-usb-i2c.o dvb-usb-dvb.o dvb-usb-remote.o usb-urb.o
obj-m += dvb-usb.o

dvb-core-objs := dvbdev.o dmxdev.o dvb_demux.o dvb_filter.o 	\
		 dvb_frontend.o  dvb_net.o dvb_ringbuffer.o dvb_math.o 

obj-m += dvb-core.o
obj-m += dvb-pll.o
endif

ifneq (,$(findstring 2.6.18,$(CURRENT)))
dvb-usb-objs := dvb-usb-firmware.o dvb-usb-init.o dvb-usb-urb.o \
		  dvb-usb-i2c.o dvb-usb-dvb.o dvb-usb-remote.o usb-urb.o
obj-m += dvb-usb.o

dvb-core-objs := dvbdev.o dmxdev.o dvb_demux.o dvb_filter.o 	\
		 dvb_frontend.o  dvb_net.o dvb_ringbuffer.o dvb_math.o 

obj-m += dvb-core.o
obj-m += dvb-pll.o
endif

ifneq (,$(findstring 2.6.19,$(CURRENT)))
dvb-usb-objs := dvb-usb-firmware.o dvb-usb-init.o dvb-usb-urb.o \
		  dvb-usb-i2c.o dvb-usb-dvb.o dvb-usb-remote.o usb-urb.o
obj-m += dvb-usb.o

dvb-core-objs := dvbdev.o dmxdev.o dvb_demux.o dvb_filter.o 	\
		 dvb_frontend.o  dvb_net.o dvb_ringbuffer.o dvb_math.o 

obj-m += dvb-core.o
obj-m += dvb-pll.o
endif

ifneq (,$(findstring 2.6.38,$(CURRENT)))
it913x-rc6-table-objs := it913x-rc6.o
obj-m += it913x-rc6-table.o

it913x-nec-table-objs := it913x-nec.o
obj-m += it913x-nec-table.o
endif

default:
	@(cp -f $(V4LPATH) ./; cp api/*.* ./; cp src/*.* ./; cp rc/*.* ./)
	@(cp api/ADRF6655/*.* ./; cp api/RFFC2072/*.* ./)
	make -s -C $(KDIR) SUBDIRS=$(PWD) modules

	@if [ -e "Get_IR_Info.c" ]; then gcc -o Get_IR_Info Get_IR_Info.c; fi
	@if [ -e "it913x-irtbl.c" ]; then gcc -o it913x-irtbl it913x-irtbl.c; fi

install:
	@echo "Removing Native module"
	@files='usb-it913x.ko usb-it9135.ko'; for i in $$files; do if [ -e $(KDIR_NATIVE)/$$i ]; then echo -n "$$i "; rm $(KDIR_NATIVE)/$$i; fi; done; for i in $$files; do if [ -e $(KDIR_NATIVE)/$$i.gz ]; then echo -n "$$i.gz "; rm $(KDIR_NATIVE)/$$i.gz; fi; done; echo;
	@files='usb_it913x usb_it9135 tty_Virtual'; for i in $$files; do modprobe -r $$i 2>/dev/null; true; done;
ifneq (,$(findstring 2.4.,$(CURRENT)))
	@install -d $(KDIR26)
	@for i in dvb-core.o dvb-pll.o dvb-usb.o dvb-usb-it913x.o;do if [ -e "$$i" ]; then echo -n "$$i "; install -m 644 -c $$i $(KDIR26); fi; done; echo;
	/sbin/depmod -a ${KERNELRELEASE}
else

ifneq (,$(findstring 2.6.01,$(CURRENT)))
	@install -d $(KDIR26)
	@for i in dvb-core.ko dvb-pll.ko dvb-usb.ko dvb-usb-it913x.ko;do if [ -e "$$i" ]; then echo -n "$$i "; install -m 644 -c $$i $(KDIR26); fi; done; echo;
else

ifneq (,$(findstring 2.6.18,$(CURRENT)))
	@install -d $(KDIR26)
	@for i in dvb-core.ko dvb-pll.ko dvb-usb.ko dvb-usb-it913x.ko;do if [ -e "$$i" ]; then echo -n "$$i "; install -m 644 -c $$i $(KDIR26); fi; done; echo;
else

ifneq (,$(findstring 2.6.19,$(CURRENT)))
	@install -d $(KDIR26)
	@for i in dvb-core.ko dvb-pll.ko dvb-usb.ko dvb-usb-it913x.ko;do if [ -e "$$i" ]; then echo -n "$$i "; install -m 644 -c $$i $(KDIR26); fi; done; echo;
else

	@echo "Removing V4L module:"
	@files='dvb-usb-it913x.ko dvb-usb-it9135.ko'; for i in $$files; do if [ -e $(KDIR26)/$$i ]; then echo -n "$$i "; rm $(KDIR26)/$$i; fi; done; for i in $$files; do if [ -e $(KDIR26)/$$i.gz ]; then echo -n "$$i.gz "; rm $(KDIR26)/$$i.gz; fi; done; echo;
	@files='it913xirtbl.bin it9135irtbl.bin'; for i in $$files; do if [ -e $(LMDIR26)/$$i ]; then echo -n "$$i "; rm $(LMDIR26)/$$i; fi; done; for i in $$files; do if [ -e $(LMDIR26)/$$i.gz ]; then echo -n "$$i.gz "; rm $(LMDIR26)/$$i.gz; fi; done; echo;
	@files='dvb_usb_it913x dvb_usb_it9135'; for i in $$files; do modprobe -r $$i 2>/dev/null; true; done;

	@/sbin/depmod -a

	@install -d $(KDIR26) 
	@echo "Installing module:"
	@for i in dvb-usb-it913x.ko; do if [ -e "$$i" ]; then echo -n "$$i "; install -m 644 -c $$i $(KDIR26); fi; done; echo;
endif

endif

endif
	@/sbin/depmod -a ${KERNELRELEASE}
	@modprobe -r dvb_usb_it913x
	@for i in it913xirtbl.bin; do if [ -e "$$i" ]; then echo -n "$$i "; cp -rf it913xirtbl.bin $(LMDIR26)/; fi; done; echo;
	@-make clean
endif

remove:
	@echo "Removing old $(KDIR26)/ files:"
	@modprobe -r dvb_usb_it913x
ifneq (,$(findstring 2.6.01,$(CURRENT)))
	@ rm -rf /lib/modules/$(CURRENT)/kernel/drivers/media/dvb
	@ mv /tmp/dvb /lib/modules/$(CURRENT)/kernel/drivers/media
	@files='dvb-core.ko dvb-pll.ko dvb-usb.ko dvb-usb-it913x.ko'; for i in $$files;do if [ -e $(KDIR26)/$$i ]; then echo -n "$$i "; rm $(KDIR26)/$$i; fi; done; for i in $$files;do if [ -e $(KDIR26)/$$i.gz ]; then echo -n "$$i.gz "; rm $(KDIR26)/$$i.gz; fi; done; echo;
else

ifneq (,$(findstring 2.6.18,$(CURRENT)))
	@files='dvb-core.ko dvb-pll.ko dvb-usb.ko dvb-usb-it913x.ko'; for i in $$files;do if [ -e $(KDIR26)/$$i ]; then echo -n "$$i "; rm $(KDIR26)/$$i; fi; done; for i in $$files;do if [ -e $(KDIR26)/$$i.gz ]; then echo -n "$$i.gz "; rm $(KDIR26)/$$i.gz; fi; done; echo;
else

ifneq (,$(findstring 2.6.19,$(CURRENT)))
	@files='dvb-core.ko dvb-pll.ko dvb-usb.ko dvb-usb-it913x.ko'; for i in $$files;do if [ -e $(KDIR26)/$$i ]; then echo -n "$$i "; rm $(KDIR26)/$$i; fi; done; for i in $$files;do if [ -e $(KDIR26)/$$i.gz ]; then echo -n "$$i.gz "; rm $(KDIR26)/$$i.gz; fi; done; echo;

endif
	
endif
	@files='dvb-usb-it913x.ko'; for i in $$files; do if [ -e $(KDIR26)/$$i ]; then echo -n "$$i "; rm $(KDIR26)/$$i; fi; done; for i in $$files; do if [ -e $(KDIR26)/$$i.gz ]; then echo -n "$$i.gz "; rm $(KDIR26)/$$i.gz; fi; done; echo;
	@files='it913xirtbl.bin'; for i in $$files; do if [ -e $(LMDIR26)/$$i ]; then echo -n "$$i "; rm $(LMDIR26)/$$i; fi; done; for i in $$files; do if [ -e $(LMDIR26)/$$i.gz ]; then echo -n "$$i.gz "; rm $(LMDIR26)/$$i.gz; fi; done; echo;
	@/sbin/depmod -a
endif

# ** cu_install , cu_remove
# This two rule is for local using
# cu_install:
#   Let customer could dynamically change URB buffer size.
# cu_remove:
#   Let customer could easily remove local insmod module.
cu_install: 
	@-rm -rf *~ *.o .*.o.cmd .*.ko.cmd *.mod.c *.*~ *.c *.h \
	demux.h dmxdev.h dvb-pll.h dvb-usb-ids.h dvb-usb.h dvb_demux.h dvb_frontend.h \
	dvb_net.h dvb_ringbuffer.h dvbdev.h dvb_math.h modules.order Module.markers Module.symvers Get_IR_Info it913x-irtbl .tmp_versions
	@[ -e "dvb-usb-it913x.ko" ] || \
	(printf "\033[91m dvb-usb-it913x.ko do not exist!!!  Please make first \33[0m \n"; exit 1); 
	@[ -e "$(KDIR26)/dvb-usb-it913x.ko" ] || \
	(cp dvb-usb-it913x.ko $(KDIR26); depmod; echo "copy dvb-usb-it913x.ko to kernel")
	@if [ -z "$(URB_BUFSIZE)" ]; then \
		printf "\033[91m Using DEFAULT URB_BUFSIZE \033[0m \n"; \
		modprobe dvb_usb_it913x; \
	fi
	@if [ -n "$(URB_BUFSIZE)" ]; then \
		if [ $(URB_BUFSIZE) -eq 0 ]; then \
			printf "\033[91m URB_BUFSIZE could not be ZERO!!! Using DEFAULT URB_BUFSIZE \033[0m \n"; \
			modprobe dvb_usb_it913x; \
		else \
			if [ $$(( $(URB_BUFSIZE) % $(ALIGNSIZE) )) -ne 0 ]; then \
				printf "\033[92m Using ALIGNMENT URB_BUFSIZE=$$(( ($(URB_BUFSIZE) / $(ALIGNSIZE) + 1) * $(ALIGNSIZE) )) \033[0m \n"; \
				modprobe dvb_usb_it913x URB_BUFSIZE=$(URB_BUFSIZE); \
			else \
				printf "\033[93m Customer set URB_BUFSIZE=$(URB_BUFSIZE) \33[0m \n"; \
				modprobe dvb_usb_it913x URB_BUFSIZE=$(URB_BUFSIZE); \
			fi \
		fi \
	fi

cu_remove:
	@rmmod dvb_usb_it913x

clean:
	@-rm -rf *~ *.o *.ko .*.o.cmd .*.ko.cmd *.mod.c *.*~ *.c *.h \
	demux.h dmxdev.h dvb-pll.h dvb-usb-ids.h dvb-usb.h dvb_demux.h dvb_frontend.h \
	dvb_net.h dvb_ringbuffer.h dvbdev.h dvb_math.h modules.order Module.markers Module.symvers Get_IR_Info it913x-irtbl .tmp_versions

release:
	@-rm -rf *~ it913x*.o *.ko .*.o.cmd .*.ko.cmd *.mod.c *.*~ *.c it913x*.h \
	demux.h dmxdev.h dvb-pll.h dvb-usb-ids.h dvb-usb.h dvb_demux.h dvb_frontend.h \
	dvb_net.h dvb_ringbuffer.h dvbdev.h dvb_math.h dvb-usb-it913x.*
	@-rm -rf api/*.*
	@-mv *.h *.o api
	@-make clean
-include $(KOBJ)/Rules.make
