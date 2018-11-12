main:
	clang++ -std=c++14 -o ReadSession ReadSession.cpp SessionFile.cpp

debug:
	clang++ -DLOG=1 -std=c++14 -o ReadSession ReadSession.cpp SessionFile.cpp
