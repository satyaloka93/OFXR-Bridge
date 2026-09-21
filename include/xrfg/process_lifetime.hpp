#pragma once

namespace xrfg {
// Live graphics/session objects must not be destroyed from CRT/DLL teardown:
// downstream layers may already be detached, and GPU waits run under loader lock.
// Owners still erase entries during explicit xrDestroySession/xrDestroyInstance.
// At process termination the OS reclaims any abandoned sessions and resources.
template<class T>
T& process_lifetime() {
    static T* const value = new T;
    return *value;
}
} // namespace xrfg
