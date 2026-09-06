#include "Renderer.h"

#include "../Core/Version.h"
#include "GeneratedWebUi.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace
{
    int CssClamp(
        int Minimum,
        float ViewportFactor,
        int ViewportPixels,
        int Maximum
    )
    {
        return std::clamp(
            static_cast<int>(
                std::round(
                    static_cast<float>(ViewportPixels) *
                    ViewportFactor
                )
            ),
            Minimum,
            Maximum
        );
    }

    glm::vec3 Mix(
        const glm::vec3& A,
        const glm::vec3& B,
        float Amount
    )
    {
        const float T = std::clamp(Amount, 0.0f, 1.0f);
        return A * (1.0f - T) + B * T;
    }

    glm::vec3 Rgb(const GeneratedWebUi::CssColor& Color)
    {
        return {Color.R, Color.G, Color.B};
    }

    std::vector<std::string> WrapText(
        const std::string& Text,
        int MaximumWidth,
        const std::function<int(const std::string&)>& Measure
    )
    {
        std::vector<std::string> Lines;
        std::istringstream Stream(Text);
        std::string Word;
        std::string Current;

        while (Stream >> Word)
        {
            const std::string Candidate =
                Current.empty()
                    ? Word
                    : Current + " " + Word;

            if (
                !Current.empty() &&
                Measure(Candidate) > MaximumWidth
            )
            {
                Lines.push_back(Current);
                Current = Word;
            }
            else
            {
                Current = Candidate;
            }
        }

        if (!Current.empty())
            Lines.push_back(Current);

        if (Lines.empty())
            Lines.emplace_back();

        return Lines;
    }
}

void Renderer::SetMenuPointer(float X, float Y)
{
    MenuPointerX = X;
    MenuPointerY = Y;
}

void Renderer::ClearMenuPointer()
{
    MenuPointerX = -10000.0f;
    MenuPointerY = -10000.0f;
}

void Renderer::UpdateInterface(float DeltaTime)
{
    const float Blend =
        1.0f - std::exp(-std::max(DeltaTime, 0.0f) * 24.0f);

    auto MoveToward = [&](float& Value, bool Hovered)
    {
        const float Target = Hovered ? 1.0f : 0.0f;
        Value += (Target - Value) * Blend;

        if (std::abs(Value - Target) < 0.001f)
            Value = Target;
    };

    MoveToward(
        MainPrimaryHover,
        MainPrimaryRect.Contains(MenuPointerX, MenuPointerY)
    );

    MoveToward(
        MainSecondaryHover,
        MainSecondaryRect.Contains(MenuPointerX, MenuPointerY)
    );

    MoveToward(
        PauseMapHover,
        PauseMapRect.Contains(MenuPointerX, MenuPointerY)
    );

    MoveToward(
        PauseResumeHover,
        PauseResumeRect.Contains(MenuPointerX, MenuPointerY)
    );

    MoveToward(
        PauseMainMenuHover,
        PauseMainMenuRect.Contains(MenuPointerX, MenuPointerY)
    );

    MoveToward(
        FullMapBackHover,
        FullMapBackRect.Contains(MenuPointerX, MenuPointerY)
    );

    MoveToward(
        FullMapRandomHover,
        FullMapRandomRect.Contains(MenuPointerX, MenuPointerY)
    );

    MoveToward(
        FullMapClearHover,
        FullMapClearRect.Contains(MenuPointerX, MenuPointerY)
    );
}

MenuUiAction Renderer::HitTestMainMenu(bool HasSession) const
{
    if (MainPrimaryRect.Contains(MenuPointerX, MenuPointerY))
        return MenuUiAction::Primary;

    if (
        HasSession &&
        MainSecondaryRect.Contains(MenuPointerX, MenuPointerY)
    )
    {
        return MenuUiAction::NewSession;
    }

    return MenuUiAction::None;
}

MenuUiAction Renderer::HitTestPauseMenu() const
{
    if (PauseMapRect.Contains(MenuPointerX, MenuPointerY))
        return MenuUiAction::Map;

    if (PauseResumeRect.Contains(MenuPointerX, MenuPointerY))
        return MenuUiAction::Resume;

    if (PauseMainMenuRect.Contains(MenuPointerX, MenuPointerY))
        return MenuUiAction::MainMenu;

    return MenuUiAction::None;
}

