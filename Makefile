.DEFAULT_GOAL := bin/run

bin/run: 
	@gcc src/run.c -o bin/run
	./bin/run

clean:
	$(RM) build/* bin/* 