CXX = g++
WASMEDGE ?= $(HOME)/.local/wasmedge
CXXFLAGS = -I$(WASMEDGE)/include
LDFLAGS = -L$(WASMEDGE)/lib64 -lwasmedge -Wl,-rpath,$(WASMEDGE)/lib64

run_fib: run_fib.cpp
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

clean:
	rm -f run_fib
