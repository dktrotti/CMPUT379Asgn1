procnanny: procnanny.c
	gcc -Wall -DMEMWATCH -DMW_STDIO procnanny.c memwatch.c -o procnanny

clean:
	rm procnanny