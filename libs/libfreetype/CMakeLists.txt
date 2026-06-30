
cmake_minimum_required (VERSION 3.22.1)

project(freetype, LANGUAGES C CXX)

set(freetype_SOURCES)

list(APPEND freetype_SOURCES 
    src/autofit/autofit_freetype.c
    src/autofit/ft-hb.c
    src/base/ftbase_freetype.c
    src/base/ftbbox_freetype.c
    src/base/ftbitmap_freetype.c
    src/base/ftgasp_freetype.c
    src/base/ftglyph_freetype.c
    src/base/ftinit_freetype.c
    src/base/ftmm_freetype.c
    # src/base/ftobjs_freetype.c
    src/base/ftpfr_freetype.c
    src/base/ftstroke_freetype.c
    src/base/ftsynth_freetype.c
    src/base/ftsystem_freetype.c
    src/base/fttype1_freetype.c
    src/base/ftwinfnt_freetype.c

    src/bdf/bdf_freetype.c
    src/cache/ftcache_freetype.c
    src/ftlinux_freetype.c
    src/winfonts/winfnt_freetype.c
    src/type42/type42_freetype.c
    src/type1/type1_freetype.c
    src/truetype/truetype_freetype.c
    src/smooth/smooth_freetype.c
    src/sfnt/sfnt_freetype.c
    src/raster/raster_freetype.c
    src/psnames/psmodule_freetype.c
    src/pshinter/pshinter_freetype.c
    src/psaux/psaux_freetype.c
    src/pfr/pfr_freetype.c
    src/pcf/pcf_freetype.c
    src/lzw/ftlzw_freetype.c
    src/gzip/ftgzip_freetype.c
    src/cid/type1cid_freetype.c
    src/cff/cff_freetype.c
    harfbuzz/src/failing-alloc.c
    harfbuzz/src/harfbuzz.cc
    harfbuzz/src/hb-cairo-utils.cc
    harfbuzz/src/hb-cairo.cc
    harfbuzz/src/hb-gobject-structs.cc
)

if(DEFINED TARGET_OS_WINDOWS)
    list(APPEND freetype_SOURCES
        src/ftdebug.c
    )
endif()

add_library(freetype STATIC ${freetype_SOURCES})

if(DEFINED TARGET_OS_WINDOWS)
    target_compile_definitions(freetype
        PRIVATE
        TARGET_OS_WINDOWS
        LIB_STATIC
        WIN64
        _WIN64
        _LIB
        FT_DEBUG_LEVEL_ERROR
        FT_DEBUG_LEVEL_TRACE
        FT2_BUILD_LIBRARY
        __STDC_CONSTANT_MACROS
        _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1
        _CRT_SECURE_NO_WARNINGS
        _CRT_NONSTDC_NO_DEPRECATE
    )

    # TARGET_OS_WINDOWS;LIB_STATIC;_DEBUG;WIN64;_WIN64;_LIB;
    # FT_DEBUG_LEVEL_ERROR;FT_DEBUG_LEVEL_TRACE;FT2_BUILD_LIBRARY;__STDC_CONSTANT_MACROS;_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1;_CRT_SECURE_NO_WARNINGS;_CRT_NONSTDC_NO_DEPRECATE;%(PreprocessorDefinitions)
    
    set_property(TARGET freetype PROPERTY C_STANDARD 90)
    set_property(TARGET freetype PROPERTY C_STANDARD_REQUIRED OFF)
    set_property(TARGET freetype PROPERTY CXX_STANDARD 14)
    set_property(TARGET freetype PROPERTY CXX_STANDARD_REQUIRED OFF)
else()
    nlc_target_add_supported_warning_options(freetype C
        -Wno-undefined-internal
        -Wno-deprecated-declarations
    )

    list(APPEND freetype_SOURCES
        src/base/ftobjs_freetype.c
    )

endif()



