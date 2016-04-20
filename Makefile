BINS=robotClient robotServer

all: $(BINS)

robotClient: RobotClient.cpp
	g++ -g -Wall RobotClient.cpp -o robotClient -lm

robotServer: RobotServer.cpp
	g++ -g -Wall RobotServer.cpp -o robotServer -lm

clean:
	rm $(BINS)
