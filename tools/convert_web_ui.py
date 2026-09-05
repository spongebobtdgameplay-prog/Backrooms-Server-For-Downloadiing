#!/usr/bin/env python3
"""Convert the original browser style.css into native C++ UI constants.

The browser CSS is the source of truth. This script converts the subset of CSS
used by Backrooms Offical's start menu/HUD into a generated C++ header so the
native UI stops hand-copying and guessing positions/sizes.
"""

from __future__ import annotations

import argparse
import re
from pathlib import Path

BLOCK_RE = re.compile(r"([^{}]+)\{([^{}]*)\}", re.S)
PROP_RE = re.compile(r"([\w-]+)\s*:\s*([^;]+);", re.S)


def split_media(css: str):
    base_parts: list[str] = []
    media_parts: list[str] = []
    cursor = 0
    while True:
        start = css.find("@media", cursor)
        if start < 0:
            base_parts.append(css[cursor:])
            break
        base_parts.append(css[cursor:start])
        open_brace = css.find("{", start)
        if open_brace < 0:
            base_parts.append(css[start:])
            break
        depth = 1
        pos = open_brace + 1
        while pos < len(css) and depth:
            if css[pos] == "{":
                depth += 1
            elif css[pos] == "}":
                depth -= 1
            pos += 1
        header = css[start:open_brace]
        body = css[open_brace + 1:pos - 1]
        if "max-width: 700px" in header:
            media_parts.append(body)
        cursor = pos
    return "\n".join(base_parts), "\n".join(media_parts)


def selector_matches(rule_selector: str, target: str) -> bool:
    for raw_selector in rule_selector.split(","):
        selector = " ".join(raw_selector.split())
        if selector == target:
            return True
        if selector.endswith(" " + target):
            return True
    return False


def blocks(css: str, selector: str):
    for match in BLOCK_RE.finditer(css):
        rule_selector = " ".join(match.group(1).split())
        if selector_matches(rule_selector, selector):
            props = {
                k.strip(): " ".join(v.split())
                for k, v in PROP_RE.findall(match.group(2))
            }
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
    match = re.search(
        r"rgba\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*([\d.]+)\s*\)",
        value,
    )
    if match:
        r, g, b = (int(match.group(i)) / 255.0 for i in range(1, 4))
        return r, g, b, float(match.group(4))
    match = re.search(r"#([0-9a-fA-F]{6})", value)
    if match:
        raw = match.group(1)
        return tuple(int(raw[i:i + 2], 16) / 255.0 for i in (0, 2, 4)) + (1.0,)
    return fallback


def last_hex(value: str, fallback=(1.0, 1.0, 1.0, 1.0)):
    matches = re.findall(r"#([0-9a-fA-F]{6})", value)
    if not matches:
        return fallback
    raw = matches[-1]
    return tuple(int(raw[i:i + 2], 16) / 255.0 for i in (0, 2, 4)) + (1.0,)


def fmt(v: float) -> str:
    return f"{v:.6f}f"


