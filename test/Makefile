OBJECTS = JsonParser.o main.o RestClient.o
DEFINES = -D_REENTRANT -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
INCLUDES += -I. -I./inc

INNOLIB = INNOLIB/lib

CLIBS = -L. -L$(INNOLIB) -linno-src

#STATIC = $(INNOLIB)/librestclient.a

#$(STATIC) : RestClient.cpp

TESTAPPL : $(OBJECTS)
	g++ -Wall -g -o testappl $(OBJECTS) $(DEFINES) -lpthread -lcurl -L/usr/lib64/ $(CLIBS) -Xlinker --start-group ./lib/libjson.a -Xlinker --end-group
	rm $(OBJECTS)

#LogUtil.o : LogUtil.cpp
#	g++ -Wall -g -c $(DEFINES) $(INCLUDES) LogUtil.cpp

#StringBuffer.o : StringBuffer.cpp
#	g++ -Wall -g -c $(DEFINES) $(INCLUDES) StringBuffer.cpp

JsonParser.o : JsonParser.cpp
	g++ -Wall -m64 -g -c $(DEFINES) $(INCLUDES) JsonParser.cpp

RestClient.o : RestClient.cpp
	g++ -Wall -g -c $(DEFINES) $(INCLUDES) RestClient.cpp

main.o : main.cpp
	g++ -Wall -m64 -g -c $(DEFINES) main.cpp

clean:
	rm $(OBJECTS)
