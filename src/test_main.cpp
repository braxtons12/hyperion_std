/// @file test_main.cpp
/// @author Braxton Salyer <braxtonsalyer@gmail.com>
/// @brief Unit tests main for hyperion::std.
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

#include <hyperion/platform/def.h>
#include <hyperion/platform/types.h>

#if HYPERION_PLATFORM_COMPILER_IS_CLANG

#include <boost/ut.hpp>

_Pragma("GCC diagnostic push");
_Pragma("GCC diagnostic ignored \"-Wmissing-variable-declarations\"");

template<>
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-avoid-non-const-global-variables)
auto boost::ut::cfg<boost::ut::override> = boost::ut::runner<boost::ut::reporter<boost::ut::printer>>{};

_Pragma("GCC diagnostic pop");

#else

#include <boost/ut.hpp>

#endif // HYPERION_PLATFORM_COMPILER_IS_CLANG

using namespace hyperion; // NOLINT(google-build-using-namespace)

[[nodiscard]] auto
main([[maybe_unused]] i32 argc, [[maybe_unused]] const char* const* argv) -> i32 {
    return 0;
}
