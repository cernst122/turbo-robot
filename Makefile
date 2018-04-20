all: reliable_sender reliable_receiver

reliable_sender:
	gcc -o reliable_sender sender_main.c -g

reliable_receiver:
	gcc -pthread -o reliable_receiver receiver_main.c -g

.PHONY: clean
clean:
	@rm -f reliable_sender reliable_receiver *.o
