#include "picopass_dev.h"

#include <furi/furi.h>

struct PicopassDev {
    PicopassData* data;
    PicopassDevLoadingCallback callback;
    void* context;
};

static const char* picopass_file_header = "Flipper Picopass device";
static const uint32_t picopass_file_version = 1;

PicopassDev* picopass_dev_alloc() {
    PicopassDev* instance = malloc(sizeof(PicopassDev));
    instance->data = picopass_protocol_alloc();

    return instance;
}

void picopass_dev_free(PicopassDev* instance) {
    furi_assert(instance);

    picopass_protocol_free(instance->data);
}

void picopass_dev_reset(PicopassDev* instance) {
    furi_assert(instance);

    picopass_protocol_reset(instance->data);
}

void picopass_dev_set_loading_callback(
    PicopassDev* instance,
    PicopassDevLoadingCallback callback,
    void* context) {
    furi_assert(instance);
    furi_assert(callback);

    instance->callback = callback;
    instance->context = context;
}

void picopass_dev_set_data(PicopassDev* instance, const PicopassData* data) {
    furi_assert(instance);

    picopass_protocol_copy(instance->data, data);
}

const PicopassData* picopass_dev_get_data(PicopassDev* instance) {
    furi_assert(instance);

    return instance->data;
}

bool picopass_dev_save(PicopassDev* instance, const char* path) {
    furi_assert(instance);
    furi_assert(path);

    bool success = false;
    Storage* storage = furi_record_open(RECORD_STORAGE);
    FlipperFormat* ff = flipper_format_buffered_file_alloc(storage);

    if(instance->callback) {
        instance->callback(instance->context, true);
    }

    do {
        // Open file
        if(!flipper_format_buffered_file_open_always(ff, path)) break;

        // Write header
        if(!flipper_format_write_header_cstr(ff, picopass_file_header, picopass_file_version))
            break;

        // Save data
        if(!picopass_protocol_save(instance->data, ff)) break;

        success = true;
    } while(false);

    if(instance->callback) {
        instance->callback(instance->context, false);
    }

    flipper_format_free(ff);
    furi_record_close(RECORD_STORAGE);

    return success;
}

bool picopass_dev_load(PicopassDev* instance, const char* path) {
    furi_assert(instance);
    furi_assert(path);

    bool success = false;
    Storage* storage = furi_record_open(RECORD_STORAGE);
    FlipperFormat* ff = flipper_format_buffered_file_alloc(storage);
    FuriString* temp_str = furi_string_alloc();

    if(instance->callback) {
        instance->callback(instance->context, true);
    }

    do {
        // Open existing file
        if(!flipper_format_buffered_file_open_existing(ff, path)) break;

        // Read and verify file header
        uint32_t version = 0;
        if(!flipper_format_read_header(ff, temp_str, &version)) break;

        if(furi_string_cmp_str(temp_str, picopass_file_header)) break;
        if(version < picopass_file_version) break;

        // Load data
        if(!picopass_protocol_load(instance->data, ff, version)) break;
        success = true;
    } while(false);

    if(instance->callback) {
        instance->callback(instance->context, false);
    }

    furi_string_free(temp_str);
    flipper_format_free(ff);
    furi_record_close(RECORD_STORAGE);

    return success;
}
