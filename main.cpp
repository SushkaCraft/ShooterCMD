#include <iostream>
#include <vector>
#include <random>
#include <conio.h>
#include <windows.h>
#include "map_data.h"

const int HEIGHT = 32;
const int WIDTH = 64;

struct Position {
    int x, y;

    bool operator==(const Position& other) const {
        return x == other.x && y == other.y;
    }

    Position operator+(const Position& other) const {
        return {x + other.x, y + other.y};
    }

    Position operator-(const Position& other) const {
        return {x - other.x, y - other.y};
    }
};

class Player {
private:
    Position position;
    int lives = 2;
    Position lastMove = {0, -1};

public:
    void move(int dx, int dy) {
        lastMove.x = dx;
        lastMove.y = dy;
        position.x += dx;
        position.y += dy;
    }

    char getDirectionSymbol() const {
        if (lastMove.x == 1 && lastMove.y == 0) return '>';
        if (lastMove.x == -1 && lastMove.y == 0) return '<';
        if (lastMove.y == 1 && lastMove.x == 0) return 'V';
        return '^'; 
    }

    Position getPositionInFront() const {
        Position frontPosition = {position.x + lastMove.x, position.y + lastMove.y};
        return frontPosition;
    }

    void increaseLives(int value) {
        lives = std::min(lives + value, 2);
    }

    void decreaseLives(int value) {
        lives = std::max(lives - value, 0);
    }

    int getLives() const {
        return lives;
    }

    Position getPosition() const {
        return position;
    }

    void setPosition(Position newPos) {
        position = newPos;
    }
};

class Bullet {
public:
    Position position;
    Position direction;
    bool isActive = false;

    void move() {
        if (isActive) {
            position = position + direction;
        }
    }
};

class Game {
private:
    std::vector<std::vector<char>> map;
    Player player;
    Bullet bullet;
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    std::mt19937 rng; 
    std::string playerName = "soldier_";

    void setColor(int color) {
        SetConsoleTextAttribute(console, color);
    }

    void resetCursorPosition() {
        COORD topLeft = {0, 0};
        SetConsoleCursorPosition(console, topLeft);
    }
    
    void setCursorVisibility(bool isVisible) {
	    CONSOLE_CURSOR_INFO cursorInfo;
	    GetConsoleCursorInfo(console, &cursorInfo);
	    cursorInfo.bVisible = isVisible; // FALSE для скрытия, TRUE для отображения
	    SetConsoleCursorInfo(console, &cursorInfo);
	}

    void displayStatus() {
        std::cout << "Health: " << player.getLives() << "                                          " << "Last move: " << player.getDirectionSymbol() << std::endl << "Name: " << playerName << std::endl;
    }

    void updatePlayerAppearance() {
        map[player.getPosition().y][player.getPosition().x] = '@';
        if (player.getLives() == 2) setColor(10); 
        else if (player.getLives() == 1) setColor(12); 
        else if (player.getLives() <= 0) {
            std::cout << "Game Over!" << std::endl;
            exit(0);
        }
    }

    void setConsoleSize(int width, int height) {
        COORD coord = {static_cast<SHORT>(width), static_cast<SHORT>(height)};
        SMALL_RECT Rect = {0, 0, static_cast<SHORT>(width - 1), static_cast<SHORT>(height + 1)};
        SetConsoleScreenBufferSize(console, coord);
        SetConsoleWindowInfo(console, TRUE, &Rect);
    }

    void initializeMap() {
        map.assign(HEIGHT, std::vector<char>(WIDTH, ' '));
        for (int i = 0; i < HEIGHT; ++i) {
            for (int j = 0; j < WIDTH; ++j) {
                if (i == 0 || i == HEIGHT - 1) map[i][j] = '-';
                else if (j == 0 || j == WIDTH - 1) map[i][j] = '|';
            }
        }
        loadMapData(map);
    }

    void placePlayer() {
        std::uniform_int_distribution<int> distX(1, WIDTH - 2), distY(1, HEIGHT - 2);
        while (true) {
            Position newPos = {distX(rng), distY(rng)};
            if (map[newPos.y][newPos.x] == ' ') {
                player.setPosition(newPos);
                updatePlayerAppearance();
                break;
            }
        }
    }

