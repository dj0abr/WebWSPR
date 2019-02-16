# makefile for WSPR Toolkit

CC = gcc
CFLAGS = -Wall -O3 -Wno-write-strings -Wno-narrowing -Wno-unused-result
LDFLAGS = -lpthread -lrt -lsndfile -lasound -lgd -lz -ljpeg -lfreetype -lz
LIBS = -lfftw3 -lm
OBJ = wsprtk.o soundcard.o debug.o decode.o upload.o JSON.o waterfall.o fft.o color.o config.o kmtools.o coder.o cat.o mysql.o hopping.o status.o civ.o spectrum.o dds.o crc.o JSON_2way.o hamlib.o spectrum_big.o mytime.o

# Default rules
%.o: %.c $(DEPS)
	${CC} ${CFLAGS} -c $<

all:    wsprtk

wsprtk: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS)

clean:
	rm -f *.o wsprtk
