#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <bits/stdc++.h>
#include <arpa/inet.h>
#include <SFML/Graphics.hpp>

using namespace std;
using namespace sf;

#define X 800
#define Y 600

/* portul de conectare la server*/
int port;
int x, y;
string name, alarmUser;
int alarmReceived;
mutex socketMutex;

struct SafeZone
{
    int x,y,r;
};
vector<SafeZone> safeZones;

struct User
{
    string name;
    int x,y,safe;
};
vector<User> users;


void sendCoord(int sd)
{
    char buffer[100];
    string aux;
    while (1)
    {
        /// daca vreau sa ramana mai mult intr-un loc pot: this_thread::sleep_for(chrono::seconds(rand()%100));
        x += 10*(rand() % 3 - 1);
        x = max(30, x);
        x = min(X-10, x);
        y += 10*(rand() % 3 - 1);
        y = max(30, y);
        y = min(Y-10, y);

        bzero(&buffer, sizeof(buffer));
        aux = to_string(x);
        strcpy(buffer, aux.c_str());
        strcat(buffer, " ");
        aux = to_string(y);
        strcat(buffer, aux.c_str());

        lock_guard<mutex> lock(socketMutex);
        if (send(sd, buffer, strlen(buffer), 0) <= 0)
        {
            cout << "[client]Coordinates error.\n";
            exit(EXIT_FAILURE);
        }
        this_thread::sleep_for(chrono::seconds(3));
    }
}

void receive(int sd)
{
    char buffer[100];
    string receivedName;
    int bufferLenght, receivedX, receivedY,receivedSafe;
    while (1)
    {
        bzero(&buffer, sizeof(buffer));
        if (bufferLenght = recv(sd, buffer, sizeof(buffer), 0) <= 0)
        {
            cout << "[client]Receive error.\n";
            exit(EXIT_FAILURE);
        }
        
        if(strstr(buffer, "Alarm"))
        {
            buffer[strlen(buffer)] = 0; //scap de /n
            alarmUser.assign(buffer);
            alarmReceived = 3;
            continue;
        }
        
        //sscanf(buffer, "%s %d %d %d", receivedName, &receivedX, &receivedY, &receivedSafe);
        istringstream iss(buffer);
        iss >> receivedName >> receivedX >> receivedY >> receivedSafe;
        
        //caut clientul
        bool found=0;
        for(auto &it : users)
            if(receivedName == it.name)
            {
                found = 1;
                it.x = receivedX;
                it.y = receivedY;
                it.safe = receivedSafe;
            }
        if(!found)
        {
            User newUser;
            newUser.name = receivedName;
            newUser.x = receivedX;
            newUser.y = receivedY;
            newUser.safe = receivedSafe;
            users.push_back(newUser);
        }
    }
}

void drawMap(RenderWindow& window)
{
    const int gridSize = 20;
    const int rows = window.getSize().y / gridSize;
    const int cols = window.getSize().x / gridSize;

    RectangleShape cell(Vector2f(gridSize, gridSize));
    cell.setOutlineThickness(1.0f);
    cell.setOutlineColor(Color(100,122,135));

    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < cols; j++)
        {
            cell.setPosition(j * gridSize, i * gridSize);
            cell.setFillColor(Color(33,60,77));
            window.draw(cell);
        }
    }

    for(auto it : safeZones)
    {
        CircleShape circle(it.r);
        circle.setFillColor(Color(14, 138, 59, 128));  // semi-transparent red (128 alpha)
        circle.setPosition(it.x - it.r, it.y - it.r);
        window.draw(circle);
    }
}

void drawUserPoints(RenderWindow& window, char letter, int userX, int userY, int safe)
{
    CircleShape userPoint(15);
    if(safe == 1)
        userPoint.setFillColor(Color(163,179,204));
    else if (safe == 0)
        userPoint.setFillColor(Color(173,49,45));
    else 
        userPoint.setFillColor(Color(8,28,41));
    userPoint.setPosition(userX - 15, userY - 15);

    ///as putea sa fac asta:
    Font font;
    if (!font.loadFromFile("font.ttf")) 
        exit(EXIT_FAILURE);
    Text text(letter, font, 20); 
    text.setFillColor(Color::White);
    text.setPosition(userX-7, userY-11); 

    window.draw(userPoint);
    window.draw(text);
}

void drawAlarmButton(RenderWindow& window)
{
    RectangleShape alarmButton(Vector2f(100, 40));
    alarmButton.setPosition(X - 110, Y - 50); 
    alarmButton.setFillColor(Color::Red);

    Font font;
    if (!font.loadFromFile("font.ttf")) 
        exit(EXIT_FAILURE);

    Text buttonText("Alarm", font, 20);
    buttonText.setPosition(alarmButton.getPosition().x + 20, alarmButton.getPosition().y + 10);
    buttonText.setFillColor(Color::White); ///

    window.draw(alarmButton);
    window.draw(buttonText);
}

void drawAlarm(RenderWindow& window)
{
    Font font;
    if (!font.loadFromFile("font.ttf")) 
        exit(EXIT_FAILURE);
    Text text(alarmUser, font, 20);
    FloatRect textBounds = text.getGlobalBounds();

    RectangleShape alarmMsg(Vector2f(textBounds.width + 20, textBounds.height + 20));
    alarmMsg.setPosition((X - alarmMsg.getSize().x) / 2, (Y - alarmMsg.getSize().y) / 2); 
    alarmMsg.setFillColor(Color::Black);

    text.setPosition(alarmMsg.getPosition().x + 10, alarmMsg.getPosition().y + 10);
    text.setFillColor(Color::White);

    window.draw(alarmMsg);
    window.draw(text);
}

