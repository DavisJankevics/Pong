all: server.exe client.exe

server.exe: headers.o network.o server.c
	gcc server.c headers.o network.o -std=gnu90 -Wall -Wextra -fno-common -lglut -lGL -lGLU -lm -o server.exe

client.exe: headers.o network.o client.c
	gcc client.c headers.o network.o -std=gnu90 -Wall -Wextra -fno-common -lglut -lGL -lGLU -lm -o client.exe

headers.o: headers.c
	gcc -o headers.o -c headers.c -std=gnu90 -Wall -Wextra -fno-common -lglut -lGL -lGLU-lm

network.o: network.c
	gcc -o network.o -c network.c -std=gnu90 -Wall -Wextra -fno-common -lglut -lGL -lGLU -lm

test_server: server.exe
	@echo "============ SERVER: =============="
	./server.exe -p=65199
	@echo "==================================="

test_client: client.exe
	@echo "============ CLIENT: =============="
	./client.exe -h=127.0.0.1 -p=65199
	@echo "==================================="


run_test: test_server clean

clean:
	rm headers.o
	rm network.o
	rm server.exe
	rm client.exe

# .INTERMEDIATE: headers.o network.o server.exe client.exe
