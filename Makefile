includes	=	flappy.h sysdefs.h bird.h pipes.h input.h banner.h
includes	+=	scoreboard.h sound.h
sources		=	flappy.c bird.c pipes.c input.c banner.c
sources		+=	scoreboard.c sound.c
resources	=	BIRD.BIN BACKGROUND.BIN TILES.BIN BANNERS.BIN
target		=	FLAPPY.PRG
pkg			=	flappyx16.zip

.PHONY: all clean run pkg

all: $(target) $(includes)

FLAPPY.PRG: flappy.c $(includes)
	cl65 -t cx16 -O -o $@ $(sources)

install:
	cp FLAPPY.PRG ~/x16/

clean:
	rm -f *.o *.s *.PRG

run: all
	x16emu -prg FLAPPY.PRG -run -joy1 SNES

test: test.c
	cl65 -t cx16 -O -o TEST test.c
	x16emu -prg TEST -debug
	
pkg: $(pkg)

$(pkg): $(target) $(resources)
	zip $@ $(target) $(resources) 
