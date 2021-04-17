notify: notify.c
	gcc -Wall notify.c -o notify
	#ar rcs $@ $^

listener: listener.c
	gcc -Wall listener.c -o listener

clean:
	rm -f notify listener
