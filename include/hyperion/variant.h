/// @file variant.h
/// @author Braxton Salyer <braxtonsalyer@gmail.com>
/// @brief Hyperion alternative to standard library `variant`, `hyperion::Variant`,
/// with improved ergonomics, safety, and reduced possibility of valueless by exception state.
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

        constexpr auto is_bare_type_or_unqualified_lvalue_reference
            = []([[maybe_unused]] mpl::MetaType auto type) {
                  return not decltype(type){}.is_const() and not decltype(type){}.is_volatile()
                         and not decltype(type){}.is_rvalue_reference();
              };

        constexpr auto has_same_return_type_for_arg_packs
            = []([[maybe_unused]] mpl::MetaType auto func,
                 [[maybe_unused]] mpl::MetaList auto arg_packs) noexcept {
                  constexpr auto have_same_return_type = []([[maybe_unused]] mpl::MetaList auto
                                                                list) {
                      constexpr auto first_list = decltype(arg_packs){}.at(0_value);
                      constexpr auto get_return_type =
                          []<typename... TTypes>([[maybe_unused]] mpl::Type<TTypes>... types) {
                              return mpl::decltype_<
                                  std::invoke_result_t<typename decltype(func)::type, TTypes...>>();
                          };
                      constexpr auto first_type = first_list.unwrap(get_return_type);

                      return first_type.is(list.unwrap(get_return_type));
                  };

                  return arg_packs.all_of(have_same_return_type);
              };
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
        requires(mpl::List<TTypes...>{}
                     .all_of(detail::is_bare_type_or_unqualified_lvalue_reference)
                     .value_of())
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
        static constexpr auto noexcept_assignable_requirements
            = variant::detail::variant_noexcept_assignable_requirements(list);

        static constexpr auto assignable_requirements
            = variant::detail::variant_assignable_requirements(list);

        template<typename... TArgs>
        constexpr auto construct(mpl::MetaValue auto _index, TArgs&&... args) noexcept(
            list.at(decltype(_index){}).is_noexcept_constructible_from(mpl::List<TArgs...>{}))
            -> void
            requires(
                list.at(decltype(_index){}).is_constructible_from(mpl::List<TArgs...>{}).value_of())
        {
            storage::construct(_index, std::forward<TArgs>(args)...);
        }

        template<typename TArg>
        constexpr auto assign(mpl::MetaValue auto _index, TArg&& arg) noexcept(
            list.at(decltype(_index){})
                .satisfies(variant::detail::nothrow_assignable(DECLTYPE(arg){}))
            or list.at(decltype(_index){}.is_nothrow_constructible_from(DECLTYPE(arg){}))) -> void
            requires(list.at(decltype(_index){})
                         .satisfies(variant::detail::assignable(DECLTYPE(arg){}))
                         .value_of())
                    or (list.at(decltype(_index){})
                            .is_constructible_from(DECLTYPE(arg){})
                            .value_of())
        {
            storage::assign(_index, std::forward<TArg>(arg));
        }

      public:
        constexpr Variant() noexcept(list.front().is_noexcept_default_constructible())
            requires(list.front().is_default_constructible().value_of())
        {
            construct(0_value);
        }

        explicit(false) constexpr Variant(auto&& value) noexcept(
            resolve_overload(list, DECLTYPE(value){})
                .is_noexcept_constructible_from(DECLTYPE(value){}))
            requires(not DECLTYPE(value){}.is(mpl::decltype_<Variant>()).value_of())
                    and (not DECLTYPE(value){}.satisfies(detail::is_metatype).value_of())
                    and (not DECLTYPE(value){}.satisfies(detail::is_metavalue).value_of())
                    and (list.contains(resolve_overload(list, DECLTYPE(value){})).value_of)
                    and (resolve_overload(list, DECLTYPE(value){})
                             .is_constructible_from(DECLTYPE(value){})
                             .value_of())
        {
            constexpr auto type = resolve_overload(list, DECLTYPE(value){});
            constexpr auto _index = list.index_of(type);
            construct(_index, std::forward<decltype(value)>(value));
        }

        template<typename... TArgs>
        constexpr explicit Variant(mpl::MetaType auto type, TArgs&&... args) noexcept(
            decltype(type){}.is_noexcept_constructible_from(mpl::List<TArgs...>{}))
            requires(list.count(decltype(type){}) == 1_value)
                    and (decltype(type){}.is_constructible_from(mpl::List<TArgs...>{}).value_of())
        {
            constexpr auto _index = list.index_of(type);
            construct(_index, std::forward<TArgs>(args)...);
        }

        template<typename TListType, typename... TArgs>
        constexpr explicit Variant(
            mpl::MetaType auto type,
            std::initializer_list<TListType> ilist,
            TArgs&&... args) noexcept(decltype(type){}
                                          .is_noexcept_constructible_from(
                                              mpl::List<decltype(ilist), TArgs...>{}))
            requires(list.count(decltype(type){}) == 1_value)
                    and (decltype(type){}
                             .is_constructible_from(mpl::List<decltype(ilist), TArgs...>{})
                             .value_of())
        {
            constexpr auto _index = list.index_of(type);
            construct(_index, ilist, std::forward<TArgs>(args)...);
        }

        template<typename... TArgs>
        constexpr explicit Variant(mpl::MetaValue auto _index, TArgs&&... args) noexcept(
            list.at(decltype(_index){}).is_noexcept_constructible_from(mpl::List<TArgs...>{}))
            requires(decltype(_index){} < size)
                    and (list.at(decltype(_index){})
                             .is_constructible_from(mpl::List<TArgs...>{})
                             .value_of())
        {
            construct(_index, std::forward<TArgs>(args)...);
        }

        template<typename TListType, typename... TArgs>
        constexpr explicit Variant(
            mpl::MetaValue auto _index,
            std::initializer_list<TListType> ilist,
            TArgs&&... args) noexcept(list.at(decltype(_index){})
                                          .is_noexcept_constructible_from(
                                              mpl::List<decltype(ilist), TArgs...>{}))
            requires(decltype(_index){} < size)
                    and (list.at(decltype(_index){})
                             .is_constructible_from(mpl::List<decltype(ilist), TArgs...>{})
                             .value_of())
        {
            construct(_index, ilist, std::forward<TArgs>(args)...);
        }

        constexpr auto operator=(auto&& value) noexcept(
            DECLTYPE(value){}.satisfies(noexcept_assignable_requirements))
                -> Variant& requires(
                    DECLTYPE(value){}.satisfies(assignable_requirements).value_of())
            and (not DECLTYPE(value){}.is_qualification_of(mpl::decltype_<Variant>()).value_of()) {
            assign(list.index_of(resolve_overload(list, DECLTYPE(value){})),
                   std::forward<decltype(value)>(value));
            return *this;
        }

        [[nodiscard]] constexpr auto index() const noexcept -> size_type {
            return static_cast<const storage*>(this)->index();
        }

        [[nodiscard]] constexpr auto
        holds_alternative(mpl::MetaType auto type) const noexcept -> bool {
            return index() == list.index_of(type);
        }

        template<typename TType>
        [[nodiscard]] constexpr auto holds_alternative() const noexcept -> bool {
            return index() == list.index_of(mpl::decltype_<TType>());
        }

        [[nodiscard]] constexpr auto holds_alternative(std::size_t _index) const noexcept -> bool {
            return index() == _index;
        }

        [[nodiscard]] constexpr auto is(mpl::MetaType auto type) const noexcept -> bool {
            return index() == list.index_of(type);
        }

        template<typename TType>
        [[nodiscard]] constexpr auto is() const noexcept -> bool {
            return index() == list.index_of(mpl::decltype_<TType>());
        }

        [[nodiscard]] constexpr auto is(std::size_t _index) const noexcept -> bool {
            return index() == _index;
        }

        [[nodiscard]] constexpr auto is_valueless() const noexcept -> bool {
            return index() == invalid_index;
        }

        [[nodiscard]] constexpr auto valueless_by_exception() const noexcept -> bool {
            return index() == invalid_index;
        }

        [[nodiscard]] constexpr auto get(mpl::MetaValue auto _index) & -> decltype(auto)
            requires(decltype(_index){} < size)
        {
            if(not is(_index)) {
                throw BadVariantAccess();
            }
            return static_cast<storage&>(*this).get(_index);
        }

        [[nodiscard]] constexpr auto get(mpl::MetaValue auto _index) const& -> decltype(auto)
            requires(decltype(_index){} < size)
        {
            if(not is(_index)) {
                throw BadVariantAccess();
            }
            return static_cast<const storage&>(*this).get(_index);
        }

        [[nodiscard]] constexpr auto get(mpl::MetaValue auto _index) && -> decltype(auto)
            requires(decltype(_index){} < size)
        {
            if(not is(_index)) {
                throw BadVariantAccess();
            }
            return static_cast<storage&&>(std::move(*this)).get(_index);
        }

        [[nodiscard]] constexpr auto get(mpl::MetaValue auto _index) const&& -> decltype(auto)
            requires(decltype(_index){} < size)
        {
            if(not is(_index)) {
                throw BadVariantAccess();
            }
            return static_cast<const storage&&>(std::move(*this)).get(_index);
        }

        template<std::size_t TIndex>
            requires(TIndex < size)
        [[nodiscard]] constexpr auto get() & -> decltype(auto) {
            if(not is(mpl::Value<TIndex>{})) {
                throw BadVariantAccess();
            }
            return static_cast<storage&>(*this).get(mpl::Value<TIndex>{});
        }

        template<std::size_t TIndex>
            requires(TIndex < size)
        [[nodiscard]] constexpr auto get() const& -> decltype(auto) {
            if(not is(mpl::Value<TIndex>{})) {
                throw BadVariantAccess();
            }
            return static_cast<const storage&>(*this).get(mpl::Value<TIndex>{});
        }

        template<std::size_t TIndex>
            requires(TIndex < size)
        [[nodiscard]] constexpr auto get() && -> decltype(auto) {
            if(not is(mpl::Value<TIndex>{})) {
                throw BadVariantAccess();
            }
            return static_cast<storage&&>(std::move(*this)).get(mpl::Value<TIndex>{});
        }

        template<std::size_t TIndex>
            requires(TIndex < size)
        [[nodiscard]] constexpr auto get() const&& -> decltype(auto) {
            if(not is(mpl::Value<TIndex>{})) {
                throw BadVariantAccess();
            }
            return static_cast<const storage&&>(std::move(*this)).get(mpl::Value<TIndex>{});
        }

        [[nodiscard]] constexpr auto get(mpl::MetaType auto type) & -> decltype(auto)
            requires(list.count(type) == 1_value)
        {
            if(not is(type)) {
                throw BadVariantAccess();
            }
            return static_cast<storage&>(*this).get(list.index_of(type));
        }

        [[nodiscard]] constexpr auto get(mpl::MetaType auto type) const& -> decltype(auto)
            requires(list.count(type) == 1_value)
        {
            if(not is(type)) {
                throw BadVariantAccess();
            }
            return static_cast<const storage&>(*this).get(list.index_of(type));
        }

        [[nodiscard]] constexpr auto get(mpl::MetaType auto type) && -> decltype(auto)
            requires(list.count(type) == 1_value)
        {
            if(not is(type)) {
                throw BadVariantAccess();
            }
            return static_cast<storage&&>(std::move(*this)).get(list.index_of(type));
        }

        [[nodiscard]] constexpr auto get(mpl::MetaType auto type) const&& -> decltype(auto)
            requires(list.count(type) == 1_value)
        {
            if(not is(type)) {
                throw BadVariantAccess();
            }
            return static_cast<const storage&&>(std::move(*this)).get(list.index_of(type));
        }

        template<typename TType>
            requires(list.count(mpl::decltype_<TType>()) == 1_value)
        [[nodiscard]] constexpr auto get() & -> decltype(auto) {
            if(not is(mpl::decltype_<TType>())) {
                throw BadVariantAccess();
            }
            return static_cast<storage&>(*this).get(list.index_of(mpl::decltype_<TType>()));
        }

        template<typename TType>
            requires(list.count(mpl::decltype_<TType>()) == 1_value)
        [[nodiscard]] constexpr auto get() const& -> decltype(auto) {
            if(not is(mpl::decltype_<TType>())) {
                throw BadVariantAccess();
            }
            return static_cast<const storage&>(*this).get(list.index_of(mpl::decltype_<TType>()));
        }

        template<typename TType>
            requires(list.count(mpl::decltype_<TType>()) == 1_value)
        [[nodiscard]] constexpr auto get() && -> decltype(auto) {
            if(not is(mpl::decltype_<TType>())) {
                throw BadVariantAccess();
            }
            return static_cast<storage&&>(std::move(*this))
                .get(list.index_of(mpl::decltype_<TType>()));
        }

        template<typename TType>
            requires(list.count(mpl::decltype_<TType>()) == 1_value)
        [[nodiscard]] constexpr auto get() const&& -> decltype(auto) {
            if(not is(mpl::decltype_<TType>())) {
                throw BadVariantAccess();
            }
            return static_cast<const storage&&>(std::move(*this))
                .get(list.index_of(mpl::decltype_<TType>()));
        }
    };

#undef DECLTYPE
} // namespace hyperion

#endif // HYPERION_VARIANT_H
