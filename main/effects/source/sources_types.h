#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum effects_source_t {
    EFFECTS_SOURCE_SIMULATION,
    EFFECTS_SOURCE_MICROPHONE,
    EFFECTS_SOURCE_WIRED,
} effects_source_t;


#ifdef __cplusplus
}
#endif