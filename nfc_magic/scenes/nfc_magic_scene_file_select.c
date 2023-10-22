#include "../nfc_magic_app_i.h"
#include <nfc/protocols/mf_classic/mf_classic.h>

static bool nfc_magic_scene_file_select_is_file_suitable(NfcMagicApp* instance) {
    NfcProtocol protocol = nfc_device_get_protocol(instance->source_dev);
    size_t uid_len = 0;
    nfc_device_get_uid(instance->source_dev, &uid_len);

    bool suitable = false;
    if((uid_len == 4) && (protocol == NfcProtocolMfClassic)) {
        const MfClassicData* mfc_data =
            nfc_device_get_data(instance->source_dev, NfcProtocolMfClassic);
        if(mfc_data->type == MfClassicType1k) {
            suitable = true;
        }
    }

    return suitable;

    // NfcDevice* nfc_dev = instance->source_dev;
    // if(nfc_dev->format == NfcDeviceSaveFormatMifareClassic) {
    //     switch(instance->dev->type) {
    //     case MagicTypeClassicGen1:
    //     case MagicTypeClassicDirectWrite:
    //     case MagicTypeClassicAPDU:
    //         if((nfc_dev->dev_data.mf_classic_data.type != MfClassicType1k) ||
    //            (nfc_dev->dev_data.nfc_data.uid_len != instance->dev->uid_len)) {
    //             return false;
    //         }
    //         return true;

    //     case MagicTypeGen4:
    //         return true;
    //     default:
    //         return false;
    //     }
    // } else if(
    //     (nfc_dev->format == NfcDeviceSaveFormatMifareUl) &&
    //     (nfc_dev->dev_data.nfc_data.uid_len == 7)) {
    //     switch(instance->dev->type) {
    //     case MagicTypeUltralightGen1:
    //     case MagicTypeUltralightDirectWrite:
    //     case MagicTypeUltralightC_Gen1:
    //     case MagicTypeUltralightC_DirectWrite:
    //     case MagicTypeGen4:
    //         switch(nfc_dev->dev_data.mf_ul_data.type) {
    //         case MfUltralightTypeNTAGI2C1K:
    //         case MfUltralightTypeNTAGI2C2K:
    //         case MfUltralightTypeNTAGI2CPlus1K:
    //         case MfUltralightTypeNTAGI2CPlus2K:
    //             return false;
    //         default:
    //             return true;
    //         }
    //     default:
    //         return false;
    //     }
    // }
}

void nfc_magic_scene_file_select_on_enter(void* context) {
    NfcMagicApp* instance = context;

    if(nfc_magic_load_from_file_select(instance)) {
        if(nfc_magic_scene_file_select_is_file_suitable(instance)) {
            scene_manager_next_scene(instance->scene_manager, NfcMagicSceneWriteConfirm);
        } else {
            scene_manager_next_scene(instance->scene_manager, NfcMagicSceneNotImplemented);
        }
    } else {
        scene_manager_previous_scene(instance->scene_manager);
    }
}

bool nfc_magic_scene_file_select_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void nfc_magic_scene_file_select_on_exit(void* context) {
    UNUSED(context);
}
