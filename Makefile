#
# Makefile
#
#命令行执行下面三行
#export PATH=$PATH:/home/ubuntu/100ask/T113s4_Lvgl9-Class_TinaSDK-V1/prebuilt/rootfsbuilt/arm/toolchain-sunxi-glibc-gcc-830/toolchain/bin
#export CROSS_COMPILE=arm-openwrt-linux-gnueabi
#export STAGING_DIR=/home/ubuntu/100ask/T113s4_Lvgl9-Class_TinaSDK-V1/out/t113_s4/100ask_lvgl9/openwrt/staging_dir/target

CC 			?= gcc
CXX 			?= g++
#CC 				= aarch64-linux-gnu-gcc
#CXX 			= aarch64-linux-gnu-g++
LVGL_DIR_NAME 	?= lvgl
LVGL_DIR 		?= .

WARNINGS		:= -Wall -Wshadow -Wundef -Wmissing-prototypes -Wno-discarded-qualifiers -Wextra -Wno-unused-function -Wno-error=strict-prototypes -Wpointer-arith \
					-fno-strict-aliasing -Wno-error=cpp -Wuninitialized -Wmaybe-uninitialized -Wno-unused-parameter -Wno-missing-field-initializers -Wtype-limits \
					-Wsizeof-pointer-memaccess -Wno-format-nonliteral -Wno-cast-qual -Wunreachable-code -Wno-switch-default -Wreturn-type -Wmultichar -Wformat-security \
					-Wno-ignored-qualifiers -Wno-error=pedantic -Wno-sign-compare -Wno-error=missing-prototypes -Wdouble-promotion -Wclobbered -Wdeprecated -Wempty-body \
					-Wshift-negative-value -Wstack-usage=2048 -Wno-unused-value -std=gnu99
CFLAGS 			?= -O0 -g0 -I$(LVGL_DIR)/ $(WARNINGS)

#CFLAGS += -I$(LVGL_DIR)/mqtt/kawaii-mqtt/common/log \
#          -I$(LVGL_DIR)/mqtt/kawaii-mqtt/common \
#          -I$(LVGL_DIR)/mqtt/kawaii-mqtt/platform/linux \
#          -I$(LVGL_DIR)/mqtt/kawaii-mqtt/mqtt \
#          -I$(LVGL_DIR)/mqtt/kawaii-mqtt/network/mbedtls/include \
#          -I$(LVGL_DIR)/mqtt/kawaii-mqtt/network \
#          -I$(LVGL_DIR)/mqtt/kawaii-mqtt/mqttclient \
#          -I$(LVGL_DIR)/mqtt/kawaii-mqtt/network/mbedtls/wrapper \
#          -I$(LVGL_DIR)/mqtt/kawaii-mqtt/network/mbedtls/include/mbedtls \
#          -I$(LVGL_DIR)/mqtt/include

LDFLAGS 		?= -lm -lstdc++ -lavformat -lavcodec -lavutil -lswscale -lz -lpthread
LDFLAGS 		+= -lpigpio -lrt
BIN 			= lvgl_YOLO
BUILD_DIR 		= ./build
BUILD_OBJ_DIR 	= $(BUILD_DIR)/obj
BUILD_BIN_DIR 	= $(BUILD_DIR)/bin

prefix 			?= /usr
bindir 			?= $(prefix)/bin

#Collect the files to compile
MAINSRC          = ./main.c

include $(LVGL_DIR)/lvgl/lvgl.mk

# 不编译未使用的 LVGL 示例和演示程序
#CSRCS := $(filter-out $(LVGL_PATH)/demos/%,$(CSRCS))
#CSRCS := $(filter-out $(LVGL_PATH)/examples/%,$(CSRCS))

# lv_conf.h 中 ThorVG 已关闭，不编译其 C++ 源码
#CXXSRCS := $(filter-out $(LVGL_PATH)/src/libs/thorvg/%,$(CXXSRCS))

CSRCS 			+=$(LVGL_DIR)/mouse_cursor_icon.c 
CSRCS 			+=$(LVGL_DIR)/ui_main.c 
CSRCS 			+=$(LVGL_DIR)/page1.c 
CSRCS 			+=$(LVGL_DIR)/page2.c 
CSRCS 			+=$(LVGL_DIR)/page_pic.c 
CSRCS 			+=$(LVGL_DIR)/page_rain.c 
#CSRCS 			+=$(LVGL_DIR)/mqtt.c 
CSRCS 			+=$(LVGL_DIR)/serial_init.c 
CSRCS 			+=$(LVGL_DIR)/servo_control.c 

# FreeType custom configuration header file
 CFLAGS += -DFT2_BUILD_LIBRARY
 #CFLAGS += -DFT_CONFIG_MODULES_H="<lvgl/src/libs/freetype/ftmodule.h>"
 #CFLAGS += -DFT_CONFIG_OPTIONS_H="<lvgl/src/libs/freetype/ftoption.h>"
 #CFLAGS += -DFT_CONFIG_MODULES_H="<freetype/config/ftmodule.h>"
 #CFLAGS += -DFT_CONFIG_OPTIONS_H="<freetype/config/ftoption.h>"
 # FreeType include path
 CFLAGS += -Ifreetype/include
 # FreeType C source file
 # 如果在 lv_conf.h 中打开了宏 LV_FREETYPE_USE_LVGL_PORT 那么需要去掉 ftsystem.c
