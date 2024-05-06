#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <bits/stdc++.h>
#include <sqlite3.h>

using namespace std;

#define PORT 2025

struct Client
{
	string name;
	int socket;
};

vector<Client> clients;
mutex clientsMutex;

sqlite3* db;
sqlite3_stmt* stmt;
const char* selectQuery = "SELECT * FROM SafeZones;";

bool checkDataBase(int x, int y)
{
	int x_db, y_db, r_db;
	sqlite3_prepare_v2(db, selectQuery, -1, &stmt, nullptr);

    while (sqlite3_step(stmt) == SQLITE_ROW) 
	{
        x_db = sqlite3_column_double(stmt, 1);
        y_db = sqlite3_column_double(stmt, 2);
        r_db = sqlite3_column_double(stmt, 3);
		if ((x - x_db) * (x - x_db) + (y - y_db) * (y - y_db) <= r_db * r_db)
		{
			sqlite3_finalize(stmt);
			return 1;
		}
	}
	sqlite3_finalize(stmt);
	return 0;
}

void doSomething(int client, string name)
{
	char buffer[100], aux[100];
	int bufferLenght, x, y;
	bool safe;
	while (1)
	{
		bzero(&buffer, sizeof(buffer));
		if (bufferLenght = recv(client, buffer, sizeof(buffer), 0) <= 0)
		{
			cout << "[server]Receive error.\n";
			lock_guard<mutex> lock(clientsMutex);
			for (auto it=clients.begin(); it != clients.end(); )
				if (it->socket == client)
					it = clients.erase(it);
				else 
					it++;
			//exit(EXIT_FAILURE);
			return;
		}
		if (strcmp(buffer, "Alarm") == 0)
		{
			strcpy(aux, "Alarm from ");
			strcat(aux, name.c_str());
			strcat(aux, "\n");
			cout<<aux;
		}
		else /// am coordonate
		{
			sscanf(buffer, "%d %d", &x, &y); /// stiu la ce pozitie e client

			/// zona sigura
			safe = checkDataBase(x,y);

			strcat(buffer, " ");
			buffer[strlen(buffer)] = '0' + safe; /// x y safe

			cout << "Client " << name.c_str() << " is at " << buffer << '\n';

			strcpy(aux, name.c_str());
			strcat(aux, " ");
			strcat(aux, buffer);
			strcat(aux, "\n");
		}

		lock_guard<mutex> lock(clientsMutex);
		for (auto it : clients)
			if (it.socket != client)
				send(it.socket, aux, strlen(aux), 0);
	}
}

int main()
{
	struct sockaddr_in server; // structura folosita de server
	struct sockaddr_in from;
	char msg[100];			 // mesajul primit de la client
	char msgrasp[100] = " "; // mesaj de raspuns pentru client
	int sd;					 // descriptorul de socket

	if (sqlite3_open("database.db", &db)) 
    {
        cout << "[server]Database open error.\n";
        exit(EXIT_FAILURE);
    }
	else 
		cout << "[server]Database open successfully.\n"; 

	/* crearea unui socket */
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		cout << "[server]Socket error.\n";
		exit(EXIT_FAILURE);
	}

	int reusePort = 1;
	if (setsockopt(sd, SOL_SOCKET, SO_REUSEPORT, &reusePort, sizeof(reusePort)) < 0)
	{
		perror("setsockopt(SO_REUSEPORT) failed");
		return -1;
	}

	/* pregatirea structurilor de date */
	bzero(&server, sizeof(server));
	bzero(&from, sizeof(from));

	/* umplem structura folosita de server */
	/* stabilirea familiei de socket-uri */
	server.sin_family = AF_INET;
	/* acceptam orice adresa */
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	/* utilizam un port utilizator */
	server.sin_port = htons(PORT);

	/* atasam socketul */
	if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
	{
		cout << "[server]Bind error.\n";
		exit(EXIT_FAILURE);
	}

	/* punem serverul sa asculte daca vin clienti sa se conecteze */
	if (listen(sd, 10) == -1)
	{
		cout << "[server]Listen error.\n";
		exit(EXIT_FAILURE);
	}

	/* servim in mod concurent clientii... */
	while (1)
	{
		int client;
		socklen_t length = sizeof(from);

		cout << "[server]Wait on port " << PORT << "...\n";
		cout.flush();

		/* acceptam un client (stare blocanta pina la realizarea conexiunii) */
		client = accept(sd, (struct sockaddr *)&from, &length);

		/* eroare la acceptarea conexiunii de la un client */
		if (client < 0)
		{
			cout << "[server]Accept error.\n";
			exit(EXIT_FAILURE);
		}

		char buffer[1024];
		int bufferLength = recv(client, buffer, sizeof(buffer), 0);
		Client newClient;
		newClient.name.assign(buffer);
		newClient.socket = client;

		lock_guard<mutex> lock(clientsMutex);
		clients.push_back(newClient);

		bzero(&buffer, sizeof(buffer));  //trimit si zonele sigure
		int x_db, y_db, r_db;
		sqlite3_prepare_v2(db, selectQuery, -1, &stmt, nullptr);

    	while (sqlite3_step(stmt) == SQLITE_ROW) 
		{
       		x_db = sqlite3_column_double(stmt, 1);
        	y_db = sqlite3_column_double(stmt, 2);
        	r_db = sqlite3_column_double(stmt, 3);
			strcat(buffer, to_string(x_db).c_str());
			strcat(buffer," ");
			strcat(buffer, to_string(y_db).c_str());
			strcat(buffer," ");
			strcat(buffer, to_string(r_db).c_str());
			strcat(buffer," ");
		}
		sqlite3_finalize(stmt);

		send(client, buffer, sizeof(buffer),0);

		thread(doSomething, newClient.socket, newClient.name).detach();
	}

	sqlite3_close(db);
	return 0;
} /* main */