cc = g++
prom = SVD
deps = $(shell find ./ -name "*.h")
src = $(shell find ./ -name "*.cpp")
obj = $(src:%.cpp=%.o)

FLAG = -fopenmp -lpthread -std=c++17 -O2

$(prom): $(obj)
	@ $(cc) -o $(prom) $(obj) $(FLAG)

%.o: %.cpp $(deps)
	@ $(cc) -c $< -o $@ $(FLAG)

run: $(prom)
	@ ./$(prom)

clean:
	@ rm -rf $(obj) $(prom) 