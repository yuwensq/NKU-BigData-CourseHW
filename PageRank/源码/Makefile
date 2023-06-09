cc = g++
prom = pagerank
deps = $(shell find ./ -name "*.h")
src = $(shell find ./ -name "*.cpp")
obj = $(src:%.cpp=%.o)

FLAG = -lpthread -std=c++17

$(prom): $(obj)
	@ $(cc) -o $(prom) $(obj) $(FLAG)

%.o: %.cpp $(deps)
	@ $(cc) -c $< -o $@ $(FLAG)

run: $(prom)
	@ ./$(prom)


clean:
	@ rm -rf $(obj) $(prom)