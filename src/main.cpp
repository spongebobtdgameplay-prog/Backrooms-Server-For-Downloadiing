#include "Core/Application.h"

#include <SDL3/SDL_main.h>

int main(int argc, char** argv)
{
    static_cast<void>(argc);
    static_cast<void>(argv);

    Application App;

    if (!App.Initialize())
    {
        return 1;
    }

    return App.Run();
}
