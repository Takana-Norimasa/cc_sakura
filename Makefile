CFLAGS=-std=c11 -g -O0 -static

cc_sakura: cc_sakura.c

test: cc_sakura
	./test.sh

clean:
	rm -f cc_sakura *.o *~ tmp*

.PHONY: test clean
