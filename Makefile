io=uart

APP = bootloader
APP_SRCS += src/main.c src/com.c

export GAP_USE_OPENOCD=1

PMSIS_OS = freertos
# Add functionality needed for FreeRTOS
APP_CFLAGS += -DconfigUSE_TIMERS=1 -DINCLUDE_xTimerPendFunctionCall=1

# Add BSP support files
BSP_SUPPORT = fs/host_fs/semihost.c fs/host_fs/host_fs.c
#BSP_SUPPORT = fs/host_fs/host_fs.c fs/host_fs/semihost.c
PMSIS_BSP_SRC=$(BSP_SUPPORT)

# Set target GAP8 version
TARGET_CHIP=GAP8_V2

include $(RULES_DIR)/pmsis_rules.mk
