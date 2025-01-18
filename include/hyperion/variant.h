/// @file variant.h
/// @author Braxton Salyer <braxtonsalyer@gmail.com>
/// @brief Hyperion alternative to standard library `variant`, `hyperion::Variant`,
/// with improved ergonomics, safety, and reduced possibility of valueless by exception state.
/// @version 0.1
/// @date 2025-01-18
///
/// MIT License
/// @copyright Copyright (c) 2025 Braxton Salyer <braxtonsalyer@gmail.com>
///
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the "Software"), to deal
/// in the Software without restriction, including without limitation the rights
/// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
/// copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions:
///
/// The above copyright notice and this permission notice shall be included in all
/// copies or substantial portions of the Software.
///
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
/// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
/// SOFTWARE.

#ifndef HYPERION_VARIANT_H
#define HYPERION_VARIANT_H

#include <hyperion/assert.h>
#include <hyperion/mpl/concepts.h>
#include <hyperion/mpl/list.h>
#include <hyperion/mpl/metapredicates.h>
#include <hyperion/platform/compare.h>
#include <hyperion/platform/def.h>
#include <hyperion/variant/storage.h>

#include <concepts>
#include <exception>
#include <initializer_list>
#include <type_traits>

#if HYPERION_PLATFORM_STD_LIB_HAS_COMPARE
    #include <compare>
#endif

namespace hyperion {

    template<typename... TFuncs>
    struct Overload : TFuncs... {
        using TFuncs::operator()...;
    };
    template<typename... TFuncs>
    Overload(TFuncs...) -> Overload<TFuncs...>;

    namespace detail {
        using mpl::operator""_value;

        [[nodiscard]] constexpr auto
        has_same_return_type_for_arg_packs([[maybe_unused]] mpl::MetaType auto func,
                                           [[maybe_unused]] mpl::MetaList auto arg_packs) noexcept
            -> bool {
            constexpr auto have_same_return_type = []([[maybe_unused]] mpl::MetaList auto list) {
                constexpr auto first_list = decltype(arg_packs){}.at(0_value);
                constexpr auto get_return_type
                    = []<typename... TTypes>([[maybe_unused]] mpl::Type<TTypes>... types) {
                          return mpl::decltype_<
                              std::invoke_result_t<typename decltype(func)::type, TTypes...>>();
                      };
                constexpr auto first_type = first_list.unwrap(get_return_type);

                return first_type.is(list.unwrap(get_return_type));
            };

            return arg_packs.all_of(have_same_return_type);
        }

        namespace test {
            static constexpr auto same_return_test_func = []([[maybe_unused]] auto... args) {
                if constexpr(sizeof...(args) == 1) {
                    return "ada";
                }
                else {
                    return true;
                }
            };

            static constexpr auto same_return_test_func2 = []([[maybe_unused]] auto... args) {
                return true;
            };

            static_assert(not has_same_return_type_for_arg_packs(
                mpl::decltype_(same_return_test_func),
                mpl::List<mpl::List<bool>, mpl::List<int, double>, mpl::List<float, int>>{}));
            static_assert(has_same_return_type_for_arg_packs(
                mpl::decltype_(same_return_test_func2),
                mpl::List<mpl::List<bool>, mpl::List<int, double>, mpl::List<float, int>>{}));
        } // namespace test
    } // namespace detail
} // namespace hyperion

#endif // HYPERION_VARIANT_H
