/// @file indexed_call.h
/// @author Braxton Salyer <braxtonsalyer@gmail.com>
/// @brief Metaprogramming function used to call a function that requires a compile-time
// index with a runtime value.
/// @version 0.1
/// @date 2025-01-17
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

#ifndef HYPERION_STD_DETAIL_INDEXED_CALL_H
#define HYPERION_STD_DETAIL_INDEXED_CALL_H

#include <hyperion/assert.h>
#include <hyperion/mpl/value.h>
#include <hyperion/platform/def.h>
#include <hyperion/platform/types.h>

namespace hyperion::detail {

    namespace detail {
        using mpl::operator""_value;

        template<typename TFunc>
        static constexpr auto call(std::size_t desired,
                                   mpl::MetaValue auto bound,
                                   mpl::MetaValue auto current,
                                   TFunc&& func)
            requires(bound > 10)
        {
            if(desired < bound / 2_value) {
                return call(desired, bound / 2_value, current, std::forward<TFunc>(func));
            }

            return call(desired,
                        bound - bound / 2_value,
                        current + bound / 2_value,
                        std::forward<TFunc>(func));
        }

        template<typename TFunc>
        static constexpr auto call([[maybe_unused]] std::size_t desired,
                                   mpl::MetaValue auto bound,
                                   mpl::MetaValue auto current,
                                   TFunc&& func)
            requires(bound == 1)
        {
            return std::forward<TFunc>(func)(current);
        }

        template<typename TFunc>
        static constexpr auto call(std::size_t desired,
                                   mpl::MetaValue auto bound,
                                   mpl::MetaValue auto current,
                                   TFunc&& func)
            requires(bound == 2)
        {
            switch(desired) {
                case 0: return std::forward<TFunc>(func)(current + 0_value);
                case 1: return std::forward<TFunc>(func)(current + 1_value);
                default: HYPERION_UNREACHABLE();
            }
        }

        template<typename TFunc>
        static constexpr auto call(std::size_t desired,
                                   mpl::MetaValue auto bound,
                                   mpl::MetaValue auto current,
                                   TFunc&& func)
            requires(bound == 3)
        {
            switch(desired) {
                case 0: return std::forward<TFunc>(func)(current + 0_value);
                case 1: return std::forward<TFunc>(func)(current + 1_value);
                case 2: return std::forward<TFunc>(func)(current + 2_value);
                default: HYPERION_UNREACHABLE();
            }
        }

        template<typename TFunc>
        static constexpr auto call(std::size_t desired,
                                   mpl::MetaValue auto bound,
                                   mpl::MetaValue auto current,
                                   TFunc&& func)
            requires(bound == 4)
        {
            switch(desired) {
                case 0: return std::forward<TFunc>(func)(current + 0_value);
                case 1: return std::forward<TFunc>(func)(current + 1_value);
                case 2: return std::forward<TFunc>(func)(current + 2_value);
                case 3: return std::forward<TFunc>(func)(current + 3_value);
                default: HYPERION_UNREACHABLE();
            }
        }

        template<typename TFunc>
        static constexpr auto call(std::size_t desired,
                                   mpl::MetaValue auto bound,
                                   mpl::MetaValue auto current,
                                   TFunc&& func)
            requires(bound == 5)
        {
            switch(desired) {
                case 0: return std::forward<TFunc>(func)(current + 0_value);
                case 1: return std::forward<TFunc>(func)(current + 1_value);
                case 2: return std::forward<TFunc>(func)(current + 2_value);
                case 3: return std::forward<TFunc>(func)(current + 3_value);
                case 4: return std::forward<TFunc>(func)(current + 4_value);
                default: HYPERION_UNREACHABLE();
            }
        }

        template<typename TFunc>
        static constexpr auto call(std::size_t desired,
                                   mpl::MetaValue auto bound,
                                   mpl::MetaValue auto current,
                                   TFunc&& func)
            requires(bound == 6)
        {
            switch(desired) {
                case 0: return std::forward<TFunc>(func)(current + 0_value);
                case 1: return std::forward<TFunc>(func)(current + 1_value);
                case 2: return std::forward<TFunc>(func)(current + 2_value);
                case 3: return std::forward<TFunc>(func)(current + 3_value);
                case 4: return std::forward<TFunc>(func)(current + 4_value);
                case 5: return std::forward<TFunc>(func)(current + 5_value);
                default: HYPERION_UNREACHABLE();
            }
        }

