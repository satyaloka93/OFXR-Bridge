#include "xrfg/process_lifetime.hpp"
#include <cstdlib>
#include <memory>
#include <unordered_map>

namespace {
bool downstream_detached = false;
unsigned destroyed = 0;
struct Session {
    ~Session() {
        // A downstream xrDestroySwapchain call after detach would crash.
        if (downstream_detached) std::_Exit(71);
        ++destroyed;
    }
};
using Sessions = std::unordered_map<unsigned, std::unique_ptr<Session>>;
void detach_downstream() { downstream_detached = true; }
}
int main() {
    // An ordinary function-static registry would be destroyed AFTER the
    // downstream detach handler registered below.
    auto& sessions = xrfg::process_lifetime<Sessions>();
    std::atexit(detach_downstream);
    sessions.emplace(1, std::make_unique<Session>());
    sessions.erase(1);
    if (destroyed != 1 || !sessions.empty()) return 1;
    sessions.emplace(2, std::make_unique<Session>());
    // Simulate an application exiting without xrDestroySession. The registry
    // must retain ownership through CRT teardown; the OS reclaims it on exit.
    return 0;
}
