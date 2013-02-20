
DEBUGFLAGS += -O2
DEBUGFLAGS += -g # Add -g in all cases to enable crash debuginfo

EXTRA_CFLAGS += $(DEBUGFLAGS)
EXTRA_CFLAGS += -I$(M)
#EXTRA_CFLAGS += -E

ifneq ($(KERNELRELEASE),)

obj-m := kwtp.o

else

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD       := $(shell pwd)


modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

endif

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions Module.symvers *.unsigned modules.order

depend .depend dep:
	$(CC) $(CFLAGS) -M *.c > .depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif
