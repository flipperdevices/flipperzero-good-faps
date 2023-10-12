#pragma once
#include <stdint.h>
#include <gui/view.h>
#include <gui/modules/widget.h>

#include <nfc/protocols/mf_classic/mf_classic.h>

typedef struct DictAttack DictAttack;

typedef void (*DictAttackCallback)(void* context);

DictAttack* dict_attack_alloc();

void dict_attack_free(DictAttack* dict_attack);

void dict_attack_reset(DictAttack* dict_attack);

View* dict_attack_get_view(DictAttack* dict_attack);

void dict_attack_set_callback(DictAttack* dict_attack, DictAttackCallback callback, void* context);

void dict_attack_set_header(DictAttack* dict_attack, const char* header);

void dict_attack_set_card_detected(DictAttack* dict_attack);

void dict_attack_set_card_removed(DictAttack* dict_attack);

void dict_attack_set_total_dict_keys(DictAttack* dict_attack, uint16_t dict_keys_total);

void dict_attack_set_current_dict_key(DictAttack* dict_attack, uint16_t current_key);
