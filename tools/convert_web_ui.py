#!/usr/bin/env python3
"""Convert the original browser style.css into native C++ UI constants.

This intentionally supports only the CSS constructs used by Backrooms Offical's
menu/HUD. The browser CSS remains the source of truth; native code consumes the
generated header instead of hand-copying/guessing measurements.
"""

from __future__ import annotations

import argparse
import re
from pathlib import Path

BLOCK_RE = re.compile(r"([^{}]+)\{([^{}]*)\}", re.S)
PROP_RE = re.compile(r"([\w-]+)\s*:\s*([^;]+);", re.S)


def blocks(css: str, selector_contains: str):
    for match in BLOCK_RE.finditer(css):
        selector = " ".join(match.group(1).split())
        if selector_contains in selector:
            props = {k.strip(): " ".join(v.split()) for k, v in PROP_RE.findall(match.group(2))}
            yield props


def prop(css: str, selector: str, name: str, default: str) -> str:
    result = default
    for item in blocks(css, selector):
        if name in item:
            result = item[name]
    return result


def px(value: str) -> int:
    match = re.search(r"(-?\d+(?:\.\d+)?)px", value)
    if not match:
        raise ValueError(f"Expected px value: {value!r}")
    return int(round(float(match.group(1))))


def number(value: str) -> float:
    match = re.search(r"-?\d+(?:\.\d+)?", value)
    if not match:
        raise ValueError(f"Expected number: {value!r}")
    return float(match.group(0))


def em(value: str) -> float:
    match = re.search(r"(-?\d+(?:\.\d+)?)em", value)
    if not match:
        raise ValueError(f"Expected em value: {value!r}")
    return float(match.group(1))


def clamp_value(value: str, unit: str):
    pattern = rf"clamp\(\s*(-?\d+(?:\.\d+)?)px\s*,\s*(-?\d+(?:\.\d+)?){unit}\s*,\s*(-?\d+(?:\.\d+)?)px\s*\)"
    match = re.search(pattern, value)
    if not match:
        raise ValueError(f"Expected clamp(... {unit} ...): {value!r}")
    return float(match.group(1)), float(match.group(2)) / 100.0, float(match.group(3))


def min_px(value: str) -> int:
    match = re.search(r"min\(\s*(\d+)px", value)
    if not match:
        raise ValueError(f"Expected min(px,...): {value!r}")
    return int(match.group(1))


def rgba(value: str, fallback=(1.0, 1.0, 1.0, 1.0)):
    match = re.search(r"rgba\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*([\d.]+)\s*\)", value)
    if match:
        r, g, b = (int(match.group(i)) / 255.0 for i in range(1, 4))
        return r, g, b, float(match.group(4))
    match = re.search(r"#([0-9a-fA-F]{6})", value)
    if match:
        raw = match.group(1)
        return tuple(int(raw[i:i+2], 16) / 255.0 for i in (0, 2, 4)) + (1.0,)
    return fallback


def fmt(v: float) -> str:
    return f"{v:.6f}f"


