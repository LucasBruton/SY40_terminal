OFILES = Main.o \
		Bateau.o \
		Portique.o \
		Train.o \
		Camion.o \

clean: 
	rm -f ${OFILES} Terminal

run: Terminal
	./Terminal 5 5 3 5

Terminal : ${OFILES}
	gcc -o Terminal ${OFILES} -lpthread

Main.o: Main.c Terminal.h
	gcc -o Main.o -c Main.c

Bateau.o: Bateau.c Terminal.h
	gcc -o Bateau.o -c Bateau.c

Portique.o: Portique.c Terminal.h
	gcc -o Portique.o -c Portique.c

Train.o: Train.c Terminal.h
	gcc -o Train.o -c Train.c
