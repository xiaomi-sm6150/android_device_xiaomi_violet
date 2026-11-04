/*
 * SPDX-FileCopyrightText: 2026 The PixelOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "vibrator-violet"

#include "VibratorWrapper.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <thread>

#include <unistd.h>

#include <android-base/logging.h>
#include <android-base/properties.h>

namespace aidl {
namespace android {
namespace hardware {
namespace vibrator {

namespace {

constexpr char kVmaxPath[] = "/sys/class/leds/vibrator/vmax_mv";

// PM6150 vibrator LDO limits, see leds-qpnp-vibrator-ldo.c.
constexpr int kLdoMinMv = 1504;
constexpr int kLdoMaxMv = 3544;
// violet's DT voltage. vmax_mv itself cannot be used as the maximum, since
// it keeps whatever this HAL last wrote across HAL restarts.
constexpr int kDefaultMaxMv = 2800;
// The driver never plays anything shorter than this.
constexpr int32_t kMinPlayMs = 50;

// How long on() waits for the setAmplitude() that follows it.
constexpr auto kAmplitudeWait = std::chrono::milliseconds(5);

struct EffectSpec {
    Effect effect;
    int32_t durationMs;
    float level[3];  // LIGHT, MEDIUM, STRONG, as a fraction of the amplitude range
};

constexpr EffectSpec kEffects[] = {
        {Effect::TEXTURE_TICK, kMinPlayMs, {0.05f, 0.10f, 0.15f}},
        {Effect::TICK, kMinPlayMs, {0.10f, 0.15f, 0.25f}},
        {Effect::CLICK, kMinPlayMs, {0.20f, 0.30f, 0.40f}},
        {Effect::HEAVY_CLICK, 70, {0.50f, 0.70f, 0.90f}},
};

const EffectSpec* findEffect(Effect effect) {
    for (const auto& spec : kEffects) {
        if (spec.effect == effect) return &spec;
    }
    return nullptr;
}

int readVmax() {
    std::ifstream file(kVmaxPath);
    int mv = -1;
    file >> mv;
    return file.fail() ? -1 : mv;
}

}  // namespace

VibratorWrapper::VibratorWrapper() : mImpl(ndk::SharedRefBase::make<Vibrator>()) {
    int currentMv = readVmax();
    if (currentMv <= 0 || access(kVmaxPath, W_OK) != 0) {
        LOG(WARNING) << "No writable " << kVmaxPath << ", passing calls through unchanged";
        return;
    }

    mMaxMv = std::clamp(::android::base::GetIntProperty("ro.vendor.vibrator.max_mv", kDefaultMaxMv),
                        kLdoMinMv, kLdoMaxMv);
    mMinMv = std::clamp(::android::base::GetIntProperty("ro.vendor.vibrator.min_mv", kLdoMinMv),
                        kLdoMinMv, mMaxMv);
    mAmplitudeMv = mMaxMv;
    mCurrentMv = currentMv;
    mHasVmax = true;

    LOG(INFO) << "Amplitude control via vmax_mv: " << mMinMv << "-" << mMaxMv << " mV";
}

int VibratorWrapper::amplitudeToMv(float amplitude) const {
    return mMinMv + static_cast<int>(std::lround(amplitude * (mMaxMv - mMinMv)));
}

void VibratorWrapper::writeVmaxLocked(int mv) {
    if (mv == mCurrentMv) return;

    std::ofstream file(kVmaxPath);
    file << mv;
    file.flush();
    if (file.fail()) {
        PLOG(ERROR) << "Failed to write " << mv << " to " << kVmaxPath;
        mCurrentMv = -1;
        return;
    }
    mCurrentMv = mv;
}

void VibratorWrapper::startPendingLocked() {
    mPending = false;
    auto status = mImpl->on(mPendingMs, mPendingCallback);
    mPendingCallback.reset();
    if (!status.isOk()) {
        LOG(ERROR) << "on(" << mPendingMs << ") failed: " << status.getDescription();
    }
}

ndk::ScopedAStatus VibratorWrapper::getCapabilities(int32_t* _aidl_return) {
    auto status = mImpl->getCapabilities(_aidl_return);
    if (status.isOk() && mHasVmax) {
        *_aidl_return |= IVibrator::CAP_AMPLITUDE_CONTROL | IVibrator::CAP_PERFORM_CALLBACK;
    }
    return status;
}

ndk::ScopedAStatus VibratorWrapper::off() {
    {
        std::lock_guard<std::mutex> lock(mLock);
        mPending = false;
        mPendingCallback.reset();
    }
    mCv.notify_all();
    return mImpl->off();
}

ndk::ScopedAStatus VibratorWrapper::on(int32_t timeoutMs,
                                       const std::shared_ptr<IVibratorCallback>& callback) {
    if (!mHasVmax) return mImpl->on(timeoutMs, callback);

    std::lock_guard<std::mutex> lock(mLock);
    writeVmaxLocked(mAmplitudeMv);
    mPending = true;
    mPendingMs = timeoutMs;
    mPendingCallback = callback;
    uint64_t seq = ++mPendingSeq;

    // Start anyway if no setAmplitude() arrives in time.
    std::thread([this, seq] {
        std::unique_lock<std::mutex> lock(mLock);
        if (!mCv.wait_for(lock, kAmplitudeWait,
                          [this, seq] { return !mPending || mPendingSeq != seq; })) {
            startPendingLocked();
        }
    }).detach();

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus VibratorWrapper::setAmplitude(float amplitude) {
    if (!mHasVmax) return mImpl->setAmplitude(amplitude);

    if (!(amplitude > 0.0f && amplitude <= 1.0f)) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    {
        std::lock_guard<std::mutex> lock(mLock);
        mAmplitudeMv = amplitudeToMv(amplitude);
        // The LDO only picks up a new voltage when a pulse starts, so a change
        // while already vibrating applies from the next pulse on.
        writeVmaxLocked(mAmplitudeMv);
        if (mPending) startPendingLocked();
    }
    mCv.notify_all();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus VibratorWrapper::perform(Effect effect, EffectStrength strength,
                                            const std::shared_ptr<IVibratorCallback>& callback,
                                            int32_t* _aidl_return) {
    const EffectSpec* spec = mHasVmax ? findEffect(effect) : nullptr;
    if (spec == nullptr) return mImpl->perform(effect, strength, callback, _aidl_return);

    int index;
    switch (strength) {
        case EffectStrength::LIGHT:
            index = 0;
            break;
        case EffectStrength::MEDIUM:
            index = 1;
            break;
        case EffectStrength::STRONG:
            index = 2;
            break;
        default:
            return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }

    ndk::ScopedAStatus status = ndk::ScopedAStatus::ok();
    {
        std::lock_guard<std::mutex> lock(mLock);
        mPending = false;
        mPendingCallback.reset();
        writeVmaxLocked(amplitudeToMv(spec->level[index]));
        status = mImpl->on(spec->durationMs, callback);
    }
    mCv.notify_all();

    if (!status.isOk()) return status;
    *_aidl_return = spec->durationMs;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus VibratorWrapper::getSupportedEffects(std::vector<Effect>* _aidl_return) {
    auto status = mImpl->getSupportedEffects(_aidl_return);
    if (!status.isOk() || !mHasVmax) return status;

    for (const auto& spec : kEffects) {
        if (std::find(_aidl_return->begin(), _aidl_return->end(), spec.effect) ==
            _aidl_return->end()) {
            _aidl_return->push_back(spec.effect);
        }
    }
    return status;
}

// Everything below is passed straight through to the QTI HAL.

ndk::ScopedAStatus VibratorWrapper::setExternalControl(bool enabled) {
    return mImpl->setExternalControl(enabled);
}

ndk::ScopedAStatus VibratorWrapper::getCompositionDelayMax(int32_t* maxDelayMs) {
    return mImpl->getCompositionDelayMax(maxDelayMs);
}

ndk::ScopedAStatus VibratorWrapper::getCompositionSizeMax(int32_t* maxSize) {
    return mImpl->getCompositionSizeMax(maxSize);
}

ndk::ScopedAStatus VibratorWrapper::getSupportedPrimitives(
        std::vector<CompositePrimitive>* supported) {
    return mImpl->getSupportedPrimitives(supported);
}

ndk::ScopedAStatus VibratorWrapper::getPrimitiveDuration(CompositePrimitive primitive,
                                                         int32_t* durationMs) {
    return mImpl->getPrimitiveDuration(primitive, durationMs);
}

ndk::ScopedAStatus VibratorWrapper::compose(const std::vector<CompositeEffect>& composite,
                                            const std::shared_ptr<IVibratorCallback>& callback) {
    return mImpl->compose(composite, callback);
}

ndk::ScopedAStatus VibratorWrapper::getSupportedAlwaysOnEffects(std::vector<Effect>* _aidl_return) {
    return mImpl->getSupportedAlwaysOnEffects(_aidl_return);
}

ndk::ScopedAStatus VibratorWrapper::alwaysOnEnable(int32_t id, Effect effect,
                                                   EffectStrength strength) {
    return mImpl->alwaysOnEnable(id, effect, strength);
}

ndk::ScopedAStatus VibratorWrapper::alwaysOnDisable(int32_t id) {
    return mImpl->alwaysOnDisable(id);
}

ndk::ScopedAStatus VibratorWrapper::getResonantFrequency(float* resonantFreqHz) {
    return mImpl->getResonantFrequency(resonantFreqHz);
}

ndk::ScopedAStatus VibratorWrapper::getQFactor(float* qFactor) {
    return mImpl->getQFactor(qFactor);
}

ndk::ScopedAStatus VibratorWrapper::getFrequencyResolution(float* freqResolutionHz) {
    return mImpl->getFrequencyResolution(freqResolutionHz);
}

ndk::ScopedAStatus VibratorWrapper::getFrequencyMinimum(float* freqMinimumHz) {
    return mImpl->getFrequencyMinimum(freqMinimumHz);
}

ndk::ScopedAStatus VibratorWrapper::getBandwidthAmplitudeMap(std::vector<float>* _aidl_return) {
    return mImpl->getBandwidthAmplitudeMap(_aidl_return);
}

ndk::ScopedAStatus VibratorWrapper::getPwlePrimitiveDurationMax(int32_t* durationMs) {
    return mImpl->getPwlePrimitiveDurationMax(durationMs);
}

ndk::ScopedAStatus VibratorWrapper::getPwleCompositionSizeMax(int32_t* maxSize) {
    return mImpl->getPwleCompositionSizeMax(maxSize);
}

ndk::ScopedAStatus VibratorWrapper::getSupportedBraking(std::vector<Braking>* supported) {
    return mImpl->getSupportedBraking(supported);
}

ndk::ScopedAStatus VibratorWrapper::composePwle(
        const std::vector<PrimitivePwle>& composite,
        const std::shared_ptr<IVibratorCallback>& callback) {
    return mImpl->composePwle(composite, callback);
}

}  // namespace vibrator
}  // namespace hardware
}  // namespace android
}  // namespace aidl
