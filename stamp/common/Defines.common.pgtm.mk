# ==============================================================================
#
# Defines.common.pgtm.mk
#
# ==============================================================================


CC       := g++
CFLAGS   += -g -Wall -pthread
CFLAGS   += -O3
CFLAGS   += -I$(LIB)
CPP      := g++
CPPFLAGS += $(CFLAGS)
LD       := g++
LIBS     += -lpthread

# Remove these files when doing clean
OUTPUT +=

LIB := ../lib

STM := ../../pgtm

LOSTM := ../../OpenTM/lostm


# ==============================================================================
#
# End of Defines.pgtm.common.mk
#
# ==============================================================================
