/*
 * SPDX-FileCopyrightText: 2026 The PixelOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <condition_variable>
#include <cstdint>
#include <mutex>

#include <Vibrator.h>

namespace aidl {
namespace android {
namespace hardware {
namespace vibrator {

/*
 * Wraps the QTI vibrator HAL and adds amplitude control plus basic
 * predefined effects for the PM6150 LDO vibrator, which on its own can only
 * be switched fully on or off at a fixed voltage. Strength is emulated by
 * reprogramming the LDO voltage (vmax_mv) before each pulse.
 */
class VibratorWrapper : public BnVibrator {
  public:
    VibratorWrapper();

    ndk::ScopedAStatus getCapabilities(int32_t* _aidl_return) override;
    ndk::ScopedAStatus off() override;
    ndk::ScopedAStatus on(int32_t timeoutMs,
                          const std::shared_ptr<IVibratorCallback>& callback) override;
    ndk::ScopedAStatus perform(Effect effect, EffectStrength strength,
                               const std::shared_ptr<IVibratorCallback>& callback,
                               int32_t* _aidl_return) override;
    ndk::ScopedAStatus getSupportedEffects(std::vector<Effect>* _aidl_return) override;
    ndk::ScopedAStatus setAmplitude(float amplitude) override;
    ndk::ScopedAStatus setExternalControl(bool enabled) override;
    ndk::ScopedAStatus getCompositionDelayMax(int32_t* maxDelayMs) override;
    ndk::ScopedAStatus getCompositionSizeMax(int32_t* maxSize) override;
    ndk::ScopedAStatus getSupportedPrimitives(std::vector<CompositePrimitive>* supported) override;
    ndk::ScopedAStatus getPrimitiveDuration(CompositePrimitive primitive,
                                            int32_t* durationMs) override;
    ndk::ScopedAStatus compose(const std::vector<CompositeEffect>& composite,
                               const std::shared_ptr<IVibratorCallback>& callback) override;
    ndk::ScopedAStatus getSupportedAlwaysOnEffects(std::vector<Effect>* _aidl_return) override;
    ndk::ScopedAStatus alwaysOnEnable(int32_t id, Effect effect, EffectStrength strength) override;
    ndk::ScopedAStatus alwaysOnDisable(int32_t id) override;
    ndk::ScopedAStatus getResonantFrequency(float* resonantFreqHz) override;
    ndk::ScopedAStatus getQFactor(float* qFactor) override;
    ndk::ScopedAStatus getFrequencyResolution(float* freqResolutionHz) override;
    ndk::ScopedAStatus getFrequencyMinimum(float* freqMinimumHz) override;
    ndk::ScopedAStatus getBandwidthAmplitudeMap(std::vector<float>* _aidl_return) override;
    ndk::ScopedAStatus getPwlePrimitiveDurationMax(int32_t* durationMs) override;
    ndk::ScopedAStatus getPwleCompositionSizeMax(int32_t* maxSize) override;
    ndk::ScopedAStatus getSupportedBraking(std::vector<Braking>* supported) override;
    ndk::ScopedAStatus composePwle(const std::vector<PrimitivePwle>& composite,
                                   const std::shared_ptr<IVibratorCallback>& callback) override;

  private:
    int amplitudeToMv(float amplitude) const;
    void writeVmaxLocked(int mv);
    void startPendingLocked();

    std::shared_ptr<Vibrator> mImpl;

    bool mHasVmax = false;
    int mMinMv = 0;
    int mMaxMv = 0;

    std::mutex mLock;
    std::condition_variable mCv;
    int mCurrentMv = -1;    // last value written to vmax_mv
    int mAmplitudeMv = 0;   // voltage for the last setAmplitude()

    // on() is held back briefly so the setAmplitude() that the framework
    // sends right after it can program the voltage before the pulse starts.
    bool mPending = false;
    uint64_t mPendingSeq = 0;
    int32_t mPendingMs = 0;
    std::shared_ptr<IVibratorCallback> mPendingCallback;
};

}  // namespace vibrator
}  // namespace hardware
}  // namespace android
}  // namespace aidl
