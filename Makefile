main: main.cc nanocube.h nanocube.inc
	clang++ -g -std=c++11 main.cc -o main -Wall

clean:
	rm main

