//In case of LNK1168 error (cannot open Snake.exe) write in command line: taskkill /F /IM Snake.exe

#include <iostream>
#include <windows.h>
#include <string>
#include <chrono>
#include <vector>
#include <fstream>

using namespace std;
using namespace std::chrono;

bool IsRunning;
enum class GameState { Menu, Game, GameOver };
GameState State;

uint64_t currentTime;

class Point {
public:
    int x;
    int y;

    bool operator==(Point a) {
        return x == a.x && y == a.y;
    }

    COORD ToCOORD() {
        return { (short)(x + 1), (short)(y + 1) };
    }
};

const Point MapSize = { 20, 10 };
const uint64_t frameLenght = 200;

vector<Point> Player;

enum class Direction { Up, Down, Left, Right };
Direction PlayerDirection, InputDirection;
bool rKeyState;

int score;
int bestScore = 0;

Point Apple;
bool foundApple;

fstream Data;

HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);

//From: https://stackoverflow.com/a/67195569/21199309
#define color_dark_red   4
#define color_green     10

#pragma region Animations

const uint64_t InstructionsAnimationRate = 700;

bool DrawnMenu;
bool menuInstructionsAnimation;
uint64_t startTime;

const uint64_t gameOverAnimationLenght = 2500;
const uint64_t gameOverAnimationRate = 400;

uint64_t GameOverTime, TimeSinceGameOver;
bool GameOverPointerAnimation;

const uint64_t EndScreenAnimationRate = 1000, GameOverAnimationRate = 400;
int EndScreenState;

bool gameOverInstructionsAnimation;

#pragma endregion

void HideCursor() {
    CONSOLE_CURSOR_INFO info;
    info.dwSize = 100;
    info.bVisible = false;
    SetConsoleCursorInfo(console, &info);
}

void SetColor(const int textcolor) {
    SetConsoleTextAttribute(console, textcolor);
}
void SetColor(const int textcolor, const int backgroundcolor) {
    SetConsoleTextAttribute(console, backgroundcolor << 4 | textcolor);
}
void ResetColor() {
    SetConsoleTextAttribute(console, 7);
}

void ClearConsole() {
    COORD topLeft = { 0, 0 };
    CONSOLE_SCREEN_BUFFER_INFO screen;
    DWORD written;

    GetConsoleScreenBufferInfo(console, &screen);
    FillConsoleOutputCharacterA(
        console, ' ', screen.dwSize.X * screen.dwSize.Y, topLeft, &written
    );
    FillConsoleOutputAttribute(
        console, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE,
        screen.dwSize.X * screen.dwSize.Y, topLeft, &written
    );
    SetConsoleCursorPosition(console, topLeft);
}

void Draw(Point position, char sprite) {
    SetConsoleCursorPosition(console, position.ToCOORD());
    cout << sprite;
}

void Print(pair<int, int> position, string text) {
    COORD realPosition = { position.first, position.second };

    SetConsoleCursorPosition(console, realPosition);
    cout << text;
}

