#pragma once

#include <algorithm>
#include <cstdint>

namespace MenuLayout
{
    struct Rect
    {
        int X = 0;
        int Y = 0;
        int Width = 0;
        int Height = 0;

        bool Contains(float PointX, float PointY) const
        {
            return
                PointX >= static_cast<float>(X) &&
                PointY >= static_cast<float>(Y) &&
                PointX < static_cast<float>(X + Width) &&
                PointY < static_cast<float>(Y + Height);
        }
    };

    struct MainMenu
    {
        Rect Panel;
        Rect PrimaryButton;
        Rect SecondaryButton;
        bool HasSecondaryButton = false;
    };

    struct PauseMenu
    {
        Rect Panel;
        Rect ResumeButton;
        Rect MainMenuButton;
    };

    inline MainMenu BuildMainMenu(
        uint32_t Width,
        uint32_t Height,
        bool HasSession
    )
    {
        const int W = std::max(1, static_cast<int>(Width));
        const int H = std::max(1, static_cast<int>(Height));

        const int PanelWidth = std::min(620, std::max(280, W - 40));
        const int DesiredPanelHeight = HasSession ? 430 : 370;
        const int PanelHeight = std::min(DesiredPanelHeight, std::max(300, H - 40));
        const int PanelX = (W - PanelWidth) / 2;
        const int PanelY = (H - PanelHeight) / 2;

        const int ButtonWidth = std::min(320, std::max(220, PanelWidth - 84));
        const int ButtonHeight = 52;
        const int ButtonX = PanelX + (PanelWidth - ButtonWidth) / 2;
        const int BottomPadding = 38;
        const int Gap = 14;

        MainMenu Layout;
        Layout.Panel = {PanelX, PanelY, PanelWidth, PanelHeight};
        Layout.HasSecondaryButton = HasSession;

        if (HasSession)
        {
            Layout.SecondaryButton = {
                ButtonX,
                PanelY + PanelHeight - BottomPadding - ButtonHeight,
                ButtonWidth,
                ButtonHeight
            };

            Layout.PrimaryButton = {
                ButtonX,
                Layout.SecondaryButton.Y - Gap - ButtonHeight,
                ButtonWidth,
                ButtonHeight
            };
        }
        else
        {
            Layout.PrimaryButton = {
                ButtonX,
                PanelY + PanelHeight - BottomPadding - ButtonHeight,
                ButtonWidth,
                ButtonHeight
            };
        }

        return Layout;
    }

    inline PauseMenu BuildPauseMenu(
        uint32_t Width,
        uint32_t Height
    )
    {
        const int W = std::max(1, static_cast<int>(Width));
        const int H = std::max(1, static_cast<int>(Height));

        const int PanelWidth = std::min(560, std::max(280, W - 40));
        const int PanelHeight = std::min(340, std::max(300, H - 40));
        const int PanelX = (W - PanelWidth) / 2;
        const int PanelY = (H - PanelHeight) / 2;

        const int ButtonWidth = std::min(300, std::max(220, PanelWidth - 84));
        const int ButtonHeight = 52;
        const int ButtonX = PanelX + (PanelWidth - ButtonWidth) / 2;
        const int Gap = 14;
        const int BottomPadding = 34;

        PauseMenu Layout;
        Layout.Panel = {PanelX, PanelY, PanelWidth, PanelHeight};
        Layout.MainMenuButton = {
            ButtonX,
            PanelY + PanelHeight - BottomPadding - ButtonHeight,
            ButtonWidth,
            ButtonHeight
        };
        Layout.ResumeButton = {
            ButtonX,
            Layout.MainMenuButton.Y - Gap - ButtonHeight,
            ButtonWidth,
            ButtonHeight
        };

        return Layout;
    }
}
