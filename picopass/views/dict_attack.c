#include "dict_attack.h"

#include <gui/elements.h>

typedef enum {
    DictAttackStateRead,
    DictAttackStateCardRemoved,
} DictAttackState;

struct DictAttack {
    View* view;
    DictAttackCallback callback;
    void* context;
};

typedef struct {
    DictAttackState state;
    FuriString* header;
    uint16_t dict_keys_total;
    uint16_t dict_keys_current;
} DictAttackViewModel;

static void dict_attack_draw_callback(Canvas* canvas, void* model) {
    DictAttackViewModel* m = model;
    if(m->state == DictAttackStateCardRemoved) {
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str_aligned(canvas, 64, 4, AlignCenter, AlignTop, "Lost the tag!");
        canvas_set_font(canvas, FontSecondary);
        elements_multiline_text_aligned(
            canvas, 64, 23, AlignCenter, AlignTop, "Make sure the tag is\npositioned correctly.");
    } else if(m->state == DictAttackStateRead) {
        char draw_str[32] = {};
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str_aligned(
            canvas, 64, 0, AlignCenter, AlignTop, furi_string_get_cstr(m->header));
        float dict_progress = m->dict_keys_total == 0 ?
                                  0 :
                                  (float)(m->dict_keys_current) / (float)(m->dict_keys_total);
        if(m->dict_keys_current == 0) {
            // Cause when people see 0 they think it's broken
            snprintf(draw_str, sizeof(draw_str), "%d/%d", 1, m->dict_keys_total);
        } else {
            snprintf(
                draw_str, sizeof(draw_str), "%d/%d", m->dict_keys_current, m->dict_keys_total);
        }
        elements_progress_bar_with_text(canvas, 0, 20, 128, dict_progress, draw_str);
        canvas_set_font(canvas, FontSecondary);
    }
    elements_button_center(canvas, "Skip");
}

static bool dict_attack_input_callback(InputEvent* event, void* context) {
    DictAttack* dict_attack = context;
    bool consumed = false;
    if(event->type == InputTypeShort && event->key == InputKeyOk) {
        if(dict_attack->callback) {
            dict_attack->callback(dict_attack->context);
        }
        consumed = true;
    }
    return consumed;
}

DictAttack* dict_attack_alloc() {
    DictAttack* dict_attack = malloc(sizeof(DictAttack));
    dict_attack->view = view_alloc();
    view_allocate_model(dict_attack->view, ViewModelTypeLocking, sizeof(DictAttackViewModel));
    view_set_draw_callback(dict_attack->view, dict_attack_draw_callback);
    view_set_input_callback(dict_attack->view, dict_attack_input_callback);
    view_set_context(dict_attack->view, dict_attack);
    with_view_model(
        dict_attack->view,
        DictAttackViewModel * model,
        { model->header = furi_string_alloc(); },
        false);
    return dict_attack;
}

void dict_attack_free(DictAttack* dict_attack) {
    furi_assert(dict_attack);
    with_view_model(
        dict_attack->view,
        DictAttackViewModel * model,
        { furi_string_free(model->header); },
        false);
    view_free(dict_attack->view);
    free(dict_attack);
}

void dict_attack_reset(DictAttack* dict_attack) {
    furi_assert(dict_attack);
    with_view_model(
        dict_attack->view,
        DictAttackViewModel * model,
        {
            model->state = DictAttackStateRead;
            model->dict_keys_total = 0;
            model->dict_keys_current = 0;
            furi_string_reset(model->header);
        },
        false);
}

View* dict_attack_get_view(DictAttack* dict_attack) {
    furi_assert(dict_attack);
    return dict_attack->view;
}

void dict_attack_set_callback(DictAttack* dict_attack, DictAttackCallback callback, void* context) {
    furi_assert(dict_attack);
    furi_assert(callback);
    dict_attack->callback = callback;
    dict_attack->context = context;
}

void dict_attack_set_header(DictAttack* dict_attack, const char* header) {
    furi_assert(dict_attack);
    furi_assert(header);

    with_view_model(
        dict_attack->view,
        DictAttackViewModel * model,
        { furi_string_set(model->header, header); },
        true);
}

void dict_attack_set_card_detected(DictAttack* dict_attack) {
    furi_assert(dict_attack);
    with_view_model(
        dict_attack->view,
        DictAttackViewModel * model,
        { model->state = DictAttackStateRead; },
        true);
}

void dict_attack_set_card_removed(DictAttack* dict_attack) {
    furi_assert(dict_attack);
    with_view_model(
        dict_attack->view,
        DictAttackViewModel * model,
        { model->state = DictAttackStateCardRemoved; },
        true);
}

void dict_attack_set_total_dict_keys(DictAttack* dict_attack, uint16_t dict_keys_total) {
    furi_assert(dict_attack);
    with_view_model(
        dict_attack->view,
        DictAttackViewModel * model,
        { model->dict_keys_total = dict_keys_total; },
        true);
}

void dict_attack_set_current_dict_key(DictAttack* dict_attack, uint16_t current_key) {
    furi_assert(dict_attack);
    with_view_model(
        dict_attack->view,
        DictAttackViewModel * model,
        { model->dict_keys_current = current_key; },
        true);
}
