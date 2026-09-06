#pragma once

// AUTO-GENERATED from assets/ui/original/style.css by tools/convert_web_ui.py.
// Do not hand-tune these values. Change the original CSS source and regenerate.
namespace GeneratedWebUi
{
struct CssColor { float R; float G; float B; float A; };

inline constexpr int MenuMargin = 34;
inline constexpr int MenuTopY = 28;
inline constexpr int MenuFooterBottom = 24;
inline constexpr int MenuMetaFont = 10;
inline constexpr float MenuMetaTracking = 0.200000f;

inline constexpr int MobileBreakpoint = 700;
inline constexpr int MobileMenuMargin = 20;
inline constexpr int MobileContentLeft = 24;
inline constexpr int MobileContentViewportSubtract = 48;
inline constexpr int MobileTitleMin = 58;
inline constexpr float MobileTitleViewport = 0.210000f;
inline constexpr int MobileTitleMax = 100;

inline constexpr int ContentLeftMin = 34;
inline constexpr float ContentLeftViewport = 0.070000f;
inline constexpr int ContentLeftMax = 110;
inline constexpr int ContentBottomMin = 74;
inline constexpr float ContentBottomViewport = 0.130000f;
inline constexpr int ContentBottomMax = 150;
inline constexpr int ContentWidthMax = 680;
inline constexpr int ContentViewportSubtract = 68;

inline constexpr int IndexFont = 12;
inline constexpr float IndexTracking = 0.350000f;
inline constexpr int IndexPaddingBottom = 7;
inline constexpr int IndexBorder = 1;

inline constexpr int TitleMin = 66;
inline constexpr float TitleViewport = 0.100000f;
inline constexpr int TitleMax = 138;
inline constexpr int TitleWeight = 900;
inline constexpr float TitleTracking = -0.070000f;
inline constexpr float TitleLineHeight = 0.780000f;
inline constexpr int TitleMarginTop = 14;
inline constexpr int TitleMarginBottom = 12;

inline constexpr int ParagraphFont = 15;
inline constexpr int ParagraphWeight = 600;
inline constexpr float ParagraphLineHeight = 1.600000f;
inline constexpr int ParagraphWidthMax = 520;
inline constexpr int ParagraphMarginTop = 24;
inline constexpr int ParagraphMarginBottom = 30;

inline constexpr int ButtonWidthMax = 360;
inline constexpr int ButtonHeight = 58;
inline constexpr int ButtonPaddingX = 4;
inline constexpr int ButtonFont = 12;
inline constexpr int ButtonWeight = 800;
inline constexpr float ButtonTracking = 0.170000f;
inline constexpr int ArrowFont = 20;

inline constexpr int LoadMarginTop = 14;
inline constexpr int LoadFont = 10;
inline constexpr float LoadTracking = 0.160000f;

inline constexpr int MapRouteOuterThickness = 4;
inline constexpr int FullMapRouteThickness = 2;
inline constexpr int FullMapTitleFont = 26;
inline constexpr int FullMapTitleWeight = 700;
inline constexpr float FullMapTitleTracking = 0.010000f;
inline constexpr int FullMapMetaFont = 11;
inline constexpr int FullMapMetaWeight = 600;
inline constexpr float FullMapMetaTracking = 0.060000f;
inline constexpr int FullMapPlayerSize = 16;
inline constexpr int FullMapWaypointSize = 18;

inline constexpr int MiniMapWallThickness = 1;
inline constexpr int MiniMapRouteThickness = 2;
inline constexpr int MiniMapPlayerFont = 14;
inline constexpr int MiniMapBreakerFont = 12;
inline constexpr int MiniMapExitFont = 12;
inline constexpr int MiniMapThreatFont = 13;
inline constexpr int MiniMapWaypointFont = 15;

inline constexpr CssColor MapBackgroundColor{0.819608f, 0.776471f, 0.435294f, 1.000000f};
inline constexpr CssColor MapFrameColor{0.317647f, 0.290196f, 0.133333f, 1.000000f};
inline constexpr CssColor MapGridColor{0.721569f, 0.678431f, 0.368627f, 1.000000f};
inline constexpr CssColor MapWallColor{0.160784f, 0.149020f, 0.066667f, 1.000000f};
inline constexpr CssColor MapRouteOuterColor{0.290196f, 0.270588f, 0.141176f, 1.000000f};
inline constexpr CssColor FullMapRouteColor{0.184314f, 0.494118f, 0.847059f, 1.000000f};
inline constexpr CssColor FullMapTitleColor{0.145098f, 0.129412f, 0.054902f, 1.000000f};
inline constexpr CssColor FullMapMetaColor{0.396078f, 0.368627f, 0.192157f, 1.000000f};
inline constexpr CssColor FullMapPlayerColor{1.000000f, 0.984314f, 0.890196f, 1.000000f};
inline constexpr CssColor FullMapWaypointColor{0.145098f, 0.533333f, 0.937255f, 1.000000f};

inline constexpr CssColor MiniMapRouteColor{0.184314f, 0.525490f, 0.933333f, 1.000000f};
inline constexpr CssColor MiniMapPlayerColor{1.000000f, 0.984314f, 0.890196f, 1.000000f};
inline constexpr CssColor MiniMapBreakerColor{1.000000f, 0.286275f, 0.211765f, 1.000000f};
inline constexpr CssColor MiniMapBreakerActiveColor{0.207843f, 0.898039f, 0.388235f, 1.000000f};
inline constexpr CssColor MiniMapExitColor{0.968627f, 0.945098f, 0.788235f, 1.000000f};
inline constexpr CssColor MiniMapExitPoweredColor{0.200000f, 0.905882f, 0.415686f, 1.000000f};
inline constexpr CssColor MiniMapThreatColor{1.000000f, 0.231373f, 0.172549f, 1.000000f};
inline constexpr CssColor MiniMapWaypointColor{0.184314f, 0.560784f, 1.000000f, 1.000000f};

inline constexpr CssColor Background{0.784314f, 0.733333f, 0.380392f, 1.000000f};
inline constexpr CssColor MetaColor{0.168627f, 0.149020f, 0.062745f, 0.580000f};
inline constexpr CssColor IndexColor{0.192157f, 0.164706f, 0.058824f, 0.670000f};
inline constexpr CssColor IndexBorderColor{0.192157f, 0.164706f, 0.058824f, 0.420000f};
inline constexpr CssColor TitleColor{0.152941f, 0.137255f, 0.058824f, 1.000000f};
inline constexpr CssColor ParagraphColor{0.168627f, 0.145098f, 0.050980f, 0.720000f};
inline constexpr CssColor ButtonColor{0.164706f, 0.145098f, 0.062745f, 1.000000f};
inline constexpr CssColor ButtonBorderTopColor{0.176471f, 0.152941f, 0.054902f, 0.560000f};
inline constexpr CssColor ButtonBorderBottomColor{0.176471f, 0.152941f, 0.054902f, 0.250000f};
inline constexpr CssColor LoadColor{0.176471f, 0.152941f, 0.054902f, 0.480000f};
}
