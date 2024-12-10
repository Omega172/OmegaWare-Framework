#pragma once

#include <type_traits>

// https://www.unknowncheats.me/forum/anti-cheat-bypass/268039-x64-return-address-spoofing-source-explanation.html
// https://www.youtube.com/watch?v=bSQau-PaCTE

namespace detail
{
    extern "C" void* _spoofer_stub();

    template <typename Returned_t, typename... Args_t>
    static inline Returned_t shellcode_stub_helper(const void* fn, Args_t... aArgs) {
        return reinterpret_cast<Returned_t(*)(Args_t...)>(fn)(aArgs...);
    }

    template <std::size_t sizeArgs, typename>
    struct argument_remapper
    {
        // At least 5 params
        template<typename Returned_t, typename a1_t, typename a2_t, typename a3_t, typename a4_t, typename... Pack_t>
        static Returned_t do_call(const void* pShell, void* pShellParam, a1_t t1, a2_t t2, a3_t t3, a4_t t4, Pack_t... aPack) {
            return shellcode_stub_helper<Returned_t, a1_t, a2_t, a3_t, a4_t, void*, void*, Pack_t...>(pShell, t1, t2, t3, t4, pShellParam, {}, aPack...);
        }
    };

    template <std::size_t sizeArgs>
    struct argument_remapper<sizeArgs, std::enable_if_t<sizeArgs <= 4>>
    {
        // 4 or less params
        template<typename Returned_t, typename a1_t = void*, typename a2_t = void*, typename a3_t = void*, typename a4_t = void*>
        static Returned_t do_call(const void* pShell, void* pShellParam, a1_t t1 = {}, a2_t t2 = {}, a3_t t3 = {}, a4_t t4 = {}) {
            return shellcode_stub_helper<Returned_t, a1_t, a2_t, a3_t, a4_t, void*, void*>(pShell, t1, t2, t3, t4, pShellParam, {});
        }
    };
}

template <typename Returned_t, typename... Args_t>
static inline Returned_t spoof_call(void* pTrampoline, Returned_t(*fn)(Args_t...), Args_t... aArgs) {
    struct {
        const void* m_pTrampoline;
        void* m_fn;
        void* m_pRBX;
    } stShellParams{ pTrampoline, fn };

    return detail::argument_remapper<sizeof...(Args_t), void>::template do_call<Returned_t, Args_t...>(const_cast<const void*>(&detail::_spoofer_stub, &stShellParams, aArgs...));
};