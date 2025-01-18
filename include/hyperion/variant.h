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

#define DECLTYPE(x) decltype(mpl::decltype_<decltype(x)>())

    template<typename... TFuncs>
    struct Overload : TFuncs... {
        using TFuncs::operator()...;
    };
    template<typename... TFuncs>
    Overload(TFuncs...) -> Overload<TFuncs...>;

    namespace detail {
        using mpl::operator""_value;

        [[nodiscard]] constexpr auto
        is_bare_type_or_unqualified_lvalue_reference(mpl::MetaType auto type) {
            return not type.is_const() and not type.is_volatile()
                   and not type.is_rvalue_reference();
        }

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

    class BadVariantAccess final : public std::exception {
      public:
        BadVariantAccess() noexcept = default;
        BadVariantAccess(const BadVariantAccess&) noexcept = default;
        BadVariantAccess(BadVariantAccess&&) noexcept = default;
        ~BadVariantAccess() noexcept final = default;
        auto operator=(const BadVariantAccess&) noexcept -> BadVariantAccess& = default;
        auto operator=(BadVariantAccess&&) noexcept -> BadVariantAccess& = default;

        [[nodiscard]] auto what() const noexcept -> const char* final {
            return message;
        }

      private:
        static constexpr auto* message = "Attempted to access a variant alternative that was not "
                                         "the alternative stored in the variant";
    };

    template<typename TType>
    static constexpr auto in_place_type = mpl::Type<TType>{};

    template<std::size_t TIndex>
    static constexpr auto in_place_index = mpl::Value<TIndex>{};

    namespace detail {
        static constexpr auto is_metatype = [](mpl::MetaType auto type) {
            return mpl::Value<mpl::MetaType<typename decltype(type)::type>, bool>{};
        };

        static constexpr auto is_metavalue = [](mpl::MetaType auto type) {
            return mpl::Value<mpl::MetaValue<typename decltype(type)::type>, bool>{};
        };
    } // namespace detail

    using mpl::operator""_value;

    template<typename... TTypes>
        requires(
            mpl::List<TTypes...>{}.all_of(detail::is_bare_type_or_unqualified_lvalue_reference))
    class Variant : public variant::detail::VariantMoveAssignment<TTypes...> {
      private:
        using storage = typename variant::detail::VariantMoveAssignment<TTypes...>::storage;

      public:
        using size_type = typename storage::size_type;
        static constexpr auto list = storage::list;
        static constexpr auto size = storage::size;
        static constexpr auto invalid_index = storage::invalid_index;

        static constexpr auto variant_alternative(mpl::MetaValue auto _index) {
            return list.at(_index);
        }

        static constexpr auto alternative_index(mpl::MetaType auto type) {
            return list.index_of(type);
        }

      protected:
        template<typename... TArgs>
        constexpr auto construct(mpl::MetaValue auto index, TArgs&&... args) noexcept(
            list.at(decltype(index){}).is_noexcept_constructible_from(mpl::List<TArgs...>{}))
            -> void
            requires(list.at(decltype(index){}).is_constructible_from(mpl::List<TArgs...>{}))
        {
            storage::construct(index, std::forward<TArgs>(args)...);
        }

        template<typename TArg>
        constexpr auto assign(mpl::MetaValue auto index, TArg&& arg) noexcept(
            list.at(decltype(index){}).satisfies(variant::detail::nothrow_assignable<TArg>)) -> void
            requires(list.at(decltype(index){}).satisfies(variant::detail::assignable<TArg>))
        {
            storage::assign(index, std::forward<TArg>(arg));
        }

      public:
        constexpr Variant() noexcept(list.front().is_noexcept_default_constructible())
            requires(list.front().is_default_constructible())
        {
            construct(0_value);
        }

        explicit(false) constexpr Variant(auto&& value) noexcept(
            resolve_overload(list, DECLTYPE(value){})
                .is_noexcept_constructible_from(DECLTYPE(value){}))
            requires(not DECLTYPE(value){}.is(mpl::decltype_<Variant>())
                     and not DECLTYPE(value){}.satisfies(detail::is_metatype)
                     and not DECLTYPE(value){}.satisfies(detail::is_metavalue)
                     and list.contains(resolve_overload(list, DECLTYPE(value){}))
                     and resolve_overload(list, DECLTYPE(value){})
                             .is_constructible_from(DECLTYPE(value){}))
        {
            constexpr auto type = resolve_overload(list, DECLTYPE(value){});
            constexpr auto index = list.indexof(type);
            construct(index, std::forward<decltype(value)>(value));
        }

        template<typename... TArgs>
        constexpr explicit Variant(mpl::MetaType auto type, TArgs&&... args) noexcept(
            decltype(type){}.is_noexcept_constructible_from(mpl::List<TArgs...>{}))
            requires(list.count(decltype(type){})
                     and decltype(type){}.is_constructible_from(mpl::List<TArgs...>{}))
        {
            constexpr auto index = list.index_of(type);
            construct(index, std::forward<TArgs>(args)...);
        }

        template<typename TListType, typename... TArgs>
        constexpr explicit Variant(
            mpl::MetaType auto type,
            std::initializer_list<TListType> ilist,
            TArgs&&... args) noexcept(decltype(type){}
                                          .is_noexcept_constructible_from(
                                              mpl::List<decltype(ilist), TArgs...>{}))
            requires(
                list.count(decltype(type){})
                and decltype(type){}.is_constructible_from(mpl::List<decltype(ilist), TArgs...>{}))
        {
            constexpr auto index = list.index_of(type);
            construct(index, ilist, std::forward<TArgs>(args)...);
        }

        template<typename... TArgs>
        constexpr explicit Variant(mpl::MetaValue auto index, TArgs&&... args) noexcept(
            list.at(decltype(index){}).is_noexcept_constructible_from(mpl::List<TArgs...>{}))
            requires(decltype(index){} < size
                     and list.at(decltype(index){}).is_constructible_from(mpl::List<TArgs...>{}))
        {
            construct(index, std::forward<TArgs>(args)...);
        }

        template<typename TListType, typename... TArgs>
        constexpr explicit Variant(
            mpl::MetaValue auto index,
            std::initializer_list<TListType> ilist,
            TArgs&&... args) noexcept(list.at(decltype(index){})
                                          .is_noexcept_constructible_from(
                                              mpl::List<decltype(ilist), TArgs...>{}))
            requires(decltype(index){} < size
                     and list.at(decltype(index){})
                             .is_constructible_from(mpl::List<decltype(ilist), TArgs...>{}))
        {
            construct(index, ilist, std::forward<TArgs>(args)...);
        }
    };

#undef DECLTYPE
} // namespace hyperion

#endif // HYPERION_VARIANT_H
