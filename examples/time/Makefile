TARIK = $(shell find ../../*/tarik)

time: time.tk
	@$(TARIK) -o time.o $^
	@gcc -o $@ time.o

run: time
	@./time

clean:
	rm -f time.o time

.PHONY: clean
