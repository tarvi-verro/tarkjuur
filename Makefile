
HOSTCC:=gcc

CC:=arm-none-eabi-gcc
AS:=arm-none-eabi-as
AR:=arm-none-eabi-ar
LD:=arm-none-eabi-ld
OC:=arm-none-eabi-objcopy


O?=build
PRINT_PRETTY?=1

f1: $O/prg-f1

PATH_LIBNEW?=$(eval PATH_LIBNEW:=$$(dir $$(abspath $$(shell \
	     $$(CC) $$(CFLAGS_F1) -print-file-name=libnosys.a \
	     ))))$(PATH_LIBNEW)
PATH_LIBGCC?=$(eval PATH_LIBGCC:=$$(dir $$(abspath $$(shell \
	     $$(CC) $$(CFLAGS_F1) -print-file-name=libgcc.a \
	     ))))$(PATH_LIBGCC)

SRC:=main.c usb-pck.c cmd.c cli.c frob/assert.c frob/usb.c \
	frob/rcc.c adc-measure.c

OBJ:= \
	$(patsubst %.c, $O/%.o, $(filter %.c, $(SRC))) \
	$(patsubst %.S, $O/%.o, $(filter %.S, $(SRC))) \
	$(patsubst %.xcf, $O/%.o, $(filter %.xcf, $(SRC)))

CFLAGS= -g3 -O0  -Wall -Ifrob -I.  -fstrict-aliasing -fstrict-volatile-bitfields  -mfloat-abi=softfp

include frob/f1/flags.mk

ASFLAGS:= -g3

LDFLAGS:=$(LDFLAGS)  \
	-L$(PATH_LIBNEW) \
	-L$(PATH_LIBGCC) \
	--start-group -lc -lnosys -lgcc --end-group

$O:
	@mkdir -p $O

f1-bin: $O/prg-f1.bin

$O/.aux: | $O
	mkdir -p $O/.aux


clean::
	find $O/.. -name '*.[do]' -exec rm '{}' \;
	find $O/.. -name 'prg-*' -exec rm '{}' \;

.PHONY: clean all chkpath f1 f1-bin

