CXX = clang++
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra

# Metal specific settings
METAL_INCLUDES = -I./metal-cpp
METAL_FRAMEWORKS = -framework Metal -framework Foundation -framework QuartzCore

# Main targets
all: metal_test

metal_test: ndarray_backend_metal.o
	$(CXX) $(CXXFLAGS) $^ $(METAL_FRAMEWORKS) -o $@

ndarray_backend_metal.o: ndarray_backend_metal.cpp
	$(CXX) $(CXXFLAGS) $(METAL_INCLUDES) $(METAL_FRAMEWORKS) -c $< -o $@

clean:
	rm -f *.o metal_test
