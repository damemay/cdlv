mongoose:
	gcc -O2 -c mongoose.c -o mongoose

all:
	gcc -O2 mongoose main.c -o mdlv

