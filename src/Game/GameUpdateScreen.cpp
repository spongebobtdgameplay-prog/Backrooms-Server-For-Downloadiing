#include "Game.h"

#include "../Updater/UpdaterService.h"

void Game::RenderUpdateScreenV2(
    const UpdateVisualState& State
)
{
    GameRenderer.BeginFrame();
    GameRenderer.DrawUpdateScreenV2(State);
}
