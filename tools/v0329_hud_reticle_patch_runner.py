from pathlib import Path

PathValue = Path("tools/v0329_hud_reticle_patch.py")
Source = PathValue.read_text(encoding="utf-8")
Source = Source.replace("if count != 6:", "if count != 7:")
Source = Source.replace("expected 6 gameplay HUD text draws", "expected 7 gameplay HUD text draws")
exec(compile(Source, str(PathValue), "exec"))
