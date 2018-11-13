main:
	mkdir -p bin
	clang++ -std=c++14 -o bin/ses2als ses2als.cpp SessionFile.cpp

debug:
	mkdir -p bin
	clang++ -DLOG=1 -std=c++14 -o bin/ses2als ses2als.cpp SessionFile.cpp
