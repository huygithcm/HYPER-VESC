cmake_minimum_required(VERSION 3.20)
project(vesc_7b_simulator C)
set(CMAKE_C_STANDARD 11)
get_filename_component(ROOT "../.." ABSOLUTE)
set(UI "${ROOT}/Super_VESC_Display")
set(SEED "${ROOT}/esp32s3-vesc-bms/Super_VESC_Display")
file(GLOB_RECURSE LV_SOURCES CONFIGURE_DEPENDS "${UI}/lvgl/src/*.c")
set(FONTS)
foreach(SIZE 32 40 64 200)
    list(APPEND FONTS "${SEED}/generated/guider_fonts/lv_font_Antonio_Regular_${SIZE}.c")
endforeach()
add_executable(vesc_7b_simulator WIN32 main.c "${ROOT}/ui/7b/preview_ui.c" ${LV_SOURCES} ${FONTS}
    "${UI}/ports/linux/lv_drivers/win32drv/win32drv.c")
target_include_directories(vesc_7b_simulator PRIVATE . "${ROOT}/ui/7b" "${UI}" "${UI}/lvgl"
    "${UI}/ports/linux/lv_drivers/win32drv")
target_compile_definitions(vesc_7b_simulator PRIVATE LV_CONF_INCLUDE_SIMPLE
    LV_CONF_PATH=${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h LV_DRV_NO_CONF USE_WIN32DRV=1
    LV_LVGL_H_INCLUDE_SIMPLE _CRT_SECURE_NO_WARNINGS UNICODE _UNICODE)
target_link_libraries(vesc_7b_simulator PRIVATE user32 gdi32)
if(MINGW)
    target_link_options(vesc_7b_simulator PRIVATE -static)
endif()
