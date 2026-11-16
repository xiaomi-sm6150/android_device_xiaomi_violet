/*
 * SPDX-FileCopyrightText: 2026 The PixelOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "acdb_voice_helper"

#include <dlfcn.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <string>

#include <log/log.h>
#include <sys/system_properties.h>
#include <tinyalsa/asoundlib.h>

namespace {

// Request: "<seq>:<rx acdb id>:<tx acdb id>", answered with kDone = "<seq>".
constexpr char kRequest[] = "vendor.audio.voice.cal";
constexpr char kDone[] = "vendor.audio.voice.cal_done";
constexpr int kAlsaCard = 0;
constexpr int kAcdbLoaderInitV4 = 4;

struct listnode {
    listnode* next;
    listnode* prev;
};

// hardware/qcom-caf/<soc>/audio/hal/acdb.h
struct acdb_init_data_v4 {
    char* cvd_version;
    char* snd_card_name;
    listnode* meta_key_list;
    bool* is_instance_id_supported;
};

using acdb_init_v4_t = int (*)(void*, int);
using acdb_send_voice_cal_t = void (*)(int, int);
using acdb_send_common_top_t = int (*)(void);

void* gHandle = nullptr;
acdb_init_v4_t gInitV4 = nullptr;
acdb_send_voice_cal_t gSendVoiceCal = nullptr;
bool gInitialized = false;

bool acdbInit() {
    if (gInitialized) return true;
    if (gHandle == nullptr) {
        gHandle = dlopen("libacdbloader.so", RTLD_NOW);
        if (gHandle == nullptr) {
            ALOGE("dlopen libacdbloader.so: %s", dlerror());
            return false;
        }
        gInitV4 = reinterpret_cast<acdb_init_v4_t>(dlsym(gHandle, "acdb_loader_init_v4"));
        gSendVoiceCal =
                reinterpret_cast<acdb_send_voice_cal_t>(dlsym(gHandle, "acdb_loader_send_voice_cal"));
        if (gInitV4 == nullptr || gSendVoiceCal == nullptr) {
            ALOGE("libacdbloader.so lacks init_v4/send_voice_cal");
            return false;
        }
    }
    struct mixer* mixer = mixer_open(kAlsaCard);
    if (mixer == nullptr) {
        ALOGW("sound card %d not available yet", kAlsaCard);
        return false;
    }
    static char cvdVersion[100];
    static char cardName[64];
    memset(cvdVersion, 0, sizeof(cvdVersion));
    if (struct mixer_ctl* ctl = mixer_get_ctl_by_name(mixer, "CVD Version"); ctl != nullptr) {
        unsigned int count = mixer_ctl_get_num_values(ctl);
        if (count > sizeof(cvdVersion) - 1) count = sizeof(cvdVersion) - 1;
        mixer_ctl_get_array(ctl, cvdVersion, count);
    }
    strlcpy(cardName, mixer_get_name(mixer), sizeof(cardName));
    mixer_close(mixer);

    static listnode metaKeys = {&metaKeys, &metaKeys};
    static bool instanceIdSupported = false;
    acdb_init_data_v4 data = {cvdVersion, cardName, &metaKeys, &instanceIdSupported};
    const int result = gInitV4(&data, kAcdbLoaderInitV4);
    ALOGI("acdb_loader_init_v4(card '%s', cvd '%s'): %d", cardName, cvdVersion, result);
    if (result != 0) return false;
    if (auto sendTop = reinterpret_cast<acdb_send_common_top_t>(
                dlsym(gHandle, "acdb_loader_send_common_custom_topology"));
        sendTop != nullptr) {
        sendTop();
    }
    gInitialized = true;
    return true;
}

void handleRequest(const char* value) {
    unsigned int seq = 0;
    int rx = 0, tx = 0;
    if (sscanf(value, "%u:%d:%d", &seq, &rx, &tx) != 3) {
        ALOGE("bad request '%s'", value);
        return;
    }
    if (acdbInit()) {
        gSendVoiceCal(rx, tx);
        ALOGI("sent voice cal rx %d tx %d (seq %u)", rx, tx, seq);
    } else {
        ALOGE("no ACDB, voice cal rx %d tx %d not sent (seq %u)", rx, tx, seq);
    }
    __system_property_set(kDone, std::to_string(seq).c_str());
}

}  // namespace

int main() {
    // Initialize early if the card is already there; requests retry otherwise.
    for (int i = 0; i < 60 && !acdbInit(); ++i) sleep(1);

    std::string last;
    uint32_t globalSerial = 0;
    for (;;) {
        const prop_info* pi = __system_property_find(kRequest);
        if (pi == nullptr) {
            __system_property_wait(nullptr, globalSerial, &globalSerial, nullptr);
            continue;
        }
        uint32_t serial = __system_property_serial(pi);
        char value[PROP_VALUE_MAX] = {};
        __system_property_get(kRequest, value);
        if (last != value) {
            last = value;
            handleRequest(value);
        }
        __system_property_wait(pi, serial, &serial, nullptr);
    }
}
