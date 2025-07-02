#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "esp_err.h"
#include "../source/sources_types.h"
#include "../leds/led_matrix.h"
#include "freertos/FreeRTOS.h"

typedef enum effects_cmd_type_t {
    EFFECTS_CMD_SET_GAIN,           // payload: float, passed deep to sources
    EFFECTS_CMD_SET_FREQUENCY,      // payload: float, passed deep to simulation source
    EFFECTS_CMD_SET_SOURCE,         // payload: Micro, Wired, Simulation
    EFFECTS_CMD_SET_EFFECT,         // payload: Fire, Electric, Simple ...
} effects_cmd_type_t;

// Effect idea: fireplace animation with speed/intensity varied by dynamics of input
typedef enum effects_type_t {
    EFFECTS_TYPE_RAW,
    EFFECTS_TYPE_SIMPLE,
    EFFECTS_TYPE_FIRE,
    EFFECTS_TYPE_MULTICOLOR,
    EFFECTS_TYPE_ELECTRIC,
} effects_type_t;

typedef union effects_cmd_data_t {
    float gain;
    float frequency;
    effects_source_t effects_source;
    effects_type_t effects_type;
} effects_cmd_data_t;

typedef struct effects_cmd_t {
    effects_cmd_type_t type;
    effects_cmd_data_t data;
} effects_cmd_t;

esp_err_t effects_init();

esp_err_t effects_send_cmd(effects_cmd_t cmd);

void effects_draw_to_led_matrix(led_matrix_t* led_matrix);



#ifdef __cplusplus
}
#endif