void Renderer::ShutdownInterfaceV3()
{
    GameplayTextRenderer.Shutdown();
}

void Renderer::DrawMainMenuV3(bool HasSession)
{
    using namespace GeneratedWebUi;

    glDisable(GL_DEPTH_TEST);
    DrawMenuBackdrop();

    const bool Mobile = Width <= MobileBreakpoint;
    const int Margin = Mobile ? MobileMenuMargin : MenuMargin;

    const int ContentX =
        Mobile
            ? MobileContentLeft
            : CssClamp(
                ContentLeftMin,
                ContentLeftViewport,
                static_cast<int>(Width),
                ContentLeftMax
            );

    const int ContentBottom =
        CssClamp(
            ContentBottomMin,
            ContentBottomViewport,
            static_cast<int>(Height),
            ContentBottomMax
        );

    const int ContentWidth =
        Mobile
            ? std::max(
                1,
                static_cast<int>(Width) -
                    MobileContentViewportSubtract
            )
            : std::min(
                ContentWidthMax,
                std::max(
                    1,
                    static_cast<int>(Width) -
                        ContentViewportSubtract
                )
            );

    const int ParagraphWidth =
        std::max(
            180,
            std::min(
                590,
                std::max(ContentWidth - 20, 1)
            )
        );

    const int TitleSize =
        Mobile
            ? std::clamp(
                static_cast<int>(
                    std::round(
                        static_cast<float>(Width) *
                        MobileTitleViewport
                    )
                ),
                MobileTitleMin,
                MobileTitleMax
            )
            : CssClamp(
                TitleMin,
                TitleViewport,
                static_cast<int>(Width),
                TitleMax
            );

    const glm::vec3 YellowBackground{
        200.0f / 255.0f,
        187.0f / 255.0f,
        97.0f / 255.0f
    };

    const glm::vec3 Ink{
        39.0f / 255.0f,
        35.0f / 255.0f,
        15.0f / 255.0f
    };

    const glm::vec3 Meta = Rgb(MetaColor);
    const glm::vec3 Index = Rgb(IndexColor);
    const glm::vec3 ParagraphColorRgb = Rgb(ParagraphColor);

    DrawMenuText(
        "THE BACKROOMS",
        Margin,
        MenuTopY,
        MenuMetaFont,
        500,
        MenuMetaTracking,
        Meta,
        MetaColor.A,
        false
    );

    const std::string Version =
        std::string("V") + BuildVersion::Text;

    const int VersionWidth =
        MenuTextWidth(
            Version,
            MenuMetaFont,
            500,
            MenuMetaTracking
        );

    DrawMenuText(
        Version,
        static_cast<int>(Width) / 2 - VersionWidth / 2,
        MenuTopY,
        MenuMetaFont,
        500,
        MenuMetaTracking,
        Meta,
        MetaColor.A * 0.72f,
        false
    );

    const std::string Status =
        HasSession ? "SESSION ACTIVE" : "UNSTABLE SESSION";

    const int StatusWidth =
        MenuTextWidth(
            Status,
            MenuMetaFont,
            500,
            MenuMetaTracking
        );

    DrawMenuText(
        Status,
        std::max(
            Margin,
            static_cast<int>(Width) - Margin - StatusWidth
        ),
        MenuTopY,
        MenuMetaFont,
        500,
        MenuMetaTracking,
        Meta,
        MetaColor.A,
        false
    );

    const std::string Paragraph =
        "Mono-yellow rooms, damp carpet and fluorescent light with no reliable layout. "
        "Restore three breakers and find the powered exit.";

    const auto ParagraphLines =
        WrapText(
            Paragraph,
            std::max(ParagraphWidth - 16, 160),
            [&](const std::string& Line)
            {
                return MenuTextWidth(
                    Line,
                    ParagraphFont,
                    ParagraphWeight,
                    0.0f
                );
            }
        );

    MenuTextWidth(
        "THE LOBBY",
        TitleSize,
        TitleWeight,
        TitleTracking
    );

    const int MeasuredTitleHeight =
        MenuTextRenderer.IsReady()
            ? MenuTextRenderer.MeasureHeight(
                "THE LOBBY",
                TitleSize,
                TitleWeight,
                TitleTracking
            )
            : TitleSize;

    const int CssTitleLineBox =
        static_cast<int>(
            std::round(
                static_cast<float>(TitleSize) *
                TitleLineHeight
            )
        );

    const int TitleAdvance =
        std::max(CssTitleLineBox, MeasuredTitleHeight);

    const int MeasuredParagraphHeight =
        MenuTextRenderer.IsReady()
            ? MenuTextRenderer.MeasureHeight(
                "Ag",
                ParagraphFont,
                ParagraphWeight,
                0.0f
            )
            : ParagraphFont;

    const int ParagraphLineAdvance =
        std::max(
            static_cast<int>(
                std::round(
                    static_cast<float>(ParagraphFont) *
                    ParagraphLineHeight
                )
            ),
            MeasuredParagraphHeight + 5
        );

    const int IndexBlockHeight =
        IndexFont + IndexPaddingBottom + IndexBorder;

    const int ButtonWidth =
        std::min(ButtonWidthMax, ContentWidth);

    const int SecondaryGap = 12;
    const int SecondaryButtonHeight = 48;

    const int ButtonsHeight =
        HasSession
            ? ButtonHeight + SecondaryGap + SecondaryButtonHeight
            : ButtonHeight;

    const int ContentHeight =
        IndexBlockHeight +
        TitleMarginTop +
        TitleAdvance +
        TitleMarginBottom +
        ParagraphMarginTop +
        static_cast<int>(ParagraphLines.size()) *
            ParagraphLineAdvance +
        ParagraphMarginBottom +
        ButtonsHeight +
        LoadMarginTop +
        LoadFont;

    const int ContentY =
        std::max(
            64,
            static_cast<int>(Height) -
                ContentBottom -
                ContentHeight
        );

    DrawMenuText(
        "LEVEL 0",
        ContentX,
        ContentY,
        IndexFont,
        500,
        IndexTracking,
        Index,
        IndexColor.A,
        false
    );

    const int IndexWidth =
        MenuTextWidth(
            "LEVEL 0",
            IndexFont,
            500,
            IndexTracking
        );

    DrawRect(
        ContentX,
        ContentY + IndexFont + IndexPaddingBottom,
        IndexWidth,
        1,
        Mix(YellowBackground, Ink, 0.42f)
    );

    const int TitleY =
        ContentY +
        IndexBlockHeight +
        TitleMarginTop;


    DrawMenuText(
        "THE LOBBY",
        ContentX,
        TitleY,
        TitleSize,
        TitleWeight,
        TitleTracking,
        Ink,
        1.0f,
        false
    );

    const int ParagraphY =
        TitleY +
        TitleAdvance +
        TitleMarginBottom +
        ParagraphMarginTop;

    for (
        std::size_t I = 0;
        I < ParagraphLines.size();
        ++I
    )
    {
        DrawMenuText(
            ParagraphLines[I],
            ContentX,
            ParagraphY +
                static_cast<int>(I) * ParagraphLineAdvance,
            ParagraphFont,
            ParagraphWeight,
            0.0f,
            ParagraphColorRgb,
            0.88f,
            false
        );
    }

    const int ButtonY =
        ParagraphY +
        static_cast<int>(ParagraphLines.size()) *
            ParagraphLineAdvance +
        ParagraphMarginBottom;

    MainPrimaryRect = {
        ContentX,
        ButtonY,
        ButtonWidth,
        ButtonHeight
    };

    MainSecondaryRect = {};

    auto DrawButton = [&](
        const UiRect& Rect,
        const std::string& Label,
        float Hover
    )
    {
        const float HoverAmount =
            std::clamp(Hover, 0.0f, 1.0f);

        if (HoverAmount > 0.001f)
        {
            DrawRect(
                Rect.X,
                Rect.Y + 1,
                Rect.Width,
                std::max(Rect.Height - 2, 1),
                Mix(YellowBackground, Ink, 0.08f * HoverAmount)
            );
        }

        const glm::vec3 TopBorder =
            Mix(YellowBackground, Ink, 0.56f);

        const glm::vec3 BottomBorder =
            Mix(YellowBackground, Ink, 0.25f);

        DrawRect(
            Rect.X,
            Rect.Y,
            Rect.Width,
            1,
            TopBorder
        );

        DrawRect(
            Rect.X,
            Rect.Y + Rect.Height - 1,
            Rect.Width,
            1,
            BottomBorder
        );

        const int Padding =
            ButtonPaddingX +
            static_cast<int>(
                std::round(HoverAmount * 10.0f)
            );

        const int TextY =
            Rect.Y +
            std::max(0, (Rect.Height - ButtonFont) / 2);

        DrawMenuText(
            Label,
            Rect.X + Padding,
            TextY,
            ButtonFont,
            ButtonWeight,
            ButtonTracking,
            Ink,
            1.0f,
            false
        );

        const std::string Arrow = "\xE2\x86\x92";
        const int ArrowWidth =
            MenuTextWidth(
                Arrow,
                ArrowFont,
                800,
                0.0f
            );

        DrawMenuText(
            Arrow,
            Rect.X + Rect.Width - Padding - ArrowWidth,
            Rect.Y +
                std::max(0, (Rect.Height - ArrowFont) / 2),
            ArrowFont,
            800,
            0.0f,
            Ink,
            1.0f,
            false
        );
    };

    DrawButton(
        MainPrimaryRect,
        HasSession ? "RESUME SESSION" : "ENTER LEVEL 0",
        MainPrimaryHover
    );

    int HintY =
        ButtonY + ButtonHeight + LoadMarginTop;

    if (HasSession)
    {
        MainSecondaryRect = {
            ContentX,
            ButtonY + ButtonHeight + SecondaryGap,
            ButtonWidth,
            SecondaryButtonHeight
        };

        DrawButton(
            MainSecondaryRect,
            "NEW SESSION",
            MainSecondaryHover
        );

        HintY =
            MainSecondaryRect.Y +
            MainSecondaryRect.Height +
            LoadMarginTop;
    }

    DrawMenuText(
        "WASD   SHIFT   MOUSE   E",
        ContentX,
        HintY,
        LoadFont,
        500,
        LoadTracking,
        Meta,
        0.58f,
        false
    );

    const int FooterY =
        static_cast<int>(Height) -
        MenuFooterBottom -
        MenuMetaFont;

    DrawMenuText(
        "NOCLIP DESTINATION",
        Margin,
        FooterY,
        MenuMetaFont,
        500,
        MenuMetaTracking,
        Meta,
        MetaColor.A,
        false
    );

    const std::string Footer =
        HasSession
            ? "ESC PAUSE   M MAP   F11 FULLSCREEN"
            : "ESC RELEASES CURSOR";

    const int FooterWidth =
        MenuTextWidth(
            Footer,
            MenuMetaFont,
            500,
            MenuMetaTracking
        );

    DrawMenuText(
        Footer,
        std::max(
            Margin,
            static_cast<int>(Width) - Margin - FooterWidth
        ),
        FooterY,
        MenuMetaFont,
        500,
        MenuMetaTracking,
        Meta,
        MetaColor.A,
        false
    );

    glEnable(GL_DEPTH_TEST);
}

