/// @file test_main.cpp
/// @author Braxton Salyer <braxtonsalyer@gmail.com>
/// @brief Unit tests main for hyperion::std.
/// @version 0.1
/// @date 2025-01-19
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

#include <hyperion/platform/def.h>
#include <hyperion/platform/types.h>

#include <boost/ut.hpp>

#if HYPERION_PLATFORM_COMPILER_IS_CLANG
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wmissing-variable-declarations"
#endif // HYPERION_PLATFORM_COMPILER_IS_CLANG

template<>
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-avoid-non-const-global-variables)
auto boost::ut::cfg<boost::ut::override>
    = boost::ut::runner<boost::ut::reporter<boost::ut::printer>>{};

#if HYPERION_PLATFORM_COMPILER_IS_CLANG
    #pragma GCC diagnostic pop
#endif // HYPERION_PLATFORM_COMPILER_IS_CLANG

#include "test/variant.h"

using namespace hyperion; // NOLINT(google-build-using-namespace)

[[nodiscard]] auto main([[maybe_unused]] i32 argc, [[maybe_unused]] const char** argv) -> i32 {
    return static_cast<i32>(
        boost::ut::cfg<boost::ut::override>.run(boost::ut::run_cfg{.argc = argc, .argv = argv}));
}
