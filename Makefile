main: main.cc nanocube.cc nanocube.hh nanocube.inc
	clang++ -g -std=c++11 main.cc nanocube.cc -o main -Wall

clean:
	rm main

