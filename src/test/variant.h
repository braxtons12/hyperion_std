/// @file variant.h
/// @author Braxton Salyer <braxtonsalyer@gmail.com>
/// @brief Unit tests for hyperion::Variant.
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

#ifndef HYPERION_STD_VARIANT_TESTS_H
#define HYPERION_STD_VARIANT_TESTS_H

#include <hyperion/mpl/list.h>
#include <hyperion/mpl/type.h>
#include <hyperion/variant.h>

#include <boost/ut.hpp>

#include <string>

namespace hyperion::_test::variant {
    static_assert(mpl::decltype_<double>().is(
        hyperion::variant::detail::resolve_overload(mpl::List<int, float, double>{},
                                                    mpl::decltype_<double>())));
    static_assert(mpl::decltype_<double>().is(
        hyperion::variant::detail::resolve_overload(mpl::List<int, double>{},
                                                    mpl::decltype_<float>())));
    static_assert(not mpl::decltype_<float>().is(
        hyperion::variant::detail::resolve_overload(mpl::List<int, double>{},
                                                    mpl::decltype_<float>())));
    static_assert(not mpl::decltype_<int>().is(
        hyperion::variant::detail::resolve_overload(mpl::List<int, double>{},
                                                    mpl::decltype_<float>())));

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

    static_assert(not detail::has_same_return_type_for_arg_packs(
        mpl::decltype_(same_return_test_func),
        mpl::List<mpl::List<bool>, mpl::List<int, double>, mpl::List<float, int>>{}));
    static_assert(detail::has_same_return_type_for_arg_packs(
        mpl::decltype_(same_return_test_func2),
        mpl::List<mpl::List<bool>, mpl::List<int, double>, mpl::List<float, int>>{}));

    static_assert(detail::is_bare_type_or_unqualified_lvalue_reference(mpl::decltype_<int>()));
    static_assert(
        detail::is_bare_type_or_unqualified_lvalue_reference(mpl::decltype_<std::string>()));
    static_assert(detail::is_bare_type_or_unqualified_lvalue_reference(mpl::decltype_<int&>()));
    static_assert(
        detail::is_bare_type_or_unqualified_lvalue_reference(mpl::decltype_<std::string&>()));
    static_assert(
        not detail::is_bare_type_or_unqualified_lvalue_reference(mpl::decltype_<int&&>()));
    static_assert(
        not detail::is_bare_type_or_unqualified_lvalue_reference(mpl::decltype_<std::string&&>()));
    static_assert(
        not detail::is_bare_type_or_unqualified_lvalue_reference(mpl::decltype_<const int>()));
    static_assert(not detail::is_bare_type_or_unqualified_lvalue_reference(
        mpl::decltype_<const std::string>()));
    static_assert(
        not detail::is_bare_type_or_unqualified_lvalue_reference(mpl::decltype_<const int&>()));
    static_assert(not detail::is_bare_type_or_unqualified_lvalue_reference(
        mpl::decltype_<const std::string&>()));
    static_assert(
        not detail::is_bare_type_or_unqualified_lvalue_reference(mpl::decltype_<const int&&>()));
    static_assert(not detail::is_bare_type_or_unqualified_lvalue_reference(
        mpl::decltype_<const std::string&&>()));
    static_assert(
        not detail::is_bare_type_or_unqualified_lvalue_reference(mpl::decltype_<volatile int>()));
    static_assert(not detail::is_bare_type_or_unqualified_lvalue_reference(
        mpl::decltype_<volatile std::string>()));
    static_assert(
        not detail::is_bare_type_or_unqualified_lvalue_reference(mpl::decltype_<volatile int&>()));
    static_assert(not detail::is_bare_type_or_unqualified_lvalue_reference(
        mpl::decltype_<volatile std::string&>()));
    static_assert(
        not detail::is_bare_type_or_unqualified_lvalue_reference(mpl::decltype_<volatile int&&>()));
    static_assert(not detail::is_bare_type_or_unqualified_lvalue_reference(
        mpl::decltype_<volatile std::string&&>()));

    // NOLINTNEXTLINE(google-build-using-namespace)
    using namespace boost::ut;
    // NOLINTNEXTLINE(google-build-using-namespace)
    using namespace boost::ut::bdd;

    static inline const suite<"hyperion::variant"> variant_tests = [] {
        scenario("default constructor") = [] {
            struct NotDefaultConstructible {
                NotDefaultConstructible() = delete;
            };

            given("a default constructible type") = [] {
                struct DefaultConstructible { };

                then("a variant with that type as the first alternative is default constructible")
                    = [] {
                          using type = Variant<DefaultConstructible, NotDefaultConstructible>;
                          static_assert(mpl::decltype_<type>().is_default_constructible());

                          expect(that % type{}.holds_alternative<DefaultConstructible>());
                      };
            };

            given("a noexcept default constructible type") = [] {
                struct DefaultConstructible {
#if HYPERION_PLATFORM_COMPILER_IS_CLANG
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunneeded-member-function"
#endif // HYPERION_PLATFORM_COMPILER_IS_CLANG

                    DefaultConstructible() noexcept = default;

#if HYPERION_PLATFORM_COMPILER_IS_CLANG
    #pragma GCC diagnostic pop
#endif // HYPERION_PLATFORM_COMPILER_IS_CLANG
                };

                then("a variant with that type as the first alternative is noexcept default "
                     "constructible")
                    = [] {
                          using type = Variant<DefaultConstructible, NotDefaultConstructible>;
                          static_assert(mpl::decltype_<type>().is_noexcept_default_constructible());

                          expect(that % type{}.holds_alternative<DefaultConstructible>());
                      };
            };

            given("a non-default constructible type") = [] {
                using type = Variant<NotDefaultConstructible, std::string>;
                static_assert(not mpl::decltype_<type>().is_noexcept_default_constructible());
                static_assert(not mpl::decltype_<type>().is_default_constructible());
            };
        };
    };
} // namespace hyperion::_test::variant

#endif // HYPERION_STD_VARIANT_TESTS_H
