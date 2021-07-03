.PHONY: clean

build:my_cat my_tac

run_cat:
	./my_cat $(file_name)

run_tac:
	./my_tac $(file_name)

my_cat:my_cat.c
	gcc my_cat.c -o my_cat

my_tac:my_tac.c
	gcc my_tac.c -o my_tac

clean:
	rm my_cat my_tac