void draw(RenderWindow& window)
{
    while(1)
    {
        window.clear();
        drawMap(window);
        drawUserPoints(window, ' ',x, y, -1);
        for(auto it: users)
            drawUserPoints(window, it.name[0], it.x, it.y, it.safe);
        
        drawAlarmButton(window);

        if(alarmReceived)
        {
            alarmReceived--;
            drawAlarm(window);
        }

        window.display();
        this_thread::sleep_for(chrono::seconds(1));
    }
}

void getName()
{
    RenderWindow inputWindow(VideoMode(X, Y), "Family Locator and Safety Network", Style::Titlebar | Style::Close);
    inputWindow.setFramerateLimit(30);

    Font font;
    if (!font.loadFromFile("font.ttf")) 
        exit(EXIT_FAILURE);

    Text prompt("Enter your name:", font, 20);
    prompt.setPosition(X/2-120, Y/2-40);

    RectangleShape inputBox(Vector2f(200, 30));
    inputBox.setPosition(X/2-120, Y/2);
    inputBox.setFillColor(Color::White);
    inputBox.setOutlineThickness(2);
    inputBox.setOutlineColor(Color::Black);

    Text userInput("", font, 20);
    userInput.setPosition(X/2-115, Y/2);
    userInput.setFillColor(Color::Black);

    bool done = 0;

    while (inputWindow.isOpen()) 
    {
        Event event;
        while (inputWindow.pollEvent(event)) 
        {
            if (event.type == Event::Closed) 
            {
                inputWindow.close();
                exit(EXIT_SUCCESS);
            }
            else if (event.type == Event::TextEntered) 
            {
                if (event.text.unicode < 128 && event.text.unicode != 8) // backspace
                    userInput.setString(userInput.getString() + (char)event.text.unicode);
                else if (event.text.unicode == 8 && !userInput.getString().isEmpty()) 
                    userInput.setString(userInput.getString().substring(0, userInput.getString().getSize() - 1));

            }
            else if (event.type == Event::KeyPressed && event.key.code == Keyboard::Return) 
            {
                name = userInput.getString();
                done = 1;
            }
        }

        inputWindow.clear(Color(33,60,77));
        inputWindow.draw(prompt);
        inputWindow.draw(inputBox);
        inputWindow.draw(userInput);
        inputWindow.display();

        if (done)
            inputWindow.close();
    }
}


int main(int argc, char *argv[])
{
    srand(time(0));
    int sd;                    // descriptorul de socket
    struct sockaddr_in server; // structura folosita pentru conectare

    /* exista toate argumentele in linia de comanda? */
    if (argc != 3)
    {
        cout << "[client]Sintax error.\n";
        exit(EXIT_FAILURE);
    }

    /* stabilim portul */
    port = atoi(argv[2]);

    /* cream socketul */
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        cout << "[client]Socket error.\n";
        exit(EXIT_FAILURE);
    }

    /* umplem structura folosita pentru realizarea conexiunii cu serverul */
    /* familia socket-ului */
    server.sin_family = AF_INET;
    /* adresa IP a serverului */
    server.sin_addr.s_addr = inet_addr(argv[1]);
    /* portul de conectare */
    server.sin_port = htons(port);

    /* ne conectam la server */
    if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        cout << "[client]Connect error.\n";
        exit(EXIT_FAILURE);
    }

    getName();
    /* trimiterea mesajului */
    if (send(sd, name.c_str(), 100, 0) <= 0)
    {
        cout << "[client]Name error.\n";
        exit(EXIT_FAILURE);
    }
    cout << "Connected to the server\n";

    char buffer[1024];
    int x_buffer, y_buffer, r_buffer;
    recv(sd, buffer, sizeof(buffer), 0);
    istringstream iss(buffer);
    while(iss >> x_buffer >> y_buffer >> r_buffer)
    {
        SafeZone newSafeZone;
        newSafeZone.x = x_buffer;
        newSafeZone.y = y_buffer;
        newSafeZone.r = r_buffer;
        safeZones.push_back(newSafeZone);
        //cout<<x_buffer<<' '<<y_buffer<<' '<<r_buffer<<'\n';
    }

    RenderWindow window(sf::VideoMode(X, Y), "Family Locator and Safety Network", Style::Titlebar | Style::Close);

    x = rand() % X;
    y = rand() % Y;

    /// acum incep sa trimit coordonate, alarme si sa primesc si eu coordonate, alarme

    thread(sendCoord, sd).detach(); // trimit coordonate

    thread(receive, sd).detach(); // primesc notificari
    
    thread(draw, ref(window)).detach();

    int x1 = X-110, x2 = X-10, y1 = Y-50, y2 = Y-10; //coordonatele butonului de alarma
    bzero(&buffer, sizeof(buffer)); /// pentru trimis alarma
    strcpy(buffer, "Alarm");
    while (window.isOpen())
    {
        Event event;
        while (window.pollEvent(event))
        {
            if (event.type == Event::Closed)
                window.close();
            else if (event.type == Event::MouseButtonPressed)  //a fost apasat butonul de alarma
            {
                Vector2f mousePos = window.mapPixelToCoords(Mouse::getPosition(window));
                if (mousePos.x>=x1 && mousePos.x<=x2 && mousePos.y>=y1 && mousePos.y<=y2)
                    send(sd, buffer, strlen(buffer), 0);
            }
        }
    }

    return 0;
}