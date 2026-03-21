#pragma once

#include <thread>

namespace ull::perf {

// Pin current thread to a specific core (best-effort)
void pin_thread_to_core(std::size_t core_id);

// Pin std::thread
void pin_thread(std::thread &t, std::size_t core_id);
} // namespace ull::perf