        template<typename TFunc>
        static constexpr auto call(std::size_t desired,
                                   mpl::MetaValue auto bound,
                                   mpl::MetaValue auto current,
                                   TFunc&& func)
            requires(bound == 7)
        {
            switch(desired) {
                case 0: return std::forward<TFunc>(func)(current + 0_value);
                case 1: return std::forward<TFunc>(func)(current + 1_value);
                case 2: return std::forward<TFunc>(func)(current + 2_value);
                case 3: return std::forward<TFunc>(func)(current + 3_value);
                case 4: return std::forward<TFunc>(func)(current + 4_value);
                case 5: return std::forward<TFunc>(func)(current + 5_value);
                case 6: return std::forward<TFunc>(func)(current + 6_value);
                default: HYPERION_UNREACHABLE();
            }
        }

        template<typename TFunc>
        static constexpr auto call(std::size_t desired,
                                   mpl::MetaValue auto bound,
                                   mpl::MetaValue auto current,
                                   TFunc&& func)
            requires(bound == 8)
        {
            switch(desired) {
                case 0: return std::forward<TFunc>(func)(current + 0_value);
                case 1: return std::forward<TFunc>(func)(current + 1_value);
                case 2: return std::forward<TFunc>(func)(current + 2_value);
                case 3: return std::forward<TFunc>(func)(current + 3_value);
                case 4: return std::forward<TFunc>(func)(current + 4_value);
                case 5: return std::forward<TFunc>(func)(current + 5_value);
                case 6: return std::forward<TFunc>(func)(current + 6_value);
                case 7: return std::forward<TFunc>(func)(current + 7_value);
                default: HYPERION_UNREACHABLE();
            }
        }

        template<typename TFunc>
        static constexpr auto call(std::size_t desired,
                                   mpl::MetaValue auto bound,
                                   mpl::MetaValue auto current,
                                   TFunc&& func)
            requires(bound == 9)
        {
            switch(desired) {
                case 0: return std::forward<TFunc>(func)(current + 0_value);
                case 1: return std::forward<TFunc>(func)(current + 1_value);
                case 2: return std::forward<TFunc>(func)(current + 2_value);
                case 3: return std::forward<TFunc>(func)(current + 3_value);
                case 4: return std::forward<TFunc>(func)(current + 4_value);
                case 5: return std::forward<TFunc>(func)(current + 5_value);
                case 6: return std::forward<TFunc>(func)(current + 6_value);
                case 7: return std::forward<TFunc>(func)(current + 7_value);
                case 8: return std::forward<TFunc>(func)(current + 8_value);
                default: HYPERION_UNREACHABLE();
            }
        }

        template<typename TFunc>
        static constexpr auto call(std::size_t desired,
                                   mpl::MetaValue auto bound,
                                   mpl::MetaValue auto current,
                                   TFunc&& func)
            requires(bound == 10)
        {
            switch(desired) {
                case 0: return std::forward<TFunc>(func)(current + 0_value);
                case 1: return std::forward<TFunc>(func)(current + 1_value);
                case 2: return std::forward<TFunc>(func)(current + 2_value);
                case 3: return std::forward<TFunc>(func)(current + 3_value);
                case 4: return std::forward<TFunc>(func)(current + 4_value);
                case 5: return std::forward<TFunc>(func)(current + 5_value);
                case 6: return std::forward<TFunc>(func)(current + 6_value);
                case 7: return std::forward<TFunc>(func)(current + 7_value);
                case 8: return std::forward<TFunc>(func)(current + 8_value);
                case 9: return std::forward<TFunc>(func)(current + 9_value);
                default: HYPERION_UNREACHABLE();
            }
        }
    } // namespace detail

    template<typename TFunc>
        requires std::invocable<TFunc, mpl::Value<0>>
    static constexpr auto
    indexed_call(std::size_t desired, mpl::MetaValue auto bound, TFunc&& func) -> decltype(auto) {
        using mpl::operator""_value;

        if(!std::is_constant_evaluated()) {
            HYPERION_ASSERT_PRECONDITION(
                desired < bound,
                "desired ({}) must be strictly less than the upper bound, bound ({})",
                desired,
                bound);
        }

        return detail::call(desired, bound, 0_value, std::forward<TFunc>(func));
    }
} // namespace hyperion::detail

#endif // HYPERION_STD_DETAIL_INDEXED_CALL_H
