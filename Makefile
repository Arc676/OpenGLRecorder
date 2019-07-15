CC=gcc
FLAGS=-std=c11

LIBOUT=libopenglrecorder.a
EXECOUT=recorderdemo

ifdef DEBUG
FLAGS+=-g -O0
endif

ifdef RELEASE
FLAGS+=-O2
endif

INCLUDE=-I src -L .
CFLAGS=$(FLAGS) $(INCLUDE)
LIB=-l openglrecorder -l GL -l GLU -l glut -l avcodec -l swscale -l avutil

ODIR=obj
SDIR=src

OBJS=glrecorder.o
_OBJS=$(patsubst %, $(ODIR)/%, $(OBJS))

test: lib
	$(CC) $(CFLAGS) tests/test.c $(LIB) -o tests/$(EXECOUT)

lib: makeodir $(_OBJS)
	ar rcs $(LIBOUT) $(_OBJS)

makeodir:
	mkdir -p $(ODIR)

$(ODIR)/%.o: $(SDIR)/%.c
	$(CC) -c $(FLAGS) -o $@ $<

clean:
	rm -f $(LIBOUT) $(EXECOUT) $(ODIR)/*.o
