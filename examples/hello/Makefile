TARIK = $(shell find ../../*/tarik)

hello: hello.tk
	@$(TARIK) -o hello.o $^
	@gcc -o $@ hello.o

run: hello
	@./hello

clean:
	rm -f hello.o hello

.PHONY: clean
