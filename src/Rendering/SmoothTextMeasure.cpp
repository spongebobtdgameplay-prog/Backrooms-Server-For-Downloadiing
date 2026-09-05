#include "SmoothTextRenderer.h"

#include <algorithm>

int SmoothTextRenderer::MeasureHeight(
    const std::string& Text,
    int PixelHeight,
    int Weight,
    float TrackingEm
)
{
    CachedText* Entry =
        GetOrCreate(
            Text,
            PixelHeight,
            Weight,
            TrackingEm
        );

    return Entry != nullptr
        ? std::max(Entry->Height - 12, 0)
        : 0;
}
