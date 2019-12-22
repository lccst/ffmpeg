
INCLUDES=-I/lccRoot/programs/x86_64/SDL-1.2.15/include/SDL \
		 -I/lccRoot/programs/x86_64/ffmpeg-3.1.11/include

LIBS=-L/lccRoot/programs/x86_64/SDL-1.2.15/lib -lSDL \
	 -L/lccRoot/programs/x86_64/ffmpeg-3.1.11/lib \
	 -lavutil -lavformat -lavcodec -lavutil -lm -g -lswscale

all:
	make clean
	gcc *.c -g -o test.out ${LIBS} ${INCLUDES}

clean:
	rm -rf *.out