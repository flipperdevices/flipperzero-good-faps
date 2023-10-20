#pragma once

#include "protocol/picopass_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PicopassDev PicopassDev;

typedef void (*PicopassDevLoadingCallback)(void* context, bool state);

PicopassDev* picopass_dev_alloc();

void picopass_dev_free(PicopassDev* instance);

void picopass_dev_reset(PicopassDev* instance);

void picopass_dev_set_loading_callback(
    PicopassDev* instance,
    PicopassDevLoadingCallback callback,
    void* context);

void picopass_dev_set_data(PicopassDev* instance, const PicopassData* data);

const PicopassData* picopass_dev_get_data(PicopassDev* instance);

bool picopass_dev_save(PicopassDev* instance, const char* path);

bool picopass_dev_save_as_lfrfid(PicopassDev* instance, const char* path);

bool picopass_dev_load(PicopassDev* instance, const char* path);

#ifdef __cplusplus
}
#endif
