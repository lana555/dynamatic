CXX=g++
CXXFLAGS=-std=c++11 -ggdb

MKDIR=mkdir
CP=cp

DEPS = CAnalyser.h CInjector.h Help.h HlsCoVerification.h HlsCVerification.h HlsLogging.h HlsVhdlTb.h HlsVhdlVerification.h Utilities.h VerificationContext.h
OBJS = CAnalyser.o CInjector.o Help.o HlsCoVerification.o HlsCVerification.o HlsLogging.o HlsVhdlTb.o HlsVhdlVerification.o Utilities.o VerificationContext.o HlsVerifier.o

%.o: %.cpp $(DEPS)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

hlsverifier: $(OBJS)
	$(MKDIR) -p build
	$(CXX) -o build/hlsverifier $^ $(CXXFLAGS) -lboost_regex
	$(CP) -R resources build/

clean:
	rm -f *.o
	rm -rf build

