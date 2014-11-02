ifndef PETALINUX
$(error You must source the petalinux/settings.sh script before working with PetaLinux)
endif

# Point to default PetaLinux root directory
ifndef ROOTDIR
ROOTDIR=$(PETALINUX)/software/petalinux-dist
endif

PATH:=$(PATH):$(ROOTDIR)/tools

UCLINUX_BUILD_USER = 1
include $(ROOTDIR)/.config
-include $(ROOTDIR)/$(CONFIG_LINUXDIR)/.config
-include $(ROOTDIR)/config/.config
LIBCDIR = $(CONFIG_LIBCDIR)
STAGEDIR=$(ROOTDIR)/stage
include $(ROOTDIR)/config.arch
ROMFSDIR=$(ROOTDIR)/romfs
ROMFSINST=$(ROOTDIR)/tools/romfs-inst.sh

APP = pushbutton_snake

# Add any other object files to this list below
APP_OBJS = pushbutton_snake.o

all: $(APP)

$(APP): $(APP_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(APP_OBJS) $(LDLIBS)

clean:
	-rm -f $(APP) *.elf *.gdb *.o

.PHONY: romfs image

# Optionally strip the final file
ifndef CONFIG_USER_DEBUG
DO_STRIP=do_strip
endif

do_strip: all
	$(STRIP) $(APP)

romfs: all $(DO_STRIP)
	$(ROMFSINST) -d $(APP) /bin/$(APP)

image: romfs
	make -C ${PETALINUX}/software/petalinux-dist image

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<


# Targets for the required .config files - if they don't exist, the tree isn't
# configured.  Tell the user this, how to fix it, and exit.
${ROOTDIR}/config.arch ${ROOTDIR}/.config:
	@echo "Error: You must configure the PetaLinux tree before compiling your application"
	@echo ""
	@echo "Change directory to ../../petalinux-dist and 'make menuconfig' or 'make xconfig'"
	@echo ""
	@echo "Once the tree is configured, return to this directory, and re-run make."
	@echo ""
	@exit -1

