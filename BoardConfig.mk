#
# Copyright (C) 2018-2020 The LineageOS Project
# Copyright (C) 2020 The PixelExperience Project
#
# SPDX-License-Identifier: Apache-2.0
#

BOARD_VENDOR := xiaomi

DEVICE_PATH := device/xiaomi/violet

# Inherit from sm6150-common
include device/xiaomi/sm6150-common/BoardConfigCommon.mk

# Audio
AUDIO_FEATURE_ENABLED_GEF_SUPPORT := true
AUDIO_FEATURE_ENABLED_INSTANCE_ID := true
BOARD_USES_ALSA_AUDIO := true

# Display
TARGET_SCREEN_DENSITY := 440

# HIDL
DEVICE_MANIFEST_FILE += $(DEVICE_PATH)/configs/hidl/manifest.xml

# MiuiCamera
-include vendor/miuicamera-violet/BoardConfig.mk

# Partitions
BOARD_BOOTIMAGE_PARTITION_SIZE := 0x04000000
BOARD_CACHEIMAGE_PARTITION_SIZE := 268435456
BOARD_DTBOIMG_PARTITION_SIZE := 0x0800000
BOARD_RECOVERYIMAGE_PARTITION_SIZE := 67108864

BOARD_SUPER_PARTITION_CUST_DEVICE_SIZE := 872415232
BOARD_SUPER_PARTITION_SYSTEM_DEVICE_SIZE := 3758096384
BOARD_SUPER_PARTITION_VENDOR_DEVICE_SIZE := 2147483648
BOARD_SUPER_PARTITION_SIZE := 6777995264

BOARD_SUPER_PARTITION_BLOCK_DEVICES := cust system vendor
BOARD_SUPER_PARTITION_METADATA_DEVICE := system

BOARD_QTI_DYNAMIC_PARTITIONS_SIZE := 6773800960 # (BOARD_SUPER_PARTITION_SIZE - 4194304) 4MiB overhead

# Platform
TARGET_BOARD_PLATFORM_GPU := qcom-adreno612

# Properties
TARGET_VENDOR_PROP += $(DEVICE_PATH)/vendor.prop

# Security
VENDOR_SECURITY_PATCH := 2021-08-01

# Sepolicy
BOARD_VENDOR_SEPOLICY_DIRS += $(DEVICE_PATH)/sepolicy/vendor

# Inherit from the proprietary version
include vendor/xiaomi/violet/BoardConfigVendor.mk

# Kernel: 6.1, built from source with the default clang.
# kernel/xiaomi/sm6150-6.1 is the 6.1 tree (combo-wip); its DTB is built
# small enough for ABL (no full __symbols__). The dtbo stays a prebuilt stub.
TARGET_KERNEL_SOURCE := kernel/xiaomi/sm6150-6.1
TARGET_KERNEL_CONFIG := gki_defconfig vendor/violet.config
TARGET_DTB_LIST_WILDCARD := vendor/qcom/violet-sm6150
BOARD_PREBUILT_DTBOIMAGE := device/xiaomi/violet-kernel/dtbo.img
BOARD_KERNEL_SEPARATED_DTBO := false
BOARD_KERNEL_CMDLINE := androidboot.hardware=qcom androidboot.console=ttyMSM0
BOARD_KERNEL_CMDLINE += service_locator.enable=1 lpm_levels.sleep_disabled=1
BOARD_KERNEL_CMDLINE += loop.max_part=7 androidboot.fstab_suffix=emmc
BOARD_KERNEL_CMDLINE += log_buf_len=4M audit=0 cpuidle.off=1
BOARD_KERNEL_CMDLINE += firmware_class.path=/vendor/firmware,/vendor/firmware_mnt/image
BOARD_KERNEL_CMDLINE += androidboot.boot_devices=soc/7c4000.sdhci
