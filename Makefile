.SILENT:

DEBUG = 0
EESIO_DEBUG = 0
#comment following line to build without vmc
VMC = 1
CHILDPROOF = 0
RTL = 0

FRONTEND_OBJS = obj/pad.o obj/fntsys.o obj/renderman.o obj/menusys.o obj/system.o obj/debug.o obj/lang.o obj/config.o obj/hdd.o obj/dialogs.o \
		obj/dia.o obj/ioman.o obj/texcache.o obj/themes.o obj/supportbase.o obj/usbsupport.o obj/ethsupport.o obj/hddsupport.o \
		obj/appsupport.o obj/gui.o obj/textures.o obj/opl.o obj/atlas.o

GFX_OBJS =	obj/usb_icon.o obj/hdd_icon.o obj/eth_icon.o obj/app_icon.o \
		obj/cross_icon.o obj/triangle_icon.o obj/circle_icon.o obj/square_icon.o obj/select_icon.o obj/start_icon.o \
		obj/left_icon.o obj/right_icon.o obj/up_icon.o obj/down_icon.o obj/L1_icon.o obj/L2_icon.o obj/R1_icon.o obj/R2_icon.o \
		obj/load0.o obj/load1.o obj/load2.o obj/load3.o obj/load4.o obj/load5.o obj/load6.o obj/load7.o obj/logo.o obj/freesans.o \
		obj/icon_sys.o obj/icon_icn.o

EECORE_OBJS = obj/ee_core.o \
		obj/alt_ee_core.o obj/elfldr.o obj/imgdrv.o obj/eesync.o \
		obj/usb_cdvdman.o obj/usb_4Ksectors_cdvdman.o obj/smb_cdvdman.o obj/smb_pcmcia_cdvdman.o \
		obj/hdd_cdvdman.o obj/hdd_pcmcia_cdvdman.o obj/hdd_hdpro_cdvdman.o \
		obj/cdvdfsv.o obj/usbd_ps2.o obj/usbd_ps3.o obj/usbhdfsd.o obj/cddev.o \
		obj/ps2dev9.o obj/smsutils.o obj/smstcpip.o obj/ingame_smstcpip.o obj/smsmap.o obj/smbman.o obj/discid.o \
		obj/ps2atad.o obj/hdpro_atad.o obj/poweroff.o obj/ps2hdd.o obj/genvmc.o obj/hdldsvr.o \
		obj/udptty.o obj/iomanx.o obj/filexio.o obj/ps2fs.o obj/util.o obj/ioptrap.o obj/ps2link.o 

ifeq ($(VMC),1)
EECORE_OBJS += obj/usb_mcemu.o obj/hdd_mcemu.o obj/smb_mcemu.o 
endif

EE_BIN = opl.elf
EE_SRC_DIR = src/
EE_OBJS_DIR = obj/
EE_ASM_DIR = asm/
EE_OBJS = $(FRONTEND_OBJS) $(GFX_OBJS) $(EECORE_OBJS)

EE_LIBS = -L$(PS2SDK)/ports/lib -L$(GSKIT)/lib -lgskit -ldmakit -lgskit_toolkit -lpoweroff -lfileXio -lpatches -lpad -ljpeg -lpng -lz -ldebug -lm -lmc -lfreetype -lvux -lcdvd
#EE_LIBS = -L$(PS2SDK)/ports/lib -L$(GSKIT)/lib -lgskit -ldmakit -lgskit_toolkit -ldebug -lpoweroff -lfileXio -lpatches -lpad -lm -lmc -lfreetype
EE_INCS += -I$(PS2SDK)/ports/include -I$(GSKIT)/include -I$(GSKIT)/ee/dma/include -I$(GSKIT)/ee/gs/include -I$(GSKIT)/ee/toolkit/include

ifeq ($(DEBUG),1) 
	EE_CFLAGS := -D__DEBUG -g
	ifeq ($(EESIO_DEBUG),1)
		EE_CFLAGS += -D__EESIO_DEBUG
	endif
