SRCDIR ?= /opt/fpp/src
include $(SRCDIR)/makefiles/common/setup.mk
include $(SRCDIR)/makefiles/platform/*.mk

all: libfpp-edmrds.$(SHLIB_EXT)
debug: all

CFLAGS+=-I. -I./edmrds
OBJECTS_fpp_edmrds_so += src/FPPEDMRDS.o src/I2C_BitBang.o
LIBS_fpp_edmrds_so += -L$(SRCDIR) -lfpp -ljsoncpp
CXXFLAGS_src/FPPEDMRDS.o += -I$(SRCDIR)
CXXFLAGS_src/I2C_BitBang.o += -I$(SRCDIR)

%.o: %.cpp Makefile
	$(CCACHE) $(CC) $(CFLAGS) $(CXXFLAGS) $(CXXFLAGS_$@) -c $< -o $@

libfpp-edmrds.$(SHLIB_EXT): $(OBJECTS_fpp_edmrds_so) $(SRCDIR)/libfpp.$(SHLIB_EXT)
	$(CCACHE) $(CC) -shared $(CFLAGS_$@) $(OBJECTS_fpp_edmrds_so) $(LIBS_fpp_edmrds_so) $(LDFLAGS) -o $@

clean:
	rm -f libfpp-edmrds.$(SHLIB_EXT) $(OBJECTS_fpp_edmrds_so)
