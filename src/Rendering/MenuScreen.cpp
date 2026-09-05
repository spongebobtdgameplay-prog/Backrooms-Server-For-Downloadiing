#include "Renderer.h"

#include "../Core/Version.h"
#include "GeneratedWebUi.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <sstream>
#include <string>
#include <vector>

namespace
{
    glm::vec3 Rgb(const GeneratedWebUi::CssColor& Color)
    {
        return {Color.R, Color.G, Color.B};
    }

    glm::vec3 Composite(
        const GeneratedWebUi::CssColor& Foreground,
        const GeneratedWebUi::CssColor& Background
    )
    {
        const float A = std::clamp(Foreground.A, 0.0f, 1.0f);
        return {
            Foreground.R * A + Background.R * (1.0f - A),
            Foreground.G * A + Background.G * (1.0f - A),
            Foreground.B * A + Background.B * (1.0f - A)
        };
    }

    glm::vec3 Mix(
        const glm::vec3& A,
        const glm::vec3& B,
        float T
    )
    {
        T = std::clamp(T, 0.0f, 1.0f);
        return A * (1.0f - T) + B * T;
    }

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

    std::vector<std::string> WrapCssText(
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

void Renderer::DrawMenuBackdrop()
{
    using namespace GeneratedWebUi;

    const int W = static_cast<int>(Width);
    const int H = static_cast<int>(Height);
    const glm::vec3 Base = Rgb(Background);
    const glm::vec3 TopLift = Mix(
        Base,
        {1.0f, 247.0f / 255.0f, 173.0f / 255.0f},
        0.10f
    );
    const glm::vec3 BottomShade = Mix(
        Base,
        {39.0f / 255.0f, 32.0f / 255.0f, 8.0f / 255.0f},
        0.09f
    );

    constexpr int GradientBand = 16;

    for (int Y = 0; Y < H; Y += GradientBand)
    {
        const float T = H > 1
            ? static_cast<float>(Y) / static_cast<float>(H - 1)
            : 0.0f;

        const glm::vec3 Row = Mix(TopLift, BottomShade, T);

        DrawRect(
            0,
            Y,
            W,
            std::min(GradientBand, H - Y),
            Row
        );
    }

    const glm::vec3 WallpaperLine =
        Mix(Base, {87.0f / 255.0f, 77.0f / 255.0f, 28.0f / 255.0f}, 0.055f);

    for (int X = 0; X < W; X += 24)
        DrawRect(X, 0, 2, H, WallpaperLine);

    const glm::vec3 GridVertical =
        Mix(Base, {71.0f / 255.0f, 62.0f / 255.0f, 22.0f / 255.0f}, 0.045f);

    for (int X = 23; X < W; X += 25)
        DrawRect(X, 0, 2, H, GridVertical);

    const glm::vec3 GridHorizontal =
        Mix(Base, {1.0f, 245.0f / 255.0f, 155.0f / 255.0f}, 0.035f);

    for (int Y = 31; Y < H; Y += 32)
        DrawRect(0, Y, W, 1, GridHorizontal);

    const glm::vec3 Noise =
        Mix(Base, {38.0f / 255.0f, 32.0f / 255.0f, 10.0f / 255.0f}, 0.008f);

    for (int Y = 0; Y < H; Y += 4)
        DrawRect(0, Y, W, 1, Noise);
}

void Renderer::DrawMenuText(
    const std::string& Text,
    int X,
    int Y,
    int PixelHeight,
    int Weight,
    float TrackingEm,
    const glm::vec3& Color,
    float Opacity,
    bool Shadow
)
{
    if (!MenuTextRenderer.IsReady())
    {
        MenuTextRenderer.Initialize();
        MenuTextRenderer.Resize(Width, Height);
    }

    if (!MenuTextRenderer.IsReady())
        return;

    MenuTextRenderer.Resize(Width, Height);
    MenuTextRenderer.Draw(
        Text,
        X,
        Y,
        PixelHeight,
        Weight,
        TrackingEm,
        Color,
        Opacity,
        Shadow
    );
}

int Renderer::MenuTextWidth(
    const std::string& Text,
    int PixelHeight,
    int Weight,
    float TrackingEm
)
{
    if (!MenuTextRenderer.IsReady())
    {
        MenuTextRenderer.Initialize();
        MenuTextRenderer.Resize(Width, Height);
    }

    if (!MenuTextRenderer.IsReady())
        return 0;

    MenuTextRenderer.Resize(Width, Height);
    return MenuTextRenderer.Measure(
        Text,
        PixelHeight,
        Weight,
        TrackingEm
    );
}

void Renderer::DrawMainMenuV2(bool HasSession)
{
    using namespace GeneratedWebUi;

    glDisable(GL_DEPTH_TEST);
    DrawMenuBackdrop();

    const bool Mobile = Width <= MobileBreakpoint;
    const int Margin = Mobile ? MobileMenuMargin : MenuMargin;

    const int ContentX = Mobile
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

    const int ContentWidth = Mobile
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

    const int TitleSize = Mobile
        ? CssClamp(
            MobileTitleMin,
            MobileTitleViewport,
            static_cast<int>(Width),
            MobileTitleMax
        )
        : CssClamp(
            TitleMin,
            TitleViewport,
            static_cast<int>(Width),
            TitleMax
        );

    const glm::vec3 MetaRgb = Rgb(MetaColor);
    const glm::vec3 IndexRgb = Rgb(IndexColor);
    const glm::vec3 TitleRgb = Rgb(TitleColor);
    const glm::vec3 ParagraphRgb = Rgb(ParagraphColor);
    const glm::vec3 ButtonRgb = Rgb(ButtonColor);
    const glm::vec3 LoadRgb = Rgb(LoadColor);

    DrawMenuText(
        "THE BACKROOMS",
        Margin,
        MenuTopY,
        MenuMetaFont,
        400,
        MenuMetaTracking,
        MetaRgb,
        MetaColor.A,
        false
    );

    const std::string Version =
        std::string("V") + BuildVersion::Text;

    const int VersionWidth =
        MenuTextWidth(
            Version,
            MenuMetaFont,
            400,
            MenuMetaTracking
        );

    DrawMenuText(
        Version,
        static_cast<int>(Width) / 2 - VersionWidth / 2,
        MenuTopY,
        MenuMetaFont,
        400,
        MenuMetaTracking,
        MetaRgb,
        MetaColor.A * 0.72f,
        false
    );

    const std::string Status =
        HasSession
            ? "SESSION ACTIVE"
            : "UNSTABLE SESSION";

    const int StatusWidth =
        MenuTextWidth(
            Status,
            MenuMetaFont,
            400,
            MenuMetaTracking
        );

    DrawMenuText(
        Status,
        std::max(
            Margin,
            static_cast<int>(Width) -
                Margin -
                StatusWidth
        ),
        MenuTopY,
        MenuMetaFont,
        400,
        MenuMetaTracking,
        MetaRgb,
        MetaColor.A,
        false
    );

    const std::string Paragraph =
        "Mono-yellow rooms, damp carpet and fluorescent light with no reliable layout. "
        "Restore three breakers and find the powered exit.";

    const auto ParagraphLines =
        WrapCssText(
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

    const int MeasuredTitleHeight =
        std::max(
            TitleSize,
            MenuTextRenderer.IsReady()
                ? MenuTextRenderer.MeasureHeight(
                    "THE LOBBY",
                    TitleSize,
                    TitleWeight,
                    TitleTracking
                )
                : TitleSize
        );

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

    const int ContentHeight =
        IndexBlockHeight +
        TitleMarginTop +
        TitleAdvance +
        TitleMarginBottom +
        ParagraphMarginTop +
        static_cast<int>(ParagraphLines.size()) *
            ParagraphLineAdvance +
        ParagraphMarginBottom +
        ButtonHeight +
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
        400,
        IndexTracking,
        IndexRgb,
        IndexColor.A,
        false
    );

    const int IndexWidth =
        MenuTextWidth(
            "LEVEL 0",
            IndexFont,
            400,
            IndexTracking
        );

    DrawRect(
        ContentX,
        ContentY + IndexFont + IndexPaddingBottom,
        IndexWidth,
        IndexBorder,
        Composite(IndexBorderColor, Background)
    );

    const int TitleY =
        ContentY + IndexBlockHeight + TitleMarginTop;

    DrawMenuText(
        "THE LOBBY",
        ContentX,
        TitleY + 1,
        TitleSize,
        TitleWeight,
        TitleTracking,
        {1.0f, 247.0f / 255.0f, 174.0f / 255.0f},
        0.22f,
        false
    );

    DrawMenuText(
        "THE LOBBY",
        ContentX,
        TitleY,
        TitleSize,
        TitleWeight,
        TitleTracking,
        TitleRgb,
        TitleColor.A,
        false
    );

    const int ParagraphY =
        TitleY +
        TitleAdvance +
        TitleMarginBottom +
        ParagraphMarginTop;

    for (std::size_t I = 0; I < ParagraphLines.size(); ++I)
    {
        DrawMenuText(
            ParagraphLines[I],
            ContentX,
            ParagraphY +
                static_cast<int>(I) *
                    ParagraphLineAdvance,
            ParagraphFont,
            ParagraphWeight,
            0.0f,
            ParagraphRgb,
            ParagraphColor.A,
            false
        );
    }

    const int ButtonY =
        ParagraphY +
        static_cast<int>(ParagraphLines.size()) *
            ParagraphLineAdvance +
        ParagraphMarginBottom;

    const int ButtonWidth =
        std::min(ButtonWidthMax, ContentWidth);

    DrawRect(
        ContentX,
        ButtonY,
        ButtonWidth,
        1,
        Composite(ButtonBorderTopColor, Background)
    );

    DrawRect(
        ContentX,
        ButtonY + ButtonHeight - 1,
        ButtonWidth,
        1,
        Composite(ButtonBorderBottomColor, Background)
    );

    const std::string PrimaryAction =
        HasSession
            ? "RESUME SESSION"
            : "ENTER LEVEL 0";

    const int ButtonTextY =
        ButtonY +
        std::max(0, (ButtonHeight - ButtonFont) / 2);

    DrawMenuText(
        PrimaryAction,
        ContentX + ButtonPaddingX,
        ButtonTextY,
        ButtonFont,
        ButtonWeight,
        ButtonTracking,
        ButtonRgb,
        ButtonColor.A,
        false
    );

    const std::string Arrow = ">";
    const int ArrowWidth =
        MenuTextWidth(
            Arrow,
            ArrowFont,
            700,
            0.0f
        );

    DrawMenuText(
        Arrow,
        ContentX +
            ButtonWidth -
            ButtonPaddingX -
            ArrowWidth,
        ButtonY +
            std::max(0, (ButtonHeight - ArrowFont) / 2),
        ArrowFont,
        700,
        0.0f,
        ButtonRgb,
        ButtonColor.A,
        false
    );

    const std::string LoadStatus =
        HasSession
            ? "N   NEW SESSION"
            : "WASD   SHIFT   MOUSE   E";

    DrawMenuText(
        LoadStatus,
        ContentX,
        ButtonY + ButtonHeight + LoadMarginTop,
        LoadFont,
        400,
        LoadTracking,
        LoadRgb,
        LoadColor.A,
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
        400,
        MenuMetaTracking,
        MetaRgb,
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
            400,
            MenuMetaTracking
        );

    DrawMenuText(
        Footer,
        std::max(
            Margin,
            static_cast<int>(Width) -
                Margin -
                FooterWidth
        ),
        FooterY,
        MenuMetaFont,
        400,
        MenuMetaTracking,
        MetaRgb,
        MetaColor.A,
        false
    );

    glEnable(GL_DEPTH_TEST);
}

void Renderer::DrawPauseMenuV2()
{
    using namespace GeneratedWebUi;

    glDisable(GL_DEPTH_TEST);
    DrawMenuBackdrop();

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
            120,
            static_cast<int>(
                static_cast<float>(Height) * 0.38f
            )
        );

    const glm::vec3 IndexRgb = Rgb(IndexColor);
    const glm::vec3 TitleRgb = Rgb(TitleColor);
    const glm::vec3 ButtonRgb = Rgb(ButtonColor);
    const glm::vec3 LoadRgb = Rgb(LoadColor);

    DrawMenuText(
        "LEVEL 0",
        ContentX,
        ContentY,
        IndexFont,
        400,
        IndexTracking,
        IndexRgb,
        IndexColor.A,
        false
    );

    const int IndexWidth =
        MenuTextWidth(
            "LEVEL 0",
            IndexFont,
            400,
            IndexTracking
        );

    DrawRect(
        ContentX,
        ContentY + IndexFont + IndexPaddingBottom,
        IndexWidth,
        1,
        Composite(IndexBorderColor, Background)
    );

    const int TitleY =
        ContentY +
        IndexFont +
        IndexPaddingBottom +
        1 +
        TitleMarginTop;

    DrawMenuText(
        "SESSION PAUSED",
        ContentX,
        TitleY,
        TitleSize,
        900,
        -0.055f,
        TitleRgb,
        TitleColor.A,
        false
    );

    const int TitleHeight =
        MenuTextRenderer.IsReady()
            ? MenuTextRenderer.MeasureHeight(
                "SESSION PAUSED",
                TitleSize,
                900,
                -0.055f
            )
            : TitleSize;

    const int ButtonY =
        TitleY + TitleHeight + 36;

    const int ButtonWidth =
        std::min(
            410,
            std::max(
                1,
                static_cast<int>(Width) -
                    ContentX -
                    34
            )
        );

    DrawRect(
        ContentX,
        ButtonY,
        ButtonWidth,
        1,
        Composite(ButtonBorderTopColor, Background)
    );

    DrawMenuText(
        "ENTER   RESUME",
        ContentX + 4,
        ButtonY + 20,
        ButtonFont,
        ButtonWeight,
        ButtonTracking,
        ButtonRgb,
        ButtonColor.A,
        false
    );

    DrawRect(
        ContentX,
        ButtonY + ButtonHeight,
        ButtonWidth,
        1,
        Composite(ButtonBorderBottomColor, Background)
    );

    DrawMenuText(
        "M   MAIN MENU",
        ContentX + 4,
        ButtonY + ButtonHeight + 22,
        ButtonFont,
        ButtonWeight,
        ButtonTracking,
        ButtonRgb,
        ButtonColor.A,
        false
    );

    DrawMenuText(
        "ESC ALSO RESUMES",
        ContentX,
        ButtonY + ButtonHeight + 66,
        LoadFont,
        400,
        LoadTracking,
        LoadRgb,
        LoadColor.A,
        false
    );

    glEnable(GL_DEPTH_TEST);
}
