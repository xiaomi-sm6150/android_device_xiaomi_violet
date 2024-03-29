#
# Copyright (C) 2018-2020 The LineageOS Project
# Copyright (C) 2020 The PixelExperience Project
#
# SPDX-License-Identifier: Apache-2.0
#

BOARD_VENDOR := xiaomi

DEVICE_PATH := device/xiaomi/violet

# Inherit from sm6150-common
-include device/xiaomi/sm6150-common/BoardConfigCommon.mk

# Audio
AUDIO_FEATURE_ENABLED_GEF_SUPPORT := true
AUDIO_FEATURE_ENABLED_INSTANCE_ID := true
BOARD_USES_ALSA_AUDIO := true

# Display
TARGET_SCREEN_DENSITY := 440

# HIDL
DEVICE_MANIFEST_FILE += $(DEVICE_PATH)/configs/hidl/manifest.xml

# Kernel
BOARD_KERNEL_CMDLINE += androidboot.fstab_suffix=emmc

TARGET_KERNEL_CONFIG += vendor/violet.config

# Partitions
BOARD_BOOTIMAGE_PARTITION_SIZE := 0x04000000
BOARD_CACHEIMAGE_PARTITION_SIZE := 268435456
BOARD_DTBOIMG_PARTITION_SIZE := 0x0800000
BOARD_RECOVERYIMAGE_PARTITION_SIZE := 67108864
BOARD_USERDATAIMAGE_PARTITION_SIZE := 57453555712

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

# Inherit from the proprietary version
include vendor/xiaomi/violet/BoardConfigVendor.mk