else
	EE_CFLAGS := -O2
endif

ifeq ($(RTL),1)
	EE_CFLAGS += -D__RTL
endif

ifeq ($(CHILDPROOF),1)
	EE_CFLAGS += -D__CHILDPROOF
endif

ifeq ($(VMC),1)
	EE_CFLAGS += -DVMC
	VMC_FLAGS = VMC=1
else
	VMC_FLAGS = VMC=0
endif

SMSTCPIP_INGAME_CFLAGS = INGAME_DRIVER=1
ifeq ($(DEBUG),1)
	MOD_DEBUG_FLAGS = DEBUG=1
endif
ifeq ($(EESIO_DEBUG),1)
	EECORE_DEBUG_FLAGS = EESIO_DEBUG=1
endif
ifeq ($(INGAME_DEBUG),1)
	EECORE_DEBUG_FLAGS = LOAD_DEBUG_MODULES=1
	CDVDMAN_DEBUG_FLAGS = USE_DEV9=1
	SMSTCPIP_INGAME_CFLAGS = 
else
	ifeq ($(IOPCORE_DEBUG),1)
		EECORE_DEBUG_FLAGS = LOAD_DEBUG_MODULES=1
		CDVDMAN_DEBUG_FLAGS = IOPCORE_DEBUG=1
		MCEMU_DEBUG_FLAGS = IOPCORE_DEBUG=1
		SMSTCPIP_INGAME_CFLAGS = 
	endif
endif


all:
	@mkdir -p obj
	@mkdir -p asm
	
	echo "Building Open PS2 Loader..."
	echo "    * Interface"
	$(MAKE) $(EE_BIN)
	
ifeq ($(DEBUG),0)
	echo "Stripping..."
	ee-strip opl.elf

	echo "Compressing..."
	ps2-packer opl.elf OPNPS2LD.ELF > /dev/null
endif

childproof:
	$(MAKE) CHILDPROOF=1 all
	
debug:
	$(MAKE) DEBUG=1 all

ingame_debug:
	$(MAKE) INGAME_DEBUG=1 DEBUG=1 all

eesio_debug:
	$(MAKE) EESIO_DEBUG=1 DEBUG=1 all

iopcore_debug:
	$(MAKE) IOPCORE_DEBUG=1 DEBUG=1 all

clean:  sclean

