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
    // Direct native rendering of the original style.css StartScreen:
    // radial-gradient(circle at 72% 42%, rgba(213,200,104,.16), transparent 34%),
    // linear-gradient(115deg, #1c1a0e 0%, #0d0c08 56%, #070706 100%).
    const glm::vec3 Left{28.0f / 255.0f, 26.0f / 255.0f, 14.0f / 255.0f};
    const glm::vec3 Middle{13.0f / 255.0f, 12.0f / 255.0f, 8.0f / 255.0f};
    const glm::vec3 Right{7.0f / 255.0f, 7.0f / 255.0f, 6.0f / 255.0f};
    const glm::vec3 Glow{213.0f / 255.0f, 200.0f / 255.0f, 104.0f / 255.0f};

    const int W = static_cast<int>(Width);
    const int H = static_cast<int>(Height);
    constexpr int Band = 8;

    for (int X = 0; X < W; X += Band)
    {
        const float T = W > 1
            ? static_cast<float>(X) / static_cast<float>(W - 1)
            : 0.0f;

        glm::vec3 Base;
        if (T <= 0.56f)
        {
            const float Local = T / 0.56f;
            Base = Left * (1.0f - Local) + Middle * Local;
        }
        else
        {
            const float Local = (T - 0.56f) / 0.44f;
            Base = Middle * (1.0f - Local) + Right * Local;
        }

        DrawRect(X, 0, std::min(Band, W - X), H, Base);
    }

    const int CenterX = static_cast<int>(std::round(W * 0.72f));
    const int CenterY = static_cast<int>(std::round(H * 0.42f));
    const int RadiusX = std::max(1, static_cast<int>(std::round(W * 0.34f)));
    const int RadiusY = std::max(1, static_cast<int>(std::round(H * 0.34f)));

    for (int I = 10; I >= 1; --I)
    {
        const float T = static_cast<float>(I) / 10.0f;
        const float Alpha = 0.016f * (1.0f - T + 0.10f);
        const int RX = static_cast<int>(RadiusX * T);
        const int RY = static_cast<int>(RadiusY * T);
        const glm::vec3 Tint = Right * (1.0f - Alpha) + Glow * Alpha;
        DrawRect(CenterX - RX, CenterY - RY, RX * 2, RY * 2, Tint);
    }

    // Exact 98px repeating grid from #StartScreen::before.
    const glm::vec3 GridV = Right * 0.975f +
        glm::vec3{235.0f / 255.0f, 224.0f / 255.0f, 143.0f / 255.0f} * 0.025f;
    const glm::vec3 GridH = Right * 0.982f +
        glm::vec3{235.0f / 255.0f, 224.0f / 255.0f, 143.0f / 255.0f} * 0.018f;

    for (int X = 97; X < W; X += 98)
        DrawRect(X, 0, 1, H, GridV);

    for (int Y = 97; Y < H; Y += 98)
        DrawRect(0, Y, W, 1, GridH);

    // .MenuNoise: 1px line every 3px at the original tiny effective opacity.
    const float NoiseAlpha = 0.12f * 0.018f;
    const glm::vec3 NoiseColor = Right * (1.0f - NoiseAlpha) +
        glm::vec3{1.0f} * NoiseAlpha;

    for (int Y = 0; Y < H; Y += 3)
        DrawRect(0, Y, W, 1, NoiseColor);
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

    if (MenuTextRenderer.IsReady())
    {
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
        return;
    }

    const int FallbackScale =
        std::max(1, PixelHeight / 8);

    DrawText(
        Text,
        X,
        Y,
        FallbackScale,
        Color
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

    if (MenuTextRenderer.IsReady())
    {
        MenuTextRenderer.Resize(Width, Height);
        return MenuTextRenderer.Measure(
            Text,
            PixelHeight,
            Weight,
            TrackingEm
        );
    }

    const int FallbackScale =
        std::max(1, PixelHeight / 8);

    return TextWidth(
        Text,
        FallbackScale
    );
}

void Renderer::DrawMainMenuV2(bool HasSession)
{
    using namespace GeneratedWebUi;

    glDisable(GL_DEPTH_TEST);
    DrawMenuBackdrop();

    const bool Mobile = Width <= 700;

    const int Margin = Mobile ? 20 : MenuMargin;
    const int ContentX = Mobile
        ? 24
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
        ? std::max(1, static_cast<int>(Width) - 48)
        : std::min(
            ContentWidthMax,
            std::max(
                1,
                static_cast<int>(Width) -
                    ContentViewportSubtract
            )
        );

    const int ParagraphWidth =
        std::min(
            ParagraphWidthMax,
            ContentWidth
        );

    const int TitleSize = Mobile
        ? std::clamp(
            static_cast<int>(
                std::round(
                    static_cast<float>(Width) * 0.21f
                )
            ),
            58,
            100
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

    // Top row is a direct conversion of .MenuTop.
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
        static_cast<int>(Width) / 2 -
            VersionWidth / 2,
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

    // Ensure the text renderer is initialized before asking for real glyph height.
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

    // Browser glyphs fit inside the CSS line box. GDI can report a taller
    // glyph box, so use the larger value to preserve the CSS box model
    // without ever allowing the paragraph to overlap the title.
    const int TitleAdvance =
        std::max(
            CssTitleLineBox,
            MeasuredTitleHeight
        );

    const int ParagraphLineAdvance =
        static_cast<int>(
            std::round(
                static_cast<float>(ParagraphFont) *
                ParagraphLineHeight
            )
        );

    const int IndexBlockHeight =
        IndexFont +
        IndexPaddingBottom +
        IndexBorder;

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

    // .MenuIndex
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

    // h1 box: margin:14px 0 12px; line-height:.78.
    const int TitleY =
        ContentY +
        IndexBlockHeight +
        TitleMarginTop;

    // Exact #StartScreen h1 highlight shadow: 0 1px 0 rgba(255,247,174,.22)
    DrawMenuText(
        "THE LOBBY",
        ContentX,
        TitleY + 1,
        TitleSize,
        TitleWeight,
        TitleTracking,
        {255.0f / 255.0f, 247.0f / 255.0f, 174.0f / 255.0f},
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

    // One real HTML <p>, wrapped by measured native glyph widths to the
    // original CSS max-width:520px and line-height:1.6.
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
        std::min(
            ButtonWidthMax,
            ContentWidth
        );

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
        std::max(
            0,
            (ButtonHeight - ButtonFont) / 2
        );

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
            std::max(
                0,
                (ButtonHeight - ArrowFont) / 2
            ),
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

    // .MenuFooter
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
        Width <= 700
            ? 24
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

    MenuTextWidth(
        "SESSION PAUSED",
        TitleSize,
        900,
        -0.055f
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
        TitleY +
        TitleHeight +
        36;

    const int ButtonWidth =
        std::min(
            410,
            std::max(
                1,
                static_cast<int>(Width) -
                    ContentX - 34
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
