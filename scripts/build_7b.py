Import("env")
from pathlib import Path
root = Path(env.subst("$PROJECT_DIR"))
env.Append(CPPPATH=[str(root / "ui/7b"), str(root / "firmware/7b"),
                   str(root / "firmware/7b/bms/include"),
                   str(root / "firmware/7b/vesc/include"),
                   str(root / "Super_VESC_Display/lvgl")])
env.BuildSources("$BUILD_DIR/ui_7b", str(root / "ui/7b"), src_filter=["+<*>", "-<fonts/>"])
env.BuildSources("$BUILD_DIR/lvgl", str(root / "Super_VESC_Display/lvgl/src"))
fonts = root / "ui/7b/fonts"
env.BuildSources("$BUILD_DIR/fonts", str(fonts), src_filter=[
    "-<*>", *[f"+<lv_font_Antonio_Regular_{size}.c>" for size in (32,40,64,200)]])
