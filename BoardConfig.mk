#
# Copyright (C) 2020 Paranoid Android
#
# SPDX-License-Identifier: Apache-2.0
#

BOARD_VENDOR := xiaomi

DEVICE_PATH := device/xiaomi/violet

# Inherit from sm6150-common
-include device/xiaomi/sm6150-common/BoardConfigCommon.mk

# HIDL
DEVICE_MANIFEST_FILE += $(DEVICE_PATH)/configs/hidl/manifest.xml

# Platform
TARGET_BOARD_PLATFORM_GPU := qcom-adreno612

# Inherit from proprietary files
include vendor/xiaomi/violet/BoardConfigVendor.mk

