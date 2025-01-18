/// @file none.h
/// @author Braxton Salyer <braxtonsalyer@gmail.com>
/// @brief Equivalent to Rust's `None` and C++'s `std::nullopt` and `std::monostate`.
/// @version 0.1
/// @date 2024-08-13
///
/// MIT License
/// @copyright Copyright (c) 2024 Braxton Salyer <braxtonsalyer@gmail.com>
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

#ifndef HYPERION_STD_NONE_H
#define HYPERION_STD_NONE_H

#include <hyperion/platform/types.h>

#include <functional>

namespace hyperion {

    struct None {
        constexpr None() noexcept = default;
        constexpr None(const None&) noexcept = default;
        constexpr None(None&&) noexcept = default;
        constexpr auto operator=(const None&) noexcept -> None& = default;
        constexpr auto operator=(None&&) noexcept -> None&  = default;
    };

}

template<>
struct std::hash<hyperion::None> {

    using result_type [[deprecated("result_type was deprecated in C++17")]] = hyperion::usize;
    using argument_type [[deprecated("argument_type was deprecated in C++17")]] = hyperion::None;

    auto operator()(const hyperion::None&) const noexcept {
        using hyperion::operator""_usize;
        // slightly different value than the "magic value" used for `std::monostate`
        constexpr hyperion::usize _hash = 0xFFFFFFFFFFFFE18F_usize;
        return _hash;
    }
};

#endif // HYPERION_STD_NONE_H
