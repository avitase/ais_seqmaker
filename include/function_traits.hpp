#pragma once

#include <functional>

namespace function_traits {
template <typename T> struct function_traits;

template <typename R, typename... Args> struct function_traits<R(Args...)> {
    using result_type = R;
    static constexpr auto n_args = sizeof...(Args);

    template <std::size_t i> using arg_type = std::tuple_element_t<i, std::tuple<Args...>>;
};

template <typename R, typename... Args>
struct function_traits<std::function<R(Args...)>>: function_traits<R(Args...)> {};

template <typename F>
using lambda_traits = function_traits<decltype(std::function{std::declval<F>()})>;
}   // namespace function_traits