def color(name: str, c) -> str:
    return (
        f"inline constexpr CssColor {name}{{{fmt(c[0])}, {fmt(c[1])}, "
        f"{fmt(c[2])}, {fmt(c[3])}}};"
    )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("css", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    css = args.css.read_text(encoding="utf-8")
    base_css, mobile_css = split_media(css)

    content_left = clamp_value(
        prop(base_css, ".MenuContent", "left", "clamp(34px, 7vw, 110px)"),
        "vw",
    )
    content_bottom = clamp_value(
        prop(base_css, ".MenuContent", "bottom", "clamp(74px, 13vh, 150px)"),
        "vh",
    )
    title_clamp = clamp_value(
        prop(base_css, ".MenuContent h1", "font-size", "clamp(66px, 11vw, 150px)"),
        "vw",
    )
    mobile_title = clamp_value(
        prop(mobile_css, ".MenuContent h1", "font-size", "clamp(58px, 21vw, 100px)"),
        "vw",
    )

    base_title_margin = prop(base_css, ".MenuContent h1", "margin", "14px 0 12px")
    margin_numbers = [
        int(round(float(x)))
        for x in re.findall(r"(-?\d+(?:\.\d+)?)px", base_title_margin)
    ]
    title_margin_top = margin_numbers[0] if margin_numbers else 14
    title_margin_bottom = margin_numbers[1] if len(margin_numbers) > 1 else 12

    p_margin = prop(base_css, ".MenuContent p", "margin", "24px 0 30px")
    p_margin_numbers = [
        int(round(float(x)))
        for x in re.findall(r"(-?\d+(?:\.\d+)?)px", p_margin)
    ]
    paragraph_margin_top = p_margin_numbers[0] if p_margin_numbers else 24
    paragraph_margin_bottom = p_margin_numbers[1] if len(p_margin_numbers) > 1 else 30

    top_color = rgba(prop(base_css, ".MenuTop", "color", "rgba(232,224,165,0.46)"))
    index_color = rgba(prop(base_css, ".MenuIndex", "color", "rgba(240,231,162,0.62)"))
    index_border = rgba(
        prop(
            base_css,
            ".MenuIndex",
            "border-color",
            prop(base_css, ".MenuIndex", "border-bottom", "rgba(236,226,151,0.42)"),
        )
    )
    title_color = rgba(prop(base_css, ".MenuContent h1", "color", "#e2d993"))
    paragraph_color = rgba(prop(base_css, ".MenuContent p", "color", "rgba(233,226,173,0.66)"))
    button_color = rgba(prop(base_css, "#StartButton", "color", "#f3ebaf"))
    button_top = rgba(
        prop(
            base_css,
            "#StartButton",
            "border-top-color",
            prop(base_css, "#StartButton", "border-top", "rgba(241,232,160,0.58)"),
        )
    )
    button_bottom = rgba(
        prop(
            base_css,
            "#StartButton",
            "border-bottom-color",
            prop(base_css, "#StartButton", "border-bottom", "rgba(241,232,160,0.2)"),
        )
    )
    load_color = rgba(prop(base_css, "#LoadStatus", "color", "rgba(232,224,165,0.34)"))
    background_color = last_hex(
        prop(base_css, "#StartScreen", "background", "#c8bb61"),
        (200.0 / 255.0, 187.0 / 255.0, 97.0 / 255.0, 1.0),
    )

    mobile_margin = px(prop(mobile_css, ".MenuTop", "left", "20px"))
    mobile_content_left = px(prop(mobile_css, ".MenuContent", "left", "24px"))

    out = f'''#pragma once

// AUTO-GENERATED from assets/ui/original/style.css by tools/convert_web_ui.py.
// Do not hand-tune these values. Change the original CSS source and regenerate.
namespace GeneratedWebUi
{{
struct CssColor {{ float R; float G; float B; float A; }};

inline constexpr int MenuMargin = {px(prop(base_css, '.MenuTop', 'left', '34px'))};
inline constexpr int MenuTopY = {px(prop(base_css, '.MenuTop', 'top', '28px'))};
inline constexpr int MenuFooterBottom = {px(prop(base_css, '.MenuFooter', 'bottom', '24px'))};
inline constexpr int MenuMetaFont = {px(prop(base_css, '.MenuTop', 'font-size', '10px'))};
inline constexpr float MenuMetaTracking = {fmt(em(prop(base_css, '.MenuTop', 'letter-spacing', '0.2em')))};

inline constexpr int MobileBreakpoint = 700;
inline constexpr int MobileMenuMargin = {mobile_margin};
inline constexpr int MobileContentLeft = {mobile_content_left};
inline constexpr int MobileContentViewportSubtract = 48;
inline constexpr int MobileTitleMin = {int(mobile_title[0])};
inline constexpr float MobileTitleViewport = {fmt(mobile_title[1])};
inline constexpr int MobileTitleMax = {int(mobile_title[2])};

inline constexpr int ContentLeftMin = {int(content_left[0])};
inline constexpr float ContentLeftViewport = {fmt(content_left[1])};
inline constexpr int ContentLeftMax = {int(content_left[2])};
inline constexpr int ContentBottomMin = {int(content_bottom[0])};
inline constexpr float ContentBottomViewport = {fmt(content_bottom[1])};
inline constexpr int ContentBottomMax = {int(content_bottom[2])};
inline constexpr int ContentWidthMax = {min_px(prop(base_css, '.MenuContent', 'width', 'min(680px, calc(100vw - 68px))'))};
inline constexpr int ContentViewportSubtract = 68;

inline constexpr int IndexFont = {px(prop(base_css, '.MenuIndex', 'font-size', '12px'))};
inline constexpr float IndexTracking = {fmt(em(prop(base_css, '.MenuIndex', 'letter-spacing', '0.35em')))};
inline constexpr int IndexPaddingBottom = {px(prop(base_css, '.MenuIndex', 'padding-bottom', '7px'))};
inline constexpr int IndexBorder = 1;

inline constexpr int TitleMin = {int(title_clamp[0])};
inline constexpr float TitleViewport = {fmt(title_clamp[1])};
inline constexpr int TitleMax = {int(title_clamp[2])};
inline constexpr int TitleWeight = {int(number(prop(base_css, '.MenuContent h1', 'font-weight', '900')))};
inline constexpr float TitleTracking = {fmt(em(prop(base_css, '.MenuContent h1', 'letter-spacing', '-0.075em')))};
inline constexpr float TitleLineHeight = {fmt(number(prop(base_css, '.MenuContent h1', 'line-height', '0.78')))};
inline constexpr int TitleMarginTop = {title_margin_top};
inline constexpr int TitleMarginBottom = {title_margin_bottom};

inline constexpr int ParagraphFont = {px(prop(base_css, '.MenuContent p', 'font-size', '15px'))};
inline constexpr int ParagraphWeight = {int(number(prop(base_css, '.MenuContent p', 'font-weight', '400')))};
inline constexpr float ParagraphLineHeight = {fmt(number(prop(base_css, '.MenuContent p', 'line-height', '1.6')))};
inline constexpr int ParagraphWidthMax = {min_px(prop(base_css, '.MenuContent p', 'width', 'min(520px, 100%)'))};
inline constexpr int ParagraphMarginTop = {paragraph_margin_top};
inline constexpr int ParagraphMarginBottom = {paragraph_margin_bottom};

inline constexpr int ButtonWidthMax = {min_px(prop(base_css, '#StartButton', 'width', 'min(360px, 100%)'))};
inline constexpr int ButtonHeight = {px(prop(base_css, '#StartButton', 'min-height', '58px'))};
inline constexpr int ButtonPaddingX = 4;
inline constexpr int ButtonFont = {px(prop(base_css, '#StartButton', 'font-size', '12px'))};
inline constexpr int ButtonWeight = {int(number(prop(base_css, '#StartButton', 'font-weight', '800')))};
inline constexpr float ButtonTracking = {fmt(em(prop(base_css, '#StartButton', 'letter-spacing', '0.17em')))};
inline constexpr int ArrowFont = {px(prop(base_css, '.ButtonArrow', 'font-size', '20px'))};

inline constexpr int LoadMarginTop = {px(prop(base_css, '#LoadStatus', 'margin-top', '14px'))};
inline constexpr int LoadFont = {px(prop(base_css, '#LoadStatus', 'font-size', '10px'))};
inline constexpr float LoadTracking = {fmt(em(prop(base_css, '#LoadStatus', 'letter-spacing', '0.16em')))};

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
