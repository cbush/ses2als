main:
	clang++ -std=c++14 -o ses2als ses2als.cpp SessionFile.cpp

debug:
	clang++ -DLOG=1 -std=c++14 -o ses2als ses2als.cpp SessionFile.cpp
