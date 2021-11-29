TARIK = $(shell find ../../*/tarik)

arguments: arguments.tk
	@$(TARIK) -o arguments.o $^
	@gcc -o $@ arguments.o

run: arguments
	@./arguments

clean:
	rm -f arguments.o arguments

.PHONY: clean
