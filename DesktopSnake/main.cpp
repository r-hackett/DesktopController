#include "DesktopSnake.h"

int main()
{
    try
    {
        DesktopSnake snake;

        while (!GetAsyncKeyState(VK_ESCAPE) && !snake.gameOver())
        {
            snake.step();
        }
    }
    catch (const std::exception& e)
    {
        fmt::print("{}\n", e.what());
    }
}