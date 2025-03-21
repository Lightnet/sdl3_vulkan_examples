// vmausage.cpp
#include <vk_mem_alloc.h>  // Use VMA API, no implementation
#include <SDL3/SDL_log.h>

// Example usage function
void log_vma_stats(VmaAllocator allocator) {
    VmaTotalStatistics stats;
    vmaCalculateStatistics(allocator, &stats);
    SDL_Log("Total VMA memory used: %llu bytes", stats.total.statistics.allocationBytes);
}