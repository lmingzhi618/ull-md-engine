#include "ull/perf/thread_affinity.h"

#ifdef __APPLE__

#include <mach/mach.h>
#include <mach/thread_policy.h>
#include <pthread.h>

namespace ull::perf {

void pin_thread_to_core(std::size_t core_id) {
  thread_affinity_policy_data_t policy = {static_cast<integer_t>(core_id)};

  thread_policy_set(mach_thread_self(), THREAD_AFFINITY_POLICY,
                    (thread_policy_t)&policy, THREAD_AFFINITY_POLICY_COUNT);

} // namespace ull::perf

void pin_thread(std::thread &t, std::size_t core_id) {
  pthread_t native = t.native_handle();

  thread_affinity_policy_data_t policy = {static_cast<integer_t>(core_id)};

  thread_policy_set(pthread_mach_thread_np(native), THREAD_AFFINITY_POLICY,
                    (thread_policy_t)&policy, THREAD_AFFINITY_POLICY_COUNT);
}
} // namespace ull::perf
#endif
