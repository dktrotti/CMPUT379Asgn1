procnanny: procnanny.c
	gcc -Wall -DMEMWATCH -DMW_STDIO procnanny.client.c memwatch.c -o procnanny.client
	gcc -Wall -DMEMWATCH -DMW_STDIO procnanny.server.c memwatch.c -o procnanny.server

clean:
	rm procnanny.client
	rm procnanny.server