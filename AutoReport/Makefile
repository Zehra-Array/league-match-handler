CXXFLAGS = -fPIC -frtti -I../../include -O
LDLIBS = -ldl
all : AutoReport.so
AutoReport.o : AutoReport.cpp
		$(CXX) $(CXXFLAGS) -c -o $@  $^ 
AutoReport.so : AutoReport.o
		$(CXX) $(CXXFLAGS) -shared -rdynamic $^ -o $@
clean :
		-rm *.so *.o
