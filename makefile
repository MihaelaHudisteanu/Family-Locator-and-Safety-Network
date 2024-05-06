all:
	g++ server.cpp -o serv -lsqlite3
	g++ -c client2.cpp
    g++ client2.o -o sfml-app -lsfml-graphics -lsfml-window -lsfml-system
	g++ client2.o -o app -lsfml-graphics -lsfml-window -lsfml-system
clean: 
	rm -f cli serv