    void movePlayer(int dx, int dy) {
        Position newPos = player.getPosition();
        newPos.x += dx;
        newPos.y += dy;
        if (newPos.y >= 1 && newPos.y < HEIGHT - 1 && newPos.x >= 1 && newPos.x < WIDTH - 1 && 
            (map[newPos.y][newPos.x] == ' ' || map[newPos.y][newPos.x] == '+' || map[newPos.y][newPos.x] == '%')) {
            map[player.getPosition().y][player.getPosition().x] = ' ';
            player.move(dx, dy);
            if (map[newPos.y][newPos.x] == '+') player.increaseLives(1);
            else if (map[newPos.y][newPos.x] == '%') player.decreaseLives(1);
            updatePlayerAppearance();
        }
    }

	void shoot() {
	    if (!bullet.isActive) {
	        Position frontPosition = player.getPositionInFront();
	        // Проверяем, свободна ли позиция перед игроком и не является ли стеной или другим объектом
	        if (frontPosition.x > 0 && frontPosition.x < WIDTH - 1 && frontPosition.y > 0 && frontPosition.y < HEIGHT - 1 && map[frontPosition.y][frontPosition.x] == ' ') {
	            bullet.position = frontPosition;
	            bullet.direction = player.getPositionInFront() - player.getPosition();
	            bullet.isActive = true;
	        }
	    }
	}

	void updateBullet() {
	    if (bullet.isActive) {
	        Position nextPosition = bullet.position + bullet.direction;
	        // Проверяем, не выходит ли следующая позиция пули за пределы карты
	        if (nextPosition.x > 0 && nextPosition.x < WIDTH - 1 && nextPosition.y > 0 && nextPosition.y < HEIGHT - 1) {
	            char nextCell = map[nextPosition.y][nextPosition.x];
	            // Если следующая позиция пуста, пуля продолжает движение
	            if (nextCell == ' ') {
	                map[bullet.position.y][bullet.position.x] = ' '; // Очищаем текущую позицию пули
	                bullet.move();
	                map[bullet.position.y][bullet.position.x] = '*'; // Отмечаем новую позицию пули
	            } else {
	                // В противном случае (если пуля столкнулась с объектом), удаляем пулю
	                map[bullet.position.y][bullet.position.x] = ' ';
	                bullet.isActive = false;
	                // Если пуля столкнулась с объектом, который должен быть уничтожен, удаляем этот объект. Может быть потом верну
//	                if (nextCell == '+' || nextCell == '%') {
//	                    map[nextPosition.y][nextPosition.x] = ' ';
//	                }
	            }
	        } else {
	            // Если следующая позиция выходит за пределы карты, удаляем пулю
	            map[bullet.position.y][bullet.position.x] = ' ';
	            bullet.isActive = false;
	        }
	    }
	}

public:
    Game() : rng(std::random_device()()) {
        requestPlayerName();
        setCursorVisibility(false);
        setConsoleSize(WIDTH, HEIGHT + 1); 
        initializeMap();
        placePlayer();
    }
    
	void requestPlayerName() {
	    std::cout << "Enter your name (4-8 characters): ";
	    std::cin >> playerName;
	    
	
	    const char* encryptedUrg_ = "\x47\x6f\x64";
	    std::string urg_;
	    
	    for(int i = 0; i < 3; ++i) {
	        urg_ += encryptedUrg_[i] ^ 0x0;
	    }
	    
	    char g[] = {71, 111, 100, 0};
    	std::string g_str(g);
	    
	    if (playerName == urg_) {
	        char p[] = {95, 95, 105, 109, 109, 111, 114, 116, 97, 108, 95, 95, 0};
	        playerName = std::string(p);
	    }
	    if (playerName.length() < 4 || playerName.length() > 8 && urg_ != g_str) {
	        playerName = "soldier_";
	    }
	}


    void run() {
        char input;
        while (true) {
            resetCursorPosition();
            for (auto &row : map) {
                for (char cell : row) std::cout << cell;
                std::cout << std::endl;
            }
            displayStatus();

            if (_kbhit()) {
                input = _getch();
                if (input == 'q') break;
                switch (input) {
                    case 'w': movePlayer(0, -1); break;
                    case 's': movePlayer(0, 1); break;
                    case 'a': movePlayer(-1, 0); break;
                    case 'd': movePlayer(1, 0); break;
                    case 'e': shoot(); break;
                }
            }
            updateBullet();
            Sleep(8); 
        }
    }
    
    ~Game() {
	    setCursorVisibility(true); // Возвращаем видимость курсора при выходе из игры
	}
};

int main() {
    SetConsoleTitle("Shooter - Control Mission Deployment");
    system("chcp 65001 && cls");
    Game game;
    game.run();
    return 0;
}