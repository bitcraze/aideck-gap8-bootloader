io=uart

APP = bootloader
APP_SRCS += src/main.c src/com.c src/cpx.c src/bl.c src/flash.c src/FreeRTOS_util.c

export GAP_USE_OPENOCD=1

PMSIS_OS = freertos
# Add functionality needed for FreeRTOS
APP_CFLAGS += -DconfigUSE_TIMERS=1 -DINCLUDE_xTimerPendFunctionCall=1 -DCONFIG_NO_CLUSTER=1

# Add linkerfile
APP_LINK_SCRIPT=bootloader.ld

# Add BSP support files
BSP_SUPPORT = fs/host_fs/semihost.c fs/host_fs/host_fs.c
#BSP_SUPPORT = fs/host_fs/host_fs.c fs/host_fs/semihost.c
PMSIS_BSP_SRC=$(BSP_SUPPORT)

# In order to fit in L1 space for the bootloader we need to limit the space in L1 the
# FreeRTOS idle task stack takes. The only way to do this is to override the settings
# in the SDK makefile and supply out own file (which is a copy of the 4.7.0, but with
# idle task stack in L2).
APP_EXCLUDE_SRCS=$(GAP_SDK_HOME)/rtos/freeRTOS/demos/gwt/config/gap8/FreeRTOS_util.c

# Set target GAP8 version
TARGET_CHIP=GAP8_V2

include $(RULES_DIR)/pmsis_rules.mk