void Renderer::DrawPauseMenuV3()
{
    glDisable(GL_DEPTH_TEST);
    DrawMenuBackdrop();

    const glm::vec3 Yellow{200.0f / 255.0f, 187.0f / 255.0f, 97.0f / 255.0f};
    const glm::vec3 Ink{39.0f / 255.0f, 35.0f / 255.0f, 15.0f / 255.0f};
    const glm::vec3 Muted{0.34f, 0.31f, 0.15f};

    const int Margin =
        std::clamp(static_cast<int>(Width) / 24, 28, 66);

    DrawMenuText(
        "BACKROOMS OFFICAL",
        Margin,
        30,
        13,
        700,
        0.18f,
        Muted,
        1.0f,
        false
    );

    DrawMenuText(
        "PAUSE",
        Margin,
        64,
        std::clamp(static_cast<int>(Width) / 18, 54, 88),
        900,
        -0.045f,
        Ink,
        1.0f,
        false
    );

    const int NavY = 164;
    const int NavGap = 12;
    const int Available =
        std::max(static_cast<int>(Width) - Margin * 2 - NavGap * 2, 3);
    const int NavWidth =
        std::clamp(Available / 3, 150, 300);
    const int NavHeight = 52;

    PauseMapRect = {Margin, NavY, NavWidth, NavHeight};
    PauseResumeRect = {Margin + NavWidth + NavGap, NavY, NavWidth, NavHeight};
    PauseMainMenuRect = {Margin + (NavWidth + NavGap) * 2, NavY, NavWidth, NavHeight};

    auto DrawTab = [&](const UiRect& Rect, const std::string& Label, float Hover)
    {
        const float Amount = std::clamp(Hover, 0.0f, 1.0f);

        if (Amount > 0.001f)
        {
            const glm::vec3 Fill = Yellow * (1.0f - Amount * 0.12f) + Ink * (Amount * 0.12f);
            DrawRect(Rect.X, Rect.Y + 2, Rect.Width, Rect.Height - 4, Fill);
        }

        DrawRect(Rect.X, Rect.Y, Rect.Width, 2, Ink);
        DrawRect(Rect.X, Rect.Y + Rect.Height - 2, Rect.Width, 2, Ink);

        const int Padding = 14 + static_cast<int>(std::round(Amount * 12.0f));

        DrawMenuText(
            Label,
            Rect.X + Padding,
            Rect.Y + 17,
            14,
            800,
            0.12f,
            Ink,
            1.0f,
            false
        );
    };

    DrawTab(PauseMapRect, "MAP", PauseMapHover);
    DrawTab(PauseResumeRect, "RESUME", PauseResumeHover);
    DrawTab(PauseMainMenuRect, "MAIN MENU", PauseMainMenuHover);

    const int DetailY = NavY + NavHeight + 56;

    DrawMenuText(
        "LEVEL 0",
        Margin,
        DetailY,
        12,
        700,
        0.20f,
        Muted,
        1.0f,
        false
    );

    DrawMenuText(
        "UNSTABLE SESSION",
        Margin,
        DetailY + 30,
        30,
        900,
        -0.02f,
        Ink,
        1.0f,
        false
    );

    DrawMenuText(
        "MAP IS A PAUSE-MENU TAB. PAN IN ANY DIRECTION, SET ROUTED WAYPOINTS, TRACK BREAKERS, EXIT AND THREATS.",
        Margin,
        DetailY + 86,
        13,
        650,
        0.03f,
        Muted,
        1.0f,
        false
    );

    DrawRect(
        Margin,
        DetailY + 128,
        std::max(static_cast<int>(Width) - Margin * 2, 1),
        1,
        Muted
    );

    DrawMenuText(
        "M MAP     ESC RESUME     F11 FULLSCREEN",
        Margin,
        static_cast<int>(Height) - 46,
        10,
        700,
        0.15f,
        Muted,
        1.0f,
        false
    );

    glEnable(GL_DEPTH_TEST);
}

