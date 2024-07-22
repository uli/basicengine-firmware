LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := ../SDL

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include

# Add your application source files here...
LOCAL_SRC_FILES := \
	arduino_compat/WString.cpp \
	arduino_compat/core_esp8266_noniso.cpp \
	arduino_compat/noniso.cpp \
	libraries/TTBAS_LIB/ps22tty.cpp \
	libraries/TTBAS_LIB/sdfiles.cpp \
	libraries/TTBAS_LIB/tGraphicDev.cpp \
	libraries/TTBAS_LIB/tTVscreen.cpp \
	libraries/TTBAS_LIB/tTVscreen_ansi.cpp \
	libraries/TTBAS_LIB/tscreenBase.cpp \
	libraries/TTBAS_LIB/tscreenBase_ansi.cpp \
	libraries/TTBAS_LIB/tvutil.cpp \
	libraries/TTBAS_LIB/unifile_stdio.cpp \
	libraries/TTVoutfonts/font6x8tt.cpp \
	libraries/TTVoutfonts/fonts.cpp \
	libraries/TTVoutfonts/hp100lx_8x8.cpp \
	libraries/alpha-lib/src/overlay_alpha.cpp \
	libraries/dyncall/dyncall/dyncall_vector.c \
	libraries/dyncall/dyncall/dyncall_api.c \
	libraries/dyncall/dyncall/dyncall_callvm.c \
	libraries/dyncall/dyncall/dyncall_callvm_base.c \
	libraries/dyncall/dyncall/dyncall_call.S \
	libraries/dyncall/dyncall/dyncall_callf.c \
	libraries/dyncall/dyncall/dyncall_aggregate.c \
	libraries/stb/miniz.c \
	libraries/stb/stb_image.c \
	libraries/stb/stb_image_resize.c \
	libraries/stb/stb_image_write.c \
	libraries/stb/sts_mixer.c \
	libraries/stb/utf8.c \
	libraries/tinycc/libtcc.c \
	sdl/Arduino.cpp \
	sdl/Time.cpp \
	sdl/sdlaudio.cpp \
	sdl/sdlgfx.cpp \
	sdl/sdljoy.cpp \
	sdl/TKeyboard.cpp \
	ttbasic/BString.cpp \
	ttbasic/GuillotineBinPack.cpp \
	ttbasic/Rect.cpp \
	ttbasic/amstrad_8x8.cpp \
	ttbasic/basic.cpp \
	ttbasic/basic_bg.cpp \
	ttbasic/basic_config.cpp \
	ttbasic/basic_file.cpp \
	ttbasic/basic_help.cpp \
	ttbasic/basic_input.cpp \
	ttbasic/basic_io.cpp \
	ttbasic/basic_math.cpp \
	ttbasic/basic_native.cpp \
	ttbasic/basic_sound.cpp \
	ttbasic/basic_string.cpp \
	ttbasic/basic_sys.cpp \
	ttbasic/basic_sys_linux.cpp \
	ttbasic/basic_var.cpp \
	ttbasic/basic_video.cpp \
	ttbasic/bgengine.cpp \
	ttbasic/cbm_ascii_8x8.cpp \
	ttbasic/colorspace.cpp \
	ttbasic/compat.c \
	ttbasic/eb_basic.cpp \
	ttbasic/eb_bg.cpp \
	ttbasic/eb_config.cpp \
	ttbasic/eb_conio.cpp \
	ttbasic/eb_file.cpp \
	ttbasic/eb_img.cpp \
	ttbasic/eb_input.cpp \
	ttbasic/eb_io.cpp \
	ttbasic/eb_io_linux.cpp \
	ttbasic/eb_native.cpp \
	ttbasic/eb_sound.cpp \
	ttbasic/eb_sys.cpp \
	ttbasic/eb_sys_linux.cpp \
	ttbasic/eb_video.cpp \
	ttbasic/export_syms.c \
	ttbasic/lexis.cpp \
	ttbasic/mml.c \
	ttbasic/mml_stack.c \
	ttbasic/rotozoom.cpp \
	ttbasic/sound.cpp \
	ttbasic/translate.cpp \
	ttbasic/ttbasic.cpp \
	ttbasic/update.cpp \
	ttbasic/video.cpp \
	ttbasic/vs23_gfx.cpp

LOCAL_SHARED_LIBRARIES := SDL2

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -lOpenSLES -llog -landroid -lc++

LOCAL_CFLAGS += -DSDL -DANDROID -Ijni/src/ttbasic -Ijni/src/sdl \
	-Ijni/src/arduino_compat -Ijni/src/libraries/TKeyboard/src \
	-Ijni/src/libraries/stb -Ijni/src/libraries/TTBAS_LIB \
	-Ijni/src/libraries/TTVoutfonts \
	-Ijni/src/libraries/alpha-lib/include \
	-Ijni/src/gfx -Ijni/src/libraries/tinycc \
	-Ijni/src/libraries/dyncall/dyncall

#LOCAL_CXXFLAGS += -DSDL -Ijni/src/sdl

include $(BUILD_SHARED_LIBRARY)
