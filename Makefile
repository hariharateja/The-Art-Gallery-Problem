CXX     = g++
CXXFLAGS= -std=c++17 -O2

main: geometry.cpp dcel.cpp sweep.cpp triangulate.cpp main.cpp
	$(CXX) $(CXXFLAGS) -o main geometry.cpp dcel.cpp sweep.cpp triangulate.cpp main.cpp

clean:
	rm -f main out
