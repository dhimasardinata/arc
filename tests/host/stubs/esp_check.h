#pragma once

#include <cstdlib>

#include "esp_err.h"

#define ESP_ERROR_CHECK(expr)   \
    do {                        \
        if ((expr) != ESP_OK) { \
            std::abort();       \
        }                       \
    } while (false)
