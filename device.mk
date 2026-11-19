#
# Copyright (C) 2018-2020 The LineageOS Project
# Copyright (C) 2020 The PixelExperience Project
#
# SPDX-License-Identifier: Apache-2.0
#

# Inherit from sm6150-common
$(call inherit-product, device/xiaomi/sm6150-common/sm6150.mk)

# Inherit from Dolby
$(call inherit-product-if-exists, vendor/oneplus/dolby/oplusdolby.mk)

# Get non-open-source specific aspects
$(call inherit-product, vendor/xiaomi/violet/violet-vendor.mk)

# API level, the device has been commercially launched on
PRODUCT_SHIPPING_API_LEVEL := 28

# Audio
PRODUCT_PACKAGES += \
    acdb_voice_helper \
    audio.primary.sm6150 \
    liba2dpoffload \
    libhdmiedid \
    libhfp \
    libsndmonitor \
    libspkrprot

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/configs/audio/audio_platform_info_intcodec.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_platform_info.xml \
    $(LOCAL_PATH)/configs/audio/audio_platform_info_intcodec.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_platform_info_intcodec.xml \
    $(LOCAL_PATH)/configs/audio/mixer_paths_idp.xml:$(TARGET_COPY_OUT_VENDOR)/etc/mixer_paths_idp.xml \
    $(LOCAL_PATH)/configs/audio/mixer_paths_violet.xml:$(TARGET_COPY_OUT_VENDOR)/etc/mixer_paths_violet.xml \
    $(LOCAL_PATH)/configs/audio/sound_trigger_mixer_paths.xml:$(TARGET_COPY_OUT_VENDOR)/etc/sound_trigger_mixer_paths.xml

# Dolby
TARGET_USES_DOLBY := true

PRODUCT_PACKAGES += \
    DSPVolumeSynchronizer

# Display
PRODUCT_COPY_FILES += \
    $(call find-copy-subdir-files,qdcm_calib_data_*.xml,$(LOCAL_PATH)/qdcm/,$(TARGET_COPY_OUT_VENDOR)/etc/)

# Init scripts
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/rootdir/etc/init.violet.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/init.violet.rc

# Input
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/configs/idc/uinput-fpc.idc:$(TARGET_COPY_OUT_VENDOR)/usr/idc/uinput-fpc.idc \
    $(LOCAL_PATH)/configs/idc/uinput-goodix.idc:$(TARGET_COPY_OUT_VENDOR)/usr/idc/uinput-goodix.idc

# IR
PRODUCT_PACKAGES += \
    android.hardware.ir-service.example

PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.consumerir.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.consumerir.xml

# Keylayouts
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/configs/keylayout/uinput-fpc.kl:$(TARGET_COPY_OUT_VENDOR)/usr/keylayout/uinput-fpc.kl \
    $(LOCAL_PATH)/configs/keylayout/uinput-goodix.kl:$(TARGET_COPY_OUT_VENDOR)/usr/keylayout/uinput-goodix.kl

# Lights
PRODUCT_PACKAGES += \
    android.hardware.light-service.lineage

# MiuiCamera
$(call inherit-product-if-exists, vendor/miuicamera-violet/device.mk)

PRODUCT_SYSTEM_PROPERTIES += \
    ro.product.mod_device=violet_global

# Overlays
DEVICE_PACKAGE_OVERLAYS += \
    $(LOCAL_PATH)/overlay \
    $(LOCAL_PATH)/overlay-custom

# Wi-Fi: ath10k (6.1) firmware IE file and board data for WCN3990
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/configs/wifi/ath10k/WCN3990/hw1.0/firmware-5.bin:$(TARGET_COPY_OUT_VENDOR)/firmware/ath10k/WCN3990/hw1.0/firmware-5.bin \
    $(LOCAL_PATH)/configs/wifi/ath10k/WCN3990/hw1.0/board-2.bin:$(TARGET_COPY_OUT_VENDOR)/firmware/ath10k/WCN3990/hw1.0/board-2.bin

# Soong namespaces
PRODUCT_SOONG_NAMESPACES += \
    $(LOCAL_PATH)

# Vibrator
PRODUCT_PACKAGES += \
    android.hardware.vibrator-service.violet

# Wifi
PRODUCT_PACKAGES += \
    VioletWifiOverlay