def color(name: str, c) -> str:
    return f"inline constexpr CssColor {name}{{{fmt(c[0])}, {fmt(c[1])}, {fmt(c[2])}, {fmt(c[3])}}};"


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("css", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    css = args.css.read_text(encoding="utf-8")

    content_left = clamp_value(prop(css, ".MenuContent", "left", "clamp(34px, 7vw, 110px)"), "vw")
    content_bottom = clamp_value(prop(css, ".MenuContent", "bottom", "clamp(74px, 13vh, 150px)"), "vh")
    title_clamp = clamp_value(prop(css, "#StartScreen .MenuContent h1", "font-size", "clamp(66px, 10vw, 138px)"), "vw")

    base_title_margin = prop(css, ".MenuContent h1", "margin", "14px 0 12px")
    margin_numbers = [int(round(float(x))) for x in re.findall(r"(-?\d+(?:\.\d+)?)px", base_title_margin)]
    title_margin_top = margin_numbers[0] if margin_numbers else 14
    title_margin_bottom = margin_numbers[1] if len(margin_numbers) > 1 else 12

    p_margin = prop(css, ".MenuContent p", "margin", "24px 0 30px")
    p_margin_numbers = [int(round(float(x))) for x in re.findall(r"(-?\d+(?:\.\d+)?)px", p_margin)]
    paragraph_margin_top = p_margin_numbers[0] if p_margin_numbers else 24
    paragraph_margin_bottom = p_margin_numbers[1] if len(p_margin_numbers) > 1 else 30

    top_color = rgba(prop(css, "#StartScreen .MenuTop", "color", "rgba(43,38,16,0.58)"))
    index_color = rgba(prop(css, "#StartScreen .MenuIndex", "color", "rgba(49,42,15,0.67)"))
    index_border = rgba(prop(css, "#StartScreen .MenuIndex", "border-color", "rgba(49,42,15,0.42)"))
    title_color = rgba(prop(css, "#StartScreen .MenuContent h1", "color", "#27230f"))
    paragraph_color = rgba(prop(css, "#StartScreen .MenuContent p", "color", "rgba(43,37,13,0.72)"))
    button_color = rgba(prop(css, "#StartScreen #StartButton", "color", "#2a2510"))
    button_top = rgba(prop(css, "#StartScreen #StartButton", "border-top-color", "rgba(45,39,14,0.56)"))
    button_bottom = rgba(prop(css, "#StartScreen #StartButton", "border-bottom-color", "rgba(45,39,14,0.25)"))
    load_color = rgba(prop(css, "#StartScreen #LoadStatus", "color", "rgba(45,39,14,0.48)"))

    # The final StartScreen background declaration ends with the actual flat CSS base color.
    start_blocks = list(blocks(css, "#StartScreen"))
    bg_value = "#c8bb61"
    for item in start_blocks:
        if "background" in item and "#c8bb61" in item["background"]:
            bg_value = "#c8bb61"
    background_color = rgba(bg_value)

    out = f'''#pragma once

// AUTO-GENERATED from assets/ui/original/style.css by tools/convert_web_ui.py.
// Do not hand-tune these values. Change the original CSS source and regenerate.
namespace GeneratedWebUi
{{
struct CssColor {{ float R; float G; float B; float A; }};

inline constexpr int MenuMargin = {px(prop(css, '.MenuTop', 'left', '34px'))};
inline constexpr int MenuTopY = {px(prop(css, '.MenuTop', 'top', '28px'))};
inline constexpr int MenuFooterBottom = {px(prop(css, '.MenuFooter', 'bottom', '24px'))};
inline constexpr int MenuMetaFont = {px(prop(css, '.MenuTop', 'font-size', '10px'))};
inline constexpr float MenuMetaTracking = {fmt(em(prop(css, '.MenuTop', 'letter-spacing', '0.2em')))};

inline constexpr int ContentLeftMin = {int(content_left[0])};
inline constexpr float ContentLeftViewport = {fmt(content_left[1])};
inline constexpr int ContentLeftMax = {int(content_left[2])};
inline constexpr int ContentBottomMin = {int(content_bottom[0])};
inline constexpr float ContentBottomViewport = {fmt(content_bottom[1])};
inline constexpr int ContentBottomMax = {int(content_bottom[2])};
inline constexpr int ContentWidthMax = {min_px(prop(css, '.MenuContent', 'width', 'min(680px, calc(100vw - 68px))'))};
inline constexpr int ContentViewportSubtract = 68;

inline constexpr int IndexFont = {px(prop(css, '.MenuIndex', 'font-size', '12px'))};
inline constexpr float IndexTracking = {fmt(em(prop(css, '.MenuIndex', 'letter-spacing', '0.35em')))};
inline constexpr int IndexPaddingBottom = {px(prop(css, '.MenuIndex', 'padding-bottom', '7px'))};
inline constexpr int IndexBorder = 1;

inline constexpr int TitleMin = {int(title_clamp[0])};
inline constexpr float TitleViewport = {fmt(title_clamp[1])};
inline constexpr int TitleMax = {int(title_clamp[2])};
inline constexpr int TitleWeight = {int(number(prop(css, '.MenuContent h1', 'font-weight', '900')))};
inline constexpr float TitleTracking = {fmt(em(prop(css, '#StartScreen .MenuContent h1', 'letter-spacing', '-0.07em')))};
inline constexpr float TitleLineHeight = {fmt(number(prop(css, '.MenuContent h1', 'line-height', '0.78')))};
inline constexpr int TitleMarginTop = {title_margin_top};
inline constexpr int TitleMarginBottom = {title_margin_bottom};

inline constexpr int ParagraphFont = {px(prop(css, '.MenuContent p', 'font-size', '15px'))};
inline constexpr int ParagraphWeight = {int(number(prop(css, '#StartScreen .MenuContent p', 'font-weight', '600')))};
inline constexpr float ParagraphLineHeight = {fmt(number(prop(css, '.MenuContent p', 'line-height', '1.6')))};
inline constexpr int ParagraphWidthMax = {min_px(prop(css, '.MenuContent p', 'width', 'min(520px, 100%)'))};
inline constexpr int ParagraphMarginTop = {paragraph_margin_top};
inline constexpr int ParagraphMarginBottom = {paragraph_margin_bottom};

inline constexpr int ButtonWidthMax = {min_px(prop(css, '#StartButton', 'width', 'min(360px, 100%)'))};
inline constexpr int ButtonHeight = {px(prop(css, '#StartButton', 'min-height', '58px'))};
inline constexpr int ButtonPaddingX = 4;
inline constexpr int ButtonFont = {px(prop(css, '#StartButton', 'font-size', '12px'))};
inline constexpr int ButtonWeight = {int(number(prop(css, '#StartButton', 'font-weight', '800')))};
inline constexpr float ButtonTracking = {fmt(em(prop(css, '#StartButton', 'letter-spacing', '0.17em')))};
inline constexpr int ArrowFont = {px(prop(css, '.ButtonArrow', 'font-size', '20px'))};

inline constexpr int LoadMarginTop = {px(prop(css, '#LoadStatus', 'margin-top', '14px'))};
inline constexpr int LoadFont = {px(prop(css, '#LoadStatus', 'font-size', '10px'))};
inline constexpr float LoadTracking = {fmt(em(prop(css, '#LoadStatus', 'letter-spacing', '0.16em')))};

{color('Background', background_color)}
{color('MetaColor', top_color)}
{color('IndexColor', index_color)}
{color('IndexBorderColor', index_border)}
{color('TitleColor', title_color)}
{color('ParagraphColor', paragraph_color)}
{color('ButtonColor', button_color)}
{color('ButtonBorderTopColor', button_top)}
{color('ButtonBorderBottomColor', button_bottom)}
{color('LoadColor', load_color)}
}}
'''

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(out, encoding="utf-8", newline="\n")


if __name__ == "__main__":
    main()
