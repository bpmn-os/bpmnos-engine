#ifndef BPMNOS_Execution_expired_H
#define BPMNOS_Execution_expired_H

#include <tuple>
#include <memory>

namespace BPMNOS::Execution {

template<typename T>
struct is_weak_ptr : std::false_type {};

template<typename T>
struct is_weak_ptr<std::weak_ptr<T>> : std::true_type {};

/**
 * @brief Determines whether any of the initial weak_ptr elements in a tuple is expired.
 */
template <std::size_t I = 0, typename... U>
bool expired(const std::tuple<U...> &t) {
    if constexpr (I < sizeof...(U)) {
        if constexpr (is_weak_ptr<typename std::tuple_element<I, std::tuple<U...>>::type>::value) {
            // element is a weak_ptr
            auto wp = std::get<I>(t);
            if ( wp.expired() ) {
                // weak_ptr is expired 
                return true;
            }
            // recursively iterate over the rest of the tuple
            return expired<I + 1>(t);
        } else {
            // first non-weak_ptr element found and none of the previous weak_ptr elements is expired
            return false;
        }
    }
    // no element is expired
    return false;
}

} // namespace BPMNOS::Execution
#endif // BPMNOS_Execution_expired_H
