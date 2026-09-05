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
        PauseResumeHover,
        PauseResumeRect.Contains(MenuPointerX, MenuPointerY)
    );

    MoveToward(
        PauseMainMenuHover,
        PauseMainMenuRect.Contains(MenuPointerX, MenuPointerY)
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
        std::min(ParagraphWidthMax, ContentWidth);

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
            ParagraphWidth,
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

    const int ParagraphLineAdvance =
        static_cast<int>(
            std::round(
                static_cast<float>(ParagraphFont) *
                ParagraphLineHeight
            )
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
        TitleY + 1,
        TitleSize,
        TitleWeight,
        TitleTracking,
        {1.0f, 247.0f / 255.0f, 174.0f / 255.0f},
        0.20f,
        false
    );

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

        const std::string Arrow = ">";
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
            ? "ESC PAUSE   M MAIN MENU"
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
    using namespace GeneratedWebUi;

    glDisable(GL_DEPTH_TEST);
    DrawMenuBackdrop();

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

    const int ContentX =
        Width <= MobileBreakpoint
            ? MobileContentLeft
            : CssClamp(
                ContentLeftMin,
                ContentLeftViewport,
                static_cast<int>(Width),
                ContentLeftMax
            );

    const int TitleSize =
        std::clamp(
            static_cast<int>(
                std::round(
                    static_cast<float>(Width) * 0.075f
                )
            ),
            58,
            108
        );

    const int ContentY =
        std::max(
            118,
            static_cast<int>(
                static_cast<float>(Height) * 0.34f
            )
        );

    DrawMenuText(
        "LEVEL 0",
        ContentX,
        ContentY,
        IndexFont,
        500,
        IndexTracking,
        Rgb(IndexColor),
        IndexColor.A,
        false
    );

    DrawMenuText(
        "SESSION PAUSED",
        ContentX,
        ContentY + 34,
        TitleSize,
        900,
        -0.055f,
        Ink,
        1.0f,
        false
    );

    const int ButtonWidth =
        std::min(
            ButtonWidthMax,
            std::max(
                220,
                static_cast<int>(Width) - ContentX - 34
            )
        );

    const int ButtonY =
        ContentY + TitleSize + 70;

    PauseResumeRect = {
        ContentX,
        ButtonY,
        ButtonWidth,
        ButtonHeight
    };

    PauseMainMenuRect = {
        ContentX,
        ButtonY + ButtonHeight + 12,
        ButtonWidth,
        ButtonHeight
    };

    auto DrawPauseButton = [&](
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

        DrawRect(
            Rect.X,
            Rect.Y,
            Rect.Width,
            1,
            Mix(YellowBackground, Ink, 0.56f)
        );

        DrawRect(
            Rect.X,
            Rect.Y + Rect.Height - 1,
            Rect.Width,
            1,
            Mix(YellowBackground, Ink, 0.25f)
        );

        const int Padding =
            ButtonPaddingX +
            static_cast<int>(
                std::round(HoverAmount * 10.0f)
            );

        DrawMenuText(
            Label,
            Rect.X + Padding,
            Rect.Y + std::max(0, (Rect.Height - ButtonFont) / 2),
            ButtonFont,
            ButtonWeight,
            ButtonTracking,
            Ink,
            1.0f,
            false
        );

        const std::string Arrow = ">";
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
            Rect.Y + std::max(0, (Rect.Height - ArrowFont) / 2),
            ArrowFont,
            800,
            0.0f,
            Ink,
            1.0f,
            false
        );
    };

    DrawPauseButton(
        PauseResumeRect,
        "RESUME",
        PauseResumeHover
    );

    DrawPauseButton(
        PauseMainMenuRect,
        "MAIN MENU",
        PauseMainMenuHover
    );

    DrawMenuText(
        "ESC ALSO RESUMES",
        ContentX,
        PauseMainMenuRect.Y + PauseMainMenuRect.Height + 18,
        LoadFont,
        500,
        LoadTracking,
        Rgb(LoadColor),
        0.70f,
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
        248.0f / 255.0f,
        239.0f / 255.0f,
        173.0f / 255.0f
    };

    const glm::vec3 Muted{
        244.0f / 255.0f,
        235.0f / 255.0f,
        169.0f / 255.0f
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
        0.68f,
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
        0.96f,
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
        0.86f,
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
        0.58f,
        true
    );

    const int SprintY =
        static_cast<int>(Height) - 34;

    GameplayTextRenderer.Draw(
        "SPRINT",
        LeftX,
        SprintY,
        9,
        700,
        0.18f,
        Muted,
        0.78f,
        true
    );

    const int SprintLabelWidth =
        GameplayTextRenderer.Measure(
            "SPRINT",
            9,
            700,
            0.18f
        );

    const int BarX = LeftX + SprintLabelWidth + 10;
    const int BarY = SprintY + 5;
    const int BarWidth = 118;
    const int BarHeight = 3;
    const int FillWidth =
        static_cast<int>(
            std::round(
                static_cast<float>(BarWidth) *
                std::clamp(Stamina, 0.0f, 1.0f)
            )
        );

    DrawRect(
        BarX,
        BarY,
        BarWidth,
        BarHeight,
        {0.34f, 0.32f, 0.19f}
    );

    if (FillWidth > 0)
    {
        DrawRect(
            BarX,
            BarY,
            FillWidth,
            BarHeight,
            {0.95f, 0.91f, 0.65f}
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

        const glm::vec3 KeyColor{
            248.0f / 255.0f,
            239.0f / 255.0f,
            173.0f / 255.0f
        };

        DrawRect(PromptX, PromptY, 24, 1, KeyColor);
        DrawRect(PromptX, PromptY + 23, 24, 1, KeyColor);
        DrawRect(PromptX, PromptY, 1, 24, KeyColor);
        DrawRect(PromptX + 23, PromptY, 1, 24, KeyColor);

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
            0.96f,
            true
        );
    }
}
