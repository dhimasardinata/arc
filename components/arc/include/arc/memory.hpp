#pragma once

// Profile umbrella: memory placement, DMA descriptors, and cache coherency.

#include "arc/axi_graph.hpp"
#include "arc/cache.hpp"
#include "arc/cache_lock.hpp"
#include "arc/caps.hpp"
#include "arc/dma_chain.hpp"
#include "arc/fram.hpp"
#include "arc/mmu_span.hpp"
#include "arc/pipeline.hpp"
#include "arc/place.hpp"
#include "arc/pmr.hpp"
#include "arc/prefetch.hpp"
#include "arc/scrub.hpp"

#if __has_include("esp_async_memcpy.h")
#include "arc/copy.hpp"
#endif
