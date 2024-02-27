mongoose:
	gcc -O2 -c mongoose.c -o mongoose

all:
	gcc -g mongoose main.c -o mdlv

