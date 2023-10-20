#pragma once

#include "protocol/picopass_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PicopassDev PicopassDev;

typedef void (*PicopassDevLoadingCallback)(void* context, bool state);

PicopassDev* picopass_device_alloc();

void picopass_device_free(PicopassDev* instance);

void picopass_device_reset(PicopassDev* instance);

void picopass_device_set_loading_callback(
    PicopassDev* instance,
    PicopassDevLoadingCallback callback,
    void* context);

void picopass_device_set_data(PicopassDev* instance, const PicopassData* data);

const PicopassData* picopass_device_get_data(PicopassDev* instance);

bool picopass_device_save(PicopassDev* instance, const char* path);

bool picopass_device_save_as_lfrfid(PicopassDev* instance, const char* path);

bool picopass_device_load(PicopassDev* instance, const char* path);

#ifdef __cplusplus
}
#endif
