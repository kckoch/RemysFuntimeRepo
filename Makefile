BINS=robotClient robotServer Compression

all: $(BINS)

robotClient: RobotClient.cpp
	g++ -g -w -Wall RobotClient.cpp Compression.cpp -o robotClient -lm

robotServer: RobotServer.cpp
	g++ -g -w -Wall RobotServer.cpp Compression.cpp -o robotServer -lm
	
Compression: Compression.cpp
	g++ -g -Wall Compression.cpp -o Compression -lm

clean:
	rm $(BINS)