sclean:
	echo "Cleaning..."
	echo "    * Interface"
	rm -f $(EE_BIN) OPNPS2LD.ELF asm/*.* obj/*.*
	echo "    * EE core"
	$(MAKE) -C ee_core clean
	$(MAKE) -C ee_core -f Makefile.alt clean
	echo "    * Elf Loader"
	$(MAKE) -C elfldr clean
	echo "    * imgdrv.irx"
	$(MAKE) -C modules/iopcore/imgdrv clean
	echo "    * eesync.irx"
	$(MAKE) -C modules/iopcore/eesync clean
	echo "    * cdvdman.irx"
	$(MAKE) -C modules/iopcore/cdvdman -f Makefile.usb clean
	$(MAKE) -C modules/iopcore/cdvdman -f Makefile.usb.4Ksectors clean
	$(MAKE) -C modules/iopcore/cdvdman -f Makefile.smb clean
	$(MAKE) -C modules/iopcore/cdvdman -f Makefile.smb.pcmcia clean
	$(MAKE) -C modules/iopcore/cdvdman -f Makefile.hdd clean
	$(MAKE) -C modules/iopcore/cdvdman -f Makefile.hdd.pcmcia clean
	$(MAKE) -C modules/iopcore/cdvdman -f Makefile.hdd.hdpro clean
	echo "    * cdvdfsv.irx"
	$(MAKE) -C modules/iopcore/cdvdfsv clean
	echo "    * cddev.irx"
	$(MAKE) -C modules/iopcore/cddev clean
	echo "    * usbhdfsd.irx"
	$(MAKE) -C modules/usb/usbhdfsd clean
	echo "    * ps2dev9.irx"
	$(MAKE) -C modules/dev9 clean
	echo "    * SMSUTILS.irx"
	$(MAKE) -C modules/network/SMSUTILS clean
	echo "    * SMSTCPIP.irx"
	$(MAKE) -C modules/network/SMSTCPIP clean
	echo "    * SMSMAP.irx"
	$(MAKE) -C modules/network/SMSMAP clean
	echo "    * discID.irx"
	$(MAKE) -C modules/cdvd/discID clean
	echo "    * ps2atad.irx"
	$(MAKE) -C modules/hdd/atad clean
	echo "    * hdpro_atad.irx"
	$(MAKE) -C modules/hdd/hdpro_atad clean
	echo "    * ps2hdd.irx"
	$(MAKE) -C modules/hdd/ps2hdd clean
ifeq ($(VMC),1)
	echo "    * ps2fs.irx"
	$(MAKE) -C modules/ps2fs clean
	echo "    * mcemu.irx"
	$(MAKE) -C modules/mcemu -f Makefile.usb clean
	$(MAKE) -C modules/mcemu -f Makefile.hdd clean
	$(MAKE) -C modules/mcemu -f Makefile.smb clean
endif
	echo "    * genvmc.irx"
	$(MAKE) -C modules/vmc/genvmc clean
	echo "    * hdldsvr.irx"
	$(MAKE) -C modules/hdd/hdldsvr clean
	echo "    * udptty.irx"
	$(MAKE) -C modules/debug/udptty clean
	echo "    * ioptrap.irx"
	$(MAKE) -C modules/debug/ioptrap clean
	echo "    * ps2link.irx"
	$(MAKE) -C modules/debug/ps2link clean
	echo "    * pc tools"
	$(MAKE) -C pc clean

rebuild: clean all

pc_tools:
	echo "Building iso2opl, opl2iso and genvmc..."
	$(MAKE) _WIN32=0 -C pc

pc_tools_win32:
	echo "Building WIN32 iso2opl, opl2iso and genvmc..."
	$(MAKE) _WIN32=1 -C pc

ee_core.s:
	echo "    * EE core"
	$(MAKE) -C ee_core clean
	$(MAKE) $(VMC_FLAGS) $(EECORE_DEBUG_FLAGS) -C ee_core
	bin2s ee_core/ee_core.elf asm/ee_core.s eecore_elf

alt_ee_core.s:
	echo "    * alternative EE core"
	$(MAKE) -C ee_core -f Makefile.alt clean
	$(MAKE) $(VMC_FLAGS) $(EECORE_DEBUG_FLAGS) -C ee_core -f Makefile.alt
	bin2s ee_core/ee_core.elf asm/alt_ee_core.s alt_eecore_elf

elfldr.s:
	echo "    * Elf Loader"
	$(MAKE) -C elfldr clean
	$(MAKE) -C elfldr
	bin2s elfldr/elfldr.elf asm/elfldr.s elfldr_elf

imgdrv.s:
	echo "    * imgdrv.irx"
	$(MAKE) -C modules/iopcore/imgdrv
	bin2s modules/iopcore/imgdrv/imgdrv.irx asm/imgdrv.s imgdrv_irx

eesync.s:
	echo "    * eesync.irx"
	$(MAKE) -C modules/iopcore/eesync
	bin2s modules/iopcore/eesync/eesync.irx asm/eesync.s eesync_irx

usb_cdvdman.s:
	echo "    * usb_cdvdman.irx"
	$(MAKE) $(VMC_FLAGS) $(CDVDMAN_DEBUG_FLAGS) -C modules/iopcore/cdvdman -f Makefile.usb rebuild
	bin2s modules/iopcore/cdvdman/cdvdman.irx asm/usb_cdvdman.s usb_cdvdman_irx

usb_4Ksectors_cdvdman.s:
	echo "    * usb_4Ksectors_cdvdman.irx"
	$(MAKE) $(VMC_FLAGS) $(CDVDMAN_DEBUG_FLAGS) -C modules/iopcore/cdvdman -f Makefile.usb.4Ksectors rebuild
	bin2s modules/iopcore/cdvdman/cdvdman.irx asm/usb_4Ksectors_cdvdman.s usb_4Ksectors_cdvdman_irx

smb_cdvdman.s:
	echo "    * smb_cdvdman.irx"
	$(MAKE) $(VMC_FLAGS) $(CDVDMAN_DEBUG_FLAGS) -C modules/iopcore/cdvdman -f Makefile.smb rebuild
	bin2s modules/iopcore/cdvdman/cdvdman.irx asm/smb_cdvdman.s smb_cdvdman_irx

smb_pcmcia_cdvdman.s:
	echo "    * smb_pcmcia_cdvdman.irx"
	$(MAKE) $(VMC_FLAGS) $(CDVDMAN_DEBUG_FLAGS) -C modules/iopcore/cdvdman -f Makefile.smb.pcmcia rebuild
	bin2s modules/iopcore/cdvdman/cdvdman.irx asm/smb_pcmcia_cdvdman.s smb_pcmcia_cdvdman_irx

hdd_cdvdman.s:
	echo "    * hdd_cdvdman.irx"
	$(MAKE) $(VMC_FLAGS) $(CDVDMAN_DEBUG_FLAGS) -C modules/iopcore/cdvdman -f Makefile.hdd rebuild
	bin2s modules/iopcore/cdvdman/cdvdman.irx asm/hdd_cdvdman.s hdd_cdvdman_irx

hdd_pcmcia_cdvdman.s:
	echo "    * hdd_pcmcia_cdvdman.irx"
	$(MAKE) $(VMC_FLAGS) $(CDVDMAN_DEBUG_FLAGS) -C modules/iopcore/cdvdman -f Makefile.hdd.pcmcia rebuild
	bin2s modules/iopcore/cdvdman/cdvdman.irx asm/hdd_pcmcia_cdvdman.s hdd_pcmcia_cdvdman_irx

hdd_hdpro_cdvdman.s:
	echo "    * hdd_hdpro_cdvdman.irx"
	$(MAKE) $(VMC_FLAGS) $(CDVDMAN_DEBUG_FLAGS) -C modules/iopcore/cdvdman -f Makefile.hdd.hdpro rebuild
	bin2s modules/iopcore/cdvdman/cdvdman.irx asm/hdd_hdpro_cdvdman.s hdd_hdpro_cdvdman_irx

cdvdfsv.s:
	echo "    * cdvdfsv.irx"
	$(MAKE) -C modules/iopcore/cdvdfsv
	bin2s modules/iopcore/cdvdfsv/cdvdfsv.irx asm/cdvdfsv.s cdvdfsv_irx

cddev.s:
	echo "    * cddev.irx"
	$(MAKE) -C modules/iopcore/cddev
	bin2s modules/iopcore/cddev/cddev.irx asm/cddev.s cddev_irx

usb_mcemu.s:
	echo "    * usb_mcemu.irx"
	$(MAKE) $(MCEMU_DEBUG_FLAGS) -C modules/mcemu -f Makefile.usb rebuild
	bin2s modules/mcemu/mcemu.irx asm/usb_mcemu.s usb_mcemu_irx

hdd_mcemu.s:
	echo "    * hdd_mcemu.irx"
	$(MAKE) $(MCEMU_DEBUG_FLAGS) -C modules/mcemu -f Makefile.hdd rebuild
	bin2s modules/mcemu/mcemu.irx asm/hdd_mcemu.s hdd_mcemu_irx

smb_mcemu.s:
	echo "    * smb_mcemu.irx"
	$(MAKE) $(MCEMU_DEBUG_FLAGS) -C modules/mcemu -f Makefile.smb rebuild
	bin2s modules/mcemu/mcemu.irx asm/smb_mcemu.s smb_mcemu_irx

usbd_ps2.s:
	bin2s $(PS2SDK)/iop/irx/usbd.irx asm/usbd_ps2.s usbd_ps2_irx

usbd_ps3.s:
	bin2s modules/usb/usbd/usbd.irx asm/usbd_ps3.s usbd_ps3_irx

usbhdfsd.s:
	echo "    * usbhdfsd.irx"
	$(MAKE) -C modules/usb/usbhdfsd
	bin2s modules/usb/usbhdfsd/usbhdfsd.irx asm/usbhdfsd.s usbhdfsd_irx

ps2dev9.s:
	echo "    * ps2dev9.irx"
	$(MAKE) -C modules/dev9
	bin2s modules/dev9/ps2dev9.irx asm/ps2dev9.s ps2dev9_irx

smsutils.s:
	echo "    * SMSUTILS.irx"
	$(MAKE) -C modules/network/SMSUTILS
	bin2s modules/network/SMSUTILS/SMSUTILS.irx asm/smsutils.s smsutils_irx

smstcpip.s:
	echo "    * SMSTCPIP.irx"
	$(MAKE) -C modules/network/SMSTCPIP -f Makefile rebuild
	bin2s modules/network/SMSTCPIP/SMSTCPIP.irx asm/smstcpip.s smstcpip_irx

ingame_smstcpip.s:
	echo "    * in-game SMSTCPIP.irx"
	$(MAKE) $(SMSTCPIP_INGAME_CFLAGS) -C modules/network/SMSTCPIP rebuild
	bin2s modules/network/SMSTCPIP/SMSTCPIP.irx asm/ingame_smstcpip.s ingame_smstcpip_irx

smsmap.s:
	echo "    * SMSMAP.irx"
	$(MAKE) -C modules/network/SMSMAP
	bin2s modules/network/SMSMAP/SMSMAP.irx asm/smsmap.s smsmap_irx

smbman.s:
	bin2s $(PS2SDK)/iop/irx/smbman.irx asm/smbman.s smbman_irx

discid.s:
	echo "    * discID.irx"
	$(MAKE) -C modules/cdvd/discID
	bin2s modules/cdvd/discID/discID.irx asm/discid.s discid_irx

ps2atad.s:
	echo "    * ps2atad.irx"
	$(MAKE) -C modules/hdd/atad
	bin2s modules/hdd/atad/ps2atad.irx asm/ps2atad.s ps2atad_irx

hdpro_atad.s:
	echo "    * hdpro_atad.irx"
	$(MAKE) -C modules/hdd/hdpro_atad
	bin2s modules/hdd/hdpro_atad/hdpro_atad.irx asm/hdpro_atad.s hdpro_atad_irx

poweroff.s:
	bin2s $(PS2SDK)/iop/irx/poweroff.irx asm/poweroff.s poweroff_irx

ps2hdd.s:
	echo "    * ps2hdd.irx"
	$(MAKE) -C modules/hdd/ps2hdd
	bin2s modules/hdd/ps2hdd/ps2hdd.irx asm/ps2hdd.s ps2hdd_irx

genvmc.s:
	echo "    * genvmc.irx"
	$(MAKE) $(MOD_DEBUG_FLAGS) -C modules/vmc/genvmc
	bin2s modules/vmc/genvmc/genvmc.irx asm/genvmc.s genvmc_irx

hdldsvr.s:
	echo "    * hdldsvr.irx"
	$(MAKE) -C modules/hdd/hdldsvr
	bin2s modules/hdd/hdldsvr/hdldsvr.irx asm/hdldsvr.s hdldsvr_irx

udptty.s:
	echo "    * udptty.irx"
	$(MAKE) -C modules/debug/udptty
	bin2s modules/debug/udptty/udptty.irx asm/udptty.s udptty_irx

ioptrap.s:
	echo "    * ioptrap.irx"
	$(MAKE) -C modules/debug/ioptrap
	bin2s modules/debug/ioptrap/ioptrap.irx asm/ioptrap.s ioptrap_irx

ps2link.s:
	echo "    * ps2link.irx"
	$(MAKE) -C modules/debug/ps2link
	bin2s modules/debug/ps2link/ps2link.irx asm/ps2link.s ps2link_irx

ps2fs.s:
	echo "    * ps2fs.irx"
	$(MAKE) -C modules/ps2fs
	bin2s modules/ps2fs/ps2fs.irx asm/ps2fs.s ps2fs_irx

iomanx.s:
	bin2s $(PS2SDK)/iop/irx/iomanX.irx asm/iomanx.s iomanx_irx

filexio.s:
	bin2s $(PS2SDK)/iop/irx/fileXio.irx asm/filexio.s filexio_irx

load0.s:
	bin2s gfx/load0.png asm/load0.s load0_png

load1.s:
	bin2s gfx/load1.png asm/load1.s load1_png

load2.s:
	bin2s gfx/load2.png asm/load2.s load2_png

load3.s:
	bin2s gfx/load3.png asm/load3.s load3_png

load4.s:
	bin2s gfx/load4.png asm/load4.s load4_png

load5.s:
	bin2s gfx/load5.png asm/load5.s load5_png

load6.s:
	bin2s gfx/load6.png asm/load6.s load6_png

load7.s:
	bin2s gfx/load7.png asm/load7.s load7_png
	
logo.s:
	bin2s gfx/logo.png asm/logo.s logo_png
		
usb_icon.s:
	bin2s gfx/usb.png asm/usb_icon.s usb_png
	
hdd_icon.s:
	bin2s gfx/hdd.png asm/hdd_icon.s hdd_png
	
eth_icon.s:
	bin2s gfx/eth.png asm/eth_icon.s eth_png
		
app_icon.s:
	bin2s gfx/app.png asm/app_icon.s app_png
  
cross_icon.s:
	bin2s gfx/cross.png asm/cross_icon.s cross_png

triangle_icon.s:
	bin2s gfx/triangle.png asm/triangle_icon.s triangle_png

circle_icon.s:
	bin2s gfx/circle.png asm/circle_icon.s circle_png

square_icon.s:
	bin2s gfx/square.png asm/square_icon.s square_png

select_icon.s:
	bin2s gfx/select.png asm/select_icon.s select_png

start_icon.s:
	bin2s gfx/start.png asm/start_icon.s start_png

left_icon.s:
	bin2s gfx/left.png asm/left_icon.s left_png

right_icon.s:
	bin2s gfx/right.png asm/right_icon.s right_png

up_icon.s:
	bin2s gfx/up.png asm/up_icon.s up_png

down_icon.s:
	bin2s gfx/down.png asm/down_icon.s down_png

L1_icon.s:
	bin2s gfx/L1.png asm/L1_icon.s L1_png

L2_icon.s:
	bin2s gfx/L2.png asm/L2_icon.s L2_png

R1_icon.s:
	bin2s gfx/R1.png asm/R1_icon.s R1_png

R2_icon.s:
	bin2s gfx/R2.png asm/R2_icon.s R2_png

freesans.s:
	bin2s thirdparty/FreeSans_basic_latin.ttf asm/freesans.s freesansfont_raw

icon_sys.s:
	bin2s gfx/icon.sys asm/icon_sys.s icon_sys
	
icon_icn.s:
	bin2s gfx/opl.icn asm/icon_icn.s icon_icn	
  
$(EE_OBJS_DIR)%.o : $(EE_SRC_DIR)%.c
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@

$(EE_OBJS_DIR)%.o : %.s
	$(EE_AS) $(EE_ASFLAGS) $(EE_ASM_DIR)$< -o $@


include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