void Renderer::DrawGameplayOverlayV3(
    float Stamina,
    int BreakersActive,
    int BreakersRequired,
    int InteractionType,
    bool CanExit,
    float Fps
)
{
    if (!GameplayTextRenderer.IsReady())
    {
        GameplayTextRenderer.Initialize();
    }

    if (!GameplayTextRenderer.IsReady())
        return;

    GameplayTextRenderer.Resize(Width, Height);

    const glm::vec3 Primary{
        1.0f,
        0.965f,
        0.74f
    };

    const glm::vec3 Muted{
        0.86f,
        0.82f,
        0.62f
    };

    const int LeftX = Width <= 700 ? 18 : 26;
    const int TopY = 24;

    GameplayTextRenderer.Draw(
        "LEVEL 0",
        LeftX,
        TopY,
        9,
        650,
        0.24f,
        Muted,
        1.0f,
        true
    );

    std::ostringstream Objective;
    Objective
        << "RESTORE POWER  "
        << BreakersActive
        << "/"
        << BreakersRequired;

    GameplayTextRenderer.Draw(
        Objective.str(),
        LeftX,
        TopY + 18,
        13,
        800,
        0.13f,
        Primary,
        1.0f,
        true
    );

    std::ostringstream FpsStream;

    if (Fps > 0.0f)
    {
        FpsStream
            << std::fixed
            << std::setprecision(1)
            << Fps
            << " FPS";
    }
    else
    {
        FpsStream << "--.- FPS";
    }

    const std::string FpsText = FpsStream.str();
    const int RightMargin = Width <= 700 ? 18 : 26;

    const int FpsWidth =
        GameplayTextRenderer.Measure(
            FpsText,
            11,
            700,
            0.10f
        );

    GameplayTextRenderer.Draw(
        FpsText,
        static_cast<int>(Width) - RightMargin - FpsWidth,
        TopY,
        11,
        700,
        0.10f,
        Primary,
        1.0f,
        true
    );

    const std::string BuildText =
        std::string("BUILD  ") + BuildVersion::Text;

    const int BuildWidth =
        GameplayTextRenderer.Measure(
            BuildText,
            9,
            600,
            0.13f
        );

    GameplayTextRenderer.Draw(
        BuildText,
        static_cast<int>(Width) - RightMargin - BuildWidth,
        TopY + 18,
        9,
        600,
        0.13f,
        Muted,
        1.0f,
        true
    );

    const float StaminaAmount =
        std::clamp(Stamina, 0.0f, 1.0f);

    const glm::vec3 LowStamina{
        0.94f,
        0.12f,
        0.07f
    };

    const glm::vec3 MediumStamina{
        1.0f,
        0.46f,
        0.04f
    };

    const glm::vec3 HighStamina{
        0.12f,
        0.82f,
        0.24f
    };

    glm::vec3 StaminaColor;

    if (StaminaAmount <= 0.5f)
    {
        const float Amount = StaminaAmount / 0.5f;
        StaminaColor =
            LowStamina * (1.0f - Amount) +
            MediumStamina * Amount;
    }
    else
    {
        const float Amount =
            (StaminaAmount - 0.5f) / 0.5f;

        StaminaColor =
            MediumStamina * (1.0f - Amount) +
            HighStamina * Amount;
    }

    const bool CompactHud = Width < 900 || Height < 620;
    const int MiniMapHeight = std::clamp(
        static_cast<int>(Height * (CompactHud ? 0.22f : 0.205f)),
        CompactHud ? 148 : 172,
        CompactHud ? 188 : 202
    );
    const int MiniMapMargin = std::clamp(
        static_cast<int>(Width * 0.015f),
        16,
        26
    );
    const int MiniMapTop =
        static_cast<int>(Height) - MiniMapHeight - MiniMapMargin;

    const int SprintFontHeight = 11;
    const int SprintY = std::max(80, MiniMapTop - 36);

    GameplayTextRenderer.Draw(
        "SPRINT",
        LeftX,
        SprintY,
        SprintFontHeight,
        780,
        0.15f,
        StaminaColor,
        1.0f,
        true
    );

    const int SprintLabelWidth =
        GameplayTextRenderer.Measure(
            "SPRINT",
            SprintFontHeight,
            780,
            0.15f
        );

    const int BarX = LeftX + SprintLabelWidth + 12;
    const int BarWidth = 148;
    const int BarHeight = 6;
    const int TextRasterTopPadding = 4;
    const int BarY =
        SprintY +
        TextRasterTopPadding +
        (SprintFontHeight - BarHeight) / 2;

    const int FillWidth =
        static_cast<int>(
            std::round(
                static_cast<float>(BarWidth) *
                StaminaAmount
            )
        );

    DrawRect(
        BarX - 1,
        BarY - 1,
        BarWidth + 2,
        BarHeight + 2,
        {0.07f, 0.065f, 0.035f}
    );

    DrawRect(
        BarX,
        BarY,
        BarWidth,
        BarHeight,
        {0.18f, 0.16f, 0.085f}
    );

    if (FillWidth > 0)
    {
        DrawRect(
            BarX,
            BarY,
            FillWidth,
            BarHeight,
            StaminaColor
        );
    }

    std::string Prompt;

    if (InteractionType == 1)
        Prompt = "ACTIVATE BREAKER";
    else if (InteractionType == 2)
        Prompt = CanExit ? "OPEN EXIT" : "EXIT HAS NO POWER";

    if (!Prompt.empty())
    {
        const int PromptTextWidth =
            GameplayTextRenderer.Measure(
                Prompt,
                13,
                800,
                0.08f
            );

        const int PromptWidth = PromptTextWidth + 48;
        const int PromptX =
            static_cast<int>(Width) / 2 - PromptWidth / 2;
        const int PromptY =
            static_cast<int>(Height) - 84;

        DrawRect(PromptX, PromptY, 24, 1, Primary);
        DrawRect(PromptX, PromptY + 23, 24, 1, Primary);
        DrawRect(PromptX, PromptY, 1, 24, Primary);
        DrawRect(PromptX + 23, PromptY, 1, 24, Primary);

        GameplayTextRenderer.Draw(
            "E",
            PromptX + 8,
            PromptY + 4,
            12,
            900,
            0.0f,
            Primary,
            1.0f,
            true
        );

        GameplayTextRenderer.Draw(
            Prompt,
            PromptX + 36,
            PromptY + 5,
            13,
            800,
            0.08f,
            Primary,
            1.0f,
            true
        );
    }
}
