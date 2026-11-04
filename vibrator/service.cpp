/*
 * SPDX-FileCopyrightText: 2026 The PixelOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "vibrator-violet"

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include <string>

#include "VibratorWrapper.h"

using aidl::android::hardware::vibrator::VibratorWrapper;

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(0);
    ABinderProcess_startThreadPool();

    std::shared_ptr<VibratorWrapper> vib = ndk::SharedRefBase::make<VibratorWrapper>();

    const std::string instance = std::string() + VibratorWrapper::descriptor + "/default";
    binder_status_t status = AServiceManager_addService(vib->asBinder().get(), instance.c_str());
    CHECK_EQ(status, STATUS_OK);

    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;  // should not reach
}
