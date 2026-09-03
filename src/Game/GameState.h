#pragma once

struct GameState
{
    int BreakersRequired = 3;
    int BreakersActive = 0;

    bool Started = false;
    bool MainMenuOpen = true;
    bool Paused = false;
    bool Ended = false;
    bool Escaped = false;
    bool EntityReleased = false;

    void ActivateBreaker()
    {
        if (Ended)
            return;

        ++BreakersActive;

        if (BreakersActive >= 1)
            EntityReleased = true;
    }

    bool CanExit() const
    {
        return BreakersActive >= BreakersRequired;
    }
};