uint64_t GetTime() {
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

void GameOver() {
    State = GameState::GameOver;
    GameOverTime = GetTime();
    TimeSinceGameOver = 0;
    GameOverPointerAnimation = true;

    if (score > bestScore) {
        bestScore = score;
        Data.open("Data.txt", fstream::out | fstream::trunc);
        Data << score;
        Data.close();
    }
}

void PlaceApple() {
    bool foundAppleSpawn = false;
    while (!foundAppleSpawn) {
        Apple = { rand() % (MapSize.x - 1), rand() % (MapSize.y - 1) };

        foundAppleSpawn = true;
        for (int i = 0; i < Player.size(); i++) {
            if (Player[i] == Apple) {
                foundAppleSpawn = false;
            }
        }
    }

    SetColor(color_dark_red);
    Draw(Apple, 'o');
    SetColor(color_green);
}

void CollectApple() {
    Draw(Apple, '#');
    PlaceApple();

    foundApple = true;

    score++;
    Print({ 7, MapSize.y + 2 }, to_string(score));
}

void AddSnakePart(Point position) {
    Draw(position, '#');

    if (count(Player.begin(), Player.end(), position))
        GameOver();

    if (position == Apple)
        CollectApple();

    Player.push_back(position);
}

void RemoveSnakePart() {
    Point position = Player.front();
    Draw(position, ' ');
    Player.erase(Player.begin());
}

int modulo(int a, int b) {
    if (a >= 0)
        return a % b;
    else
        return b + (a % b);
}

void PrepareGame() {
    GameOverTime = 0;
    TimeSinceGameOver = 0;
    EndScreenState = 0;
    foundApple = false;

    State = GameState::Game;
    InputDirection = Direction::Right;
    PlayerDirection = Direction::Right;

    ClearConsole();
    SetColor(color_dark_red);
    for (int y = 0; y < MapSize.y + 2; y++) {
        cout << "#";
        for (int x = 0; x < MapSize.x; x++) {
            if (y == 0 || y == MapSize.y + 1) cout << "#";
            else cout << " ";
        }
        cout << "#";

        cout << endl;
    }
    SetColor(color_green);
    cout << "Score: 0";

    Player.clear();
    AddSnakePart({ MapSize.x / 2 - 1, MapSize.y / 2 });
    AddSnakePart({ MapSize.x / 2, MapSize.y / 2 });
    AddSnakePart({ MapSize.x / 2 + 1, MapSize.y / 2 });

    score = 0;
    Data.open("Data.txt", fstream::in);
    if (!Data) {
        Data.open("Data.txt", fstream::out);
        Data << 0;
        bestScore = 0;
    }
    else {
        Data >> bestScore;
    }
    Data.close();

    PlaceApple();
}

void Frame() {
    if (State == GameState::GameOver) return;

    pair<int, int> PlayerVelocity;
    PlayerDirection = InputDirection;
    switch (PlayerDirection) {
    case Direction::Left:
        PlayerVelocity = { -1, 0 };
        break;

    case Direction::Right:
        PlayerVelocity = { 1, 0 };
        break;

    case Direction::Up:
        PlayerVelocity = { 0, -1 };
        break;

    case Direction::Down:
        PlayerVelocity = { 0, 1 };
        break;
    }

    Point lastCell = Player.back();

    int x = modulo((lastCell.x + PlayerVelocity.first), MapSize.x);
    int y = modulo((lastCell.y + PlayerVelocity.second), MapSize.y);

    Point headCell = { x, y };

    AddSnakePart(headCell);
    if (foundApple)
        foundApple = false;
    else
        if (State != GameState::GameOver) RemoveSnakePart();
}

bool Button(int key) {
    return GetKeyState(key) & 0x8000;
}

void RegisterInput() {
    if (State == GameState::Menu) {
        if (Button(VK_SPACE))
            PrepareGame();
    }
    else {
        if (Button(VK_LEFT) && PlayerDirection != Direction::Right)
            InputDirection = Direction::Left;
        else if (Button(VK_RIGHT) && PlayerDirection != Direction::Left)
            InputDirection = Direction::Right;
        else if (Button(VK_UP) && PlayerDirection != Direction::Down)
            InputDirection = Direction::Up;
        else if (Button(VK_DOWN) && PlayerDirection != Direction::Up)
            InputDirection = Direction::Down;


        bool lastRKeyState = rKeyState;
        rKeyState = Button('R');
        if (rKeyState && !lastRKeyState)
            PrepareGame();
    }

    if (Button(VK_ESCAPE))
        IsRunning = false;
}

void GameOverAnimation() {
    bool visible = (TimeSinceGameOver / gameOverAnimationRate) % 2 == 0;
    if (visible != GameOverPointerAnimation) {
        GameOverPointerAnimation = visible;
        Draw(Player.back(), visible ? '#' : ' ');
    }
}

//Ascii text art generator: https://patorjk.com/software/taag/
void DrawMenuScreen() {
    if (!DrawnMenu) {
        SetColor(color_green);
        cout << "   _____             _                      " << endl;
        cout << "  / ____|           | |                     " << endl;
        cout << " | (___  _ __   __ _| | _____     "; SetColor(color_dark_red); cout << "_     _   " << endl; SetColor(color_green);
        cout << "  \\___ \\| '_ \\ / _` | |/ / _ \\  "; SetColor(color_dark_red); cout << "_| |_ _| |_ " << endl; SetColor(color_green);
        cout << "  ____) | | | | (_| |   <  __/ "; SetColor(color_dark_red); cout << "|_   _|_   _|" << endl; SetColor(color_green);
        cout << " |_____/|_| |_|\\__,_|_|\\_\\___|   "; SetColor(color_dark_red); cout << "|_|   |_|  " << endl; SetColor(color_green);

        SetColor(color_dark_red);
        cout << "" << endl;
        cout << "" << endl;
        cout << "" << endl;
        cout << "           Use arrow keys to move" << endl;
        cout << "                                 " << endl;
        cout << "            Press Space To Start" << endl;
        cout << "" << endl;
        cout << "" << endl;
        cout << "" << endl;
        cout << "" << endl;
        cout << "" << endl;
        cout << "" << endl;
        cout << "" << endl;
        cout << "       Dominik Przybylo (KUNGERMOoN)" << endl;
        DrawnMenu = true;
    }
    else
    {
        uint64_t timeSinceStart = startTime - currentTime;
        bool visible = (timeSinceStart / InstructionsAnimationRate) % 2 == 1;
        if (menuInstructionsAnimation != visible) {
            menuInstructionsAnimation = visible;
            SetConsoleCursorPosition(console, { 0, 11 });

            if (visible) {
                cout << "            Press Space To Start" << endl;
            }
            else {
                cout << "                                                   " << endl;
                cout << "                                                   " << endl;
            }
        }
    }
}

void DrawEndScreen() {
    uint64_t timeSinceEndscreenStart = TimeSinceGameOver - gameOverAnimationLenght;

    if (EndScreenState == 0) {
        ClearConsole();
        SetColor(color_dark_red);
        cout << "   _____                      " << endl; //Width: 30
        cout << "  / ____|                     " << endl;
        cout << " | |  __  __ _ _ __ ___   ___ " << endl;
        cout << " | | |_ |/ _` | '_ ` _ \\ / _ \\" << endl;
        cout << " | |__| | (_| | | | | | |  __/" << endl;
        cout << "  \\_____|\\____|_| |_| |_|\\___|" << endl;

        EndScreenState = 1;
    }
    else if (EndScreenState == 1 && timeSinceEndscreenStart > GameOverAnimationRate) {
        cout << "      ____                       " << endl;
        cout << "     / __ \\                      " << endl;
        cout << "    | |  | |_   _____ _ __       " << endl;
        cout << "    | |  | \\ \\ / / _ \\ '__|      " << endl;
        cout << "    | |__| |\\ V /  __/ |         " << endl;
        cout << "     \\____/  \\_/ \\___|_|         " << endl;

        EndScreenState = 2;
    }
    else if (EndScreenState == 2 && timeSinceEndscreenStart > EndScreenAnimationRate + GameOverAnimationRate) {
        SetColor(color_green);
        cout << endl;
        cout << endl;
        cout << "        Score: " << score << endl;
        cout << "        Best Score: " << bestScore << endl;

        EndScreenState = 3;
        gameOverInstructionsAnimation = true;
    }
    else if (EndScreenState == 3 && timeSinceEndscreenStart > 2 * EndScreenAnimationRate + GameOverAnimationRate) {
        SetColor(color_green);
        cout << endl;
        cout << endl;
        cout << "      Press \"R\" to restart     " << endl;
        cout << "      Press \"Esc\" to exit      " << endl;

        EndScreenState = 4;
        gameOverInstructionsAnimation = true;
    }
    else if (EndScreenState == 4) {
        uint64_t timeSinceEndscreenEnd = timeSinceEndscreenStart - 2 * EndScreenAnimationRate + GameOverAnimationRate;

        bool visible = (timeSinceEndscreenEnd / InstructionsAnimationRate) % 2 == 1;
        if (gameOverInstructionsAnimation != visible) {
            gameOverInstructionsAnimation = visible;
            SetConsoleCursorPosition(console, { 0, 18 });

            if (visible) {
                cout << "     Press \"R\" to restart     " << endl;
                cout << "     Press \"Esc\" to exit      " << endl;
            }
            else {
                cout << "                                                   " << endl;
                cout << "                                                   " << endl;
            }
        }

    }
}

int main()
{
    srand(GetTime() % 10000);

    rKeyState = false;

    ClearConsole();

    State = GameState::Menu;
    DrawnMenu = false;

    startTime = GetTime();

    uint64_t lastFrameTime = startTime;
    IsRunning = true;
    while (IsRunning) {
        currentTime = GetTime();
        HideCursor();

        RegisterInput();
        if (State == GameState::Menu) {
            DrawMenuScreen();
        }
        else if (State == GameState::Game) {
            if (currentTime - lastFrameTime > frameLenght) {
                lastFrameTime = currentTime;
                Frame();
            }
        }
        else {
            TimeSinceGameOver = currentTime - GameOverTime;
            if (TimeSinceGameOver > gameOverAnimationLenght) {
                DrawEndScreen();
            }
            else {
                GameOverAnimation();
            }
        }
    }

    ClearConsole();
    SetColor(color_green);

    return 0;
}
