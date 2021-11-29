TARIK = $(shell find ../../*/tarik)

fib: fib.tk
	@$(TARIK) -o fib.o $^
	@gcc -o $@ fib.o

run: fib
	@./fib

clean:
	rm -f fib.o fib

.PHONY: clean