#  FT_CSRCS += freetype/src/base/ftsystem.c
 FT_CSRCS += freetype/src/autofit/autofit.c
 FT_CSRCS += freetype/src/base/ftbase.c
 FT_CSRCS += freetype/src/base/ftbbox.c
 FT_CSRCS += freetype/src/base/ftbitmap.c
 FT_CSRCS += freetype/src/base/ftdebug.c
 FT_CSRCS += freetype/src/base/ftglyph.c
 FT_CSRCS += freetype/src/base/ftinit.c
 FT_CSRCS += freetype/src/base/ftmm.c
 FT_CSRCS += freetype/src/bdf/bdf.c
 FT_CSRCS += freetype/src/cache/ftcache.c
 FT_CSRCS += freetype/src/cff/cff.c
 FT_CSRCS += freetype/src/cid/type1cid.c
 FT_CSRCS += freetype/src/gzip/ftgzip.c
 FT_CSRCS += freetype/src/lzw/ftlzw.c
 FT_CSRCS += freetype/src/sfnt/sfnt.c
 FT_CSRCS += freetype/src/pcf/pcf.c
 FT_CSRCS += freetype/src/pfr/pfr.c
 FT_CSRCS += freetype/src/psaux/psaux.c
 FT_CSRCS += freetype/src/pshinter/pshrec.c
 FT_CSRCS += freetype/src/pshinter/pshalgo.c
 FT_CSRCS += freetype/src/pshinter/pshglob.c
 FT_CSRCS += freetype/src/pshinter/pshmod.c
 FT_CSRCS += freetype/src/psnames/psmodule.c
 FT_CSRCS += freetype/src/raster/raster.c
 FT_CSRCS += freetype/src/sdf/sdf.c
 FT_CSRCS += freetype/src/svg/svg.c
 FT_CSRCS += freetype/src/type1/type1.c
 FT_CSRCS += freetype/src/type42/type42.c
 FT_CSRCS += freetype/src/winfonts/winfnt.c
 FT_CSRCS += freetype/src/smooth/smooth.c
 FT_CSRCS += freetype/src/truetype/truetype.c
 CSRCS += $(FT_CSRCS)


#CSRCS += $(wildcard $(LVGL_DIR)/mqtt/kawaii-mqtt/mqttclient/*.c) \
#         $(wildcard $(LVGL_DIR)/mqtt/kawaii-mqtt/network/mbedtls/library/*.c) \
#         $(wildcard $(LVGL_DIR)/mqtt/kawaii-mqtt/network/mbedtls/wrapper/*.c) \
#         $(wildcard $(LVGL_DIR)/mqtt/kawaii-mqtt/network/*.c) \
#         $(wildcard $(LVGL_DIR)/mqtt/kawaii-mqtt/mqtt/*.c) \
#         $(wildcard $(LVGL_DIR)/mqtt/kawaii-mqtt/platform/linux/*.c) \
#         $(wildcard $(LVGL_DIR)/mqtt/kawaii-mqtt/common/log/*.c) \
#         $(wildcard $(LVGL_DIR)/mqtt/kawaii-mqtt/common/log/arch/linux/*.c) \
#         $(wildcard $(LVGL_DIR)/mqtt/kawaii-mqtt/common/*.c)

OBJEXT 			?= .o

AOBJS 			= $(ASRCS:.S=$(OBJEXT))
COBJS 			= $(CSRCS:.c=$(OBJEXT))
CXXOBJS 		= $(CXXSRCS:.cpp=$(OBJEXT))

MAINOBJ 		= $(MAINSRC:.c=$(OBJEXT))

SRCS 			= $(ASRCS) $(CSRCS) $(CXXSRCS) $(MAINSRC)
OBJS 			= $(AOBJS) $(COBJS) $(CXXOBJS) $(MAINOBJ)
TARGET 			= $(addprefix $(BUILD_OBJ_DIR)/, $(patsubst ./%, %, $(OBJS)))

## MAINOBJ -> OBJFILES


all: default

$(BUILD_OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@$(CXX)  $(CXXFLAGS) -c $< -o $@
	@echo "CXX $<"

$(BUILD_OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@$(CC)  $(CFLAGS) -c $< -o $@
	@echo "CC $<"

$(BUILD_OBJ_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	@$(CC)  $(CFLAGS) -c $< -o $@
	@echo "CC $<"


default: $(TARGET)
	@mkdir -p $(dir $(BUILD_BIN_DIR)/)
	$(CC) -o $(BUILD_BIN_DIR)/$(BIN) $(TARGET) $(LDFLAGS)

clean: 
	rm -rf $(BUILD_DIR)

install:
	install -d $(DESTDIR)$(bindir)
	install $(BUILD_BIN_DIR)/$(BIN) $(DESTDIR)$(bindir)

uninstall:
	$(RM) -r $(addprefix $(DESTDIR)$(bindir)/,$(BIN))
