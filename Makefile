OBJECTS = MiscUtil.o Packet.o PerfInfo.o main.o JsonParser.o RestClient.o
DEFINES = -D_REENTRANT -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64

INCLUDES += -I. -I./inc

inno_client : $(OBJECTS)
	g++ -Wall -m64 -g -o inno_client $(DEFINES) $(OBJECTS) -lpthread -lcurl -L/usr/lib64 -L/usr/local/lib -Xlinker --start-group /usr/local/lib/libjson.a -Xlinker --end-group
	rm $(OBJECTS)

MiscUtil.o : MiscUtil.cpp
	g++ -Wall -m64 -g -c $(DEFINES) MiscUtil.cpp

Packet.o : Packet.cpp
	g++ -Wall -m64 -g -c $(DEFINES) Packet.cpp

PerfInfo.o : PerfInfo.cpp
	g++ -Wall -m64 -g -c $(DEFINES) PerfInfo.cpp

JsonParser.o : JsonParser.cpp
	g++ -Wall -m64 -g -c $(DEFINES) $(INCLUDES) JsonParser.cpp

RestClient.o : RestClient.cpp
	g++ -Wall -g -c $(DEFINES) $(INCLUDES) RestClient.cpp

#main.o : main.cpp main_report.cpp main_clean.cpp
main.o : main.cpp main_report.cpp main_alive.cpp main_clean.cpp
	g++ -Wall -m64 -g -c $(DEFINES) main.cpp

clean:
	rm $(OBJECTS)

cscope:
	cscope -bRu -I /usr/include -I
