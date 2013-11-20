BIN_NAME=tum

all: main.o Array.o UniversalMachine.o Exceptions.o
	g++ $^ -o $(BIN_NAME)

.o: .cpp .h
	g++ -c $^

clean:
	rm -f *.o *~ tum
