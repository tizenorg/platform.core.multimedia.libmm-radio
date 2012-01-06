ifeq ($(ARCH),target)
	PKG_CONFIG_PATH=/usr/lib/pkgconfig
	export PKG_CONFIG_PATH
	CC=arm-linux-gcc -Wall
	CXX=arm-linux-g++ -Wall
else
	PKG_CONFIG_PATH=/usr/lib/pkgconfig
	export PKG_CONFIG_PATH
	CC=gcc -Wall
    CXX=gcc -Wall
endif

TS1 = utc_mm_radio_create_func
TS2 = utc_mm_radio_destroy_func
TS3 = utc_mm_radio_realize_func
TS4 = utc_mm_radio_unrealize_func
TS5 = utc_mm_radio_set_message_callback_func
TS6 = utc_mm_radio_get_state_func
TS7 = utc_mm_radio_start_func
TS8 = utc_mm_radio_stop_func
TS9 = utc_mm_radio_seek_func
TS10 = utc_mm_radio_set_frequency_func
TS11 = utc_mm_radio_get_frequency_func
TS12 = utc_mm_radio_scan_start_func
TS13 = utc_mm_radio_scan_stop_func
TS14 = utc_mm_radio_set_sound_path_func
TS15 = utc_mm_radio_get_sound_path_func

LIBS = `pkg-config --libs mm-radio`
LIBS +=-L/usr/lib/:/usr/lib/pkgconfig
LIBS +=$(TET_ROOT)/lib/tet3/tcm_s.o
LIBS +=-L$(TET_ROOT)/lib/tet3 -ltcm_s
LIBS +=-L$(TET_ROOT)/lib/tet3/ -lapi_s

INCS = -I. `pkg-config --cflags  mm-radio `
INCS += -I$(TET_ROOT)/inc/tet3
INCS += -I/usr/include/mmf

CFLAGS = $(INCS) 
CC += $(CFLAGS)
LDFLAGS = $(LIBS)

all : $(TS1) $(TS2) $(TS3) $(TS4) $(TS5) $(TS6) $(TS7) $(TS8) $(TS9) $(TS10) $(TS11) $(TS12) $(TS13) $(TS14) $(TS15)

$(TS1): $(TS1).c
	$(CC) -o $(TS1) $(TS1).c $(LDFLAGS)

$(TS2): $(TS2).c
	$(CC) -o $(TS2) $(TS2).c $(LDFLAGS)
	
$(TS3): $(TS3).c
	$(CC) -o $(TS3) $(TS3).c $(LDFLAGS)

$(TS4): $(TS4).c
	$(CC) -o $(TS4) $(TS4).c $(LDFLAGS)

$(TS5): $(TS5).c
	$(CC) -o $(TS5) $(TS5).c $(LDFLAGS)
	
$(TS6): $(TS6).c
	$(CC) -o $(TS6) $(TS6).c $(LDFLAGS)

$(TS7): $(TS7).c
	$(CC) -o $(TS7) $(TS7).c $(LDFLAGS)

$(TS8): $(TS8).c
	$(CC) -o $(TS8) $(TS8).c $(LDFLAGS)

$(TS9): $(TS9).c
	$(CC) -o $(TS9) $(TS9).c $(LDFLAGS)

$(TS10): $(TS10).c
	$(CC) -o $(TS10) $(TS10).c $(LDFLAGS)

$(TS11): $(TS11).c
	$(CC) -o $(TS11) $(TS11).c $(LDFLAGS)

$(TS12): $(TS12).c
	$(CC) -o $(TS12) $(TS12).c $(LDFLAGS)

$(TS13): $(TS13).c
	$(CC) -o $(TS13) $(TS13).c $(LDFLAGS)

$(TS14): $(TS14).c
	$(CC) -o $(TS14) $(TS14).c $(LDFLAGS)

$(TS15): $(TS15).c
	$(CC) -o $(TS15) $(TS15).c $(LDFLAGS)

clean:
	rm -rf *~ *.o $(TS1) $(TS2) $(TS3) $(TS4) $(TS5) $(TS6) $(TS7) $(TS8) $(TS9) $(TS10) $(TS11) $(TS12) $(TS13) $(TS14) $(TS15)
	


