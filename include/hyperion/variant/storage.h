/// @file storage.h
/// @author Braxton Salyer <braxtonsalyer@gmail.com>
/// @brief Storage implementation for hyperion::Variant.
/// @version 0.1
/// @date 2025-01-16
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

#ifndef HYPERION_STD_ENUM_STORAGE_H
#define HYPERION_STD_ENUM_STORAGE_H

#include <hyperion/mpl/concepts.h>
#include <hyperion/mpl/list.h>
#include <hyperion/mpl/metapredicates.h>
#include <hyperion/mpl/type.h>
#include <hyperion/mpl/type_traits.h>
#include <hyperion/mpl/value.h>
#include <hyperion/none.h>
#include <hyperion/platform/def.h>
#include <hyperion/platform/types.h>

#define DECLTYPE(x) decltype(mpl::decltype_(x))

namespace hyperion::enum_::detail {
    using mpl::operator""_value;

    template<typename... TTypes>
    struct OverloadResolution;

    template<>
    struct OverloadResolution<> {
        void operator()() const;
    };

    template<typename TFirst, typename... TRest>
    struct OverloadResolution<TFirst, TRest...> : OverloadResolution<TRest...> {
        using OverloadResolution<TRest...>::operator();

        auto operator()(TFirst) const -> TFirst;
    };

    template<typename TArg, typename... TTypes>
    auto resolve_overload([[maybe_unused]] mpl::List<TTypes...> types,
                          [[maybe_unused]] mpl::Type<TArg> type) {
        return mpl::decltype_<decltype(OverloadResolution<TTypes...>{}(std::declval<TArg>()))>();
    }

    template<typename... TTypes>
    auto calculate_index_type(mpl::List<TTypes...> list) {
        if constexpr(list.size() < std::numeric_limits<u8>::max()) {
            return mpl::decltype_<u8>();
        }
        else if constexpr(list.size() < std::numeric_limits<u16>::max()) {
            return mpl::decltype_<u16>();
        }
        else if constexpr(list.size() < std::numeric_limits<u32>::max()) {
            return mpl::decltype_<u32>();
        }
        else {
            return mpl::decltype_<u64>();
        }
    }

    auto reference_to_ptr(mpl::MetaType auto type) {
        if constexpr(type.template apply<std::remove_reference>() != type) {
            return type.template apply<std::remove_reference>().template apply<std::add_pointer>();
        }
        else {
            return type;
        }
    }

    static constexpr auto ptr_to_reference(mpl::MetaType auto _type, auto&& ptr) {
        using type = DECLTYPE(ptr);
        using actual = decltype(_type);
        if constexpr(type{}
                         .template apply<std::remove_cvref_t>()
                         .template satisfies<std::is_pointer>()
                     and actual{}.template statisfies<std::is_reference>())
        {
            return *ptr;
        }
        else {
            return std::forward<decltype(ptr)>(ptr);
        }
    }

    template<usize TIndex, typename... TTypes>
        requires(mpl::List<TTypes...>{}.any_of(
            [](mpl::MetaType auto type) { return not type.template satisfies<std::is_empty>(); }))
    union VariantUnion;

    template<typename... TTypes>
    struct MetaInfo {
        static constexpr auto list = mpl::List<TTypes...>{};
        static constexpr auto index_type = calcualte_index_type(list);
        using size_type = typename decltype(index_type.self())::type;
        static constexpr auto SIZE = list.size();

        auto variant(mpl::MetaValue auto index)
            requires(index < SIZE)
        {
            return mpl::decltype_<VariantUnion<index, TTypes...>>();
        }

        auto next_variant(mpl::MetaValue auto index) {
            if constexpr(index + 1 < SIZE) {
                return variant(index + 1_value);
            }
            else {
                return mpl::decltype_<None>();
            }
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////// we manually handle the special member functions to support compilers that ///////////
    ////////// have not implemented conditionally trivially special member functions yet ///////////
    ////////// The actual SMFs are implemented one step higher in the composition of the ///////////
    ////////// hierarchy used to implement the overall storage of `Variant`. We also do  ///////////
    ////////// the same special casing in order to optimize for empty types (clang and   ///////////
    ////////// MSVC do not currently support the `[[no_unique_address]]` attribute on MS ///////////
    ////////////////////////////////////////////////////////////////////////////////////////////////

    template<typename... TTypes>
        requires(mpl::List<TTypes...>{}.all_of(
            [](mpl::MetaType auto type) { return type.template satisfies<std::is_empty>(); }))
    struct VariantEBO : TTypes... {
        using meta_info = MetaInfo<TTypes...>;
        static constexpr auto list = meta_info::list;

        constexpr auto get(mpl::MetaValue auto index) & noexcept ->
            typename decltype(list.at(index).as_lvalue_reference())::type
            requires(index < list.size())
        {
            using type = typename decltype(meta_info::list.at(index).as_lvalue_reference())::type;
            return static_cast<type>(*this);
        }

        constexpr auto get(mpl::MetaValue auto index) const& noexcept ->
            typename decltype(list.at(index).as_const().as_lvalue_reference())::type
            requires(index < list.size())
        {
            using type = typename decltype(list.at(index).as_const().as_lvalue_reference())::type;
            return static_cast<type>(*this);
        }

        constexpr auto get(mpl::MetaValue auto index) && noexcept ->
            typename decltype(list.at(index).as_rvalue_reference())::type
            requires(index < list.size())
        {
            using type = typename decltype(list.at(index).as_rvalue_reference())::type;
            return static_cast<type>(std::move(*this));
        }

        constexpr auto get(mpl::MetaValue auto index) const&& noexcept ->
            typename decltype(list.at(index).as_const().as_rvalue_reference())::type
            requires(index < list.size())
        {
            using type = typename decltype(list.at(index).as_const().as_rvalue_reference())::type;
            return static_cast<type>(std::move(*this));
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    /////// unroll the union for small list sizes to optimize compile times for common cases ///////
    ////////////////////////////////////////////////////////////////////////////////////////////////

    static inline constexpr auto variant_num_unrolled_instantiations = 5_usize;

    template<usize TIndex, typename TType>
        requires(mpl::decltype_<TType>().is_trivially_destructible())
                and (mpl::List<TType>{}.any_of([](mpl::MetaType auto type) {
                        return not type.template satisfies<std::is_empty>();
                    }))
    union VariantUnion<TIndex, TType> {
      public:
        using meta_info = MetaInfo<TType>;
        static constexpr auto index = mpl::Value<TIndex>{};
        static constexpr auto list = meta_info::list;

      private:
        static constexpr auto ref_to_ptr(mpl::MetaValue auto idx) {
            return reference_to_ptr(list.at(idx));
        }

        static constexpr auto one = ref_to_ptr(0_value);
        using t_one = typename decltype(one.self())::type;

      public:
        constexpr VariantUnion() noexcept = default;
        constexpr ~VariantUnion() noexcept = default;

        t_one m_one;
        None m_none{};

        constexpr auto get(mpl::MetaValue auto _index) & noexcept ->
            typename decltype(one.as_lvalue_reference())::type
            requires(_index < list.size())
        {
            return ptr_to_reference(list.at(_index), m_one);
        }
        constexpr auto get(mpl::MetaValue auto _index) const& noexcept ->
            typename decltype(one.as_const().as_lvalue_reference())::type
            requires(_index < list.size())
        {
            return ptr_to_reference(list.at(_index), m_one);
        }
        constexpr auto get(mpl::MetaValue auto _index) && noexcept ->
            typename decltype(one.as_rvalue_reference())::type
            requires(_index < list.size())
        {
            return ptr_to_reference(list.at(_index), std::move(m_one));
        }
        constexpr auto get(mpl::MetaValue auto _index) const&& noexcept ->
            typename decltype(one.as_const().as_rvalue_reference())::type
            requires(_index < list.size())
        {
            return ptr_to_reference(list.at(_index), std::move(m_one));
        }
    };

    template<usize TIndex, typename TType>
        requires(not mpl::decltype_<TType>().is_trivially_destructible())
                and (mpl::List<TType>{}.any_of([](mpl::MetaType auto type) {
                        return not type.template satisfies<std::is_empty>();
                    }))
    union VariantUnion<TIndex, TType> {
      public:
        using meta_info = MetaInfo<TType>;
        static constexpr auto index = mpl::Value<TIndex>{};
        static constexpr auto list = meta_info::list;

      private:
        static constexpr auto ref_to_ptr(mpl::MetaValue auto idx) {
            return reference_to_ptr(list.at(idx));
        }

        static constexpr auto one = ref_to_ptr(0_value);
        using t_one = typename decltype(one.self())::type;

      public:
        constexpr VariantUnion() noexcept = default;
        constexpr ~VariantUnion() noexcept {
        }

        t_one m_one;
        None m_none{};

        constexpr auto get(mpl::MetaValue auto _index) & noexcept ->
            typename decltype(one.as_lvalue_reference())::type {
            return ptr_to_reference(list.at(_index), m_one);
        }
        constexpr auto get(mpl::MetaValue auto _index) const& noexcept ->
            typename decltype(one.as_const().as_lvalue_reference())::type {
            return ptr_to_reference(list.at(_index), m_one);
        }
        constexpr auto get(mpl::MetaValue auto _index) && noexcept ->
            typename decltype(one.as_rvalue_reference())::type {
            return ptr_to_reference(list.at(_index), std::move(m_one));
        }
        constexpr auto get(mpl::MetaValue auto _index) const&& noexcept ->
            typename decltype(one.as_const().as_rvalue_reference())::type {
            return ptr_to_reference(list.at(_index), std::move(m_one));
        }
    };

    template<usize TIndex, typename TType1, typename TType2>
        requires(mpl::List<TType1, TType2>{}.all_of(mpl::trivially_destructible))
                and (mpl::List<TType1, TType2>{}.any_of([](mpl::MetaType auto type) {
                        return not type.template satisfies<std::is_empty>();
                    }))
    union VariantUnion<TIndex, TType1, TType2> {
      public:
        using meta_info = MetaInfo<TType1, TType2>;
        static constexpr auto index = mpl::Value<TIndex>{};
        static constexpr auto list = meta_info::list;

      private:
        static constexpr auto ref_to_ptr(mpl::MetaValue auto idx) {
            return reference_to_ptr(list.at(idx));
        }

        using t_one = typename decltype(ref_to_ptr(0_value))::type;
        using t_two = typename decltype(ref_to_ptr(1_value))::type;

      public:
        constexpr VariantUnion() noexcept = default;
        constexpr ~VariantUnion() noexcept = default;

        t_one m_one;
        t_two m_two;
        None m_none{};

        constexpr auto get(mpl::MetaValue auto _index) & noexcept ->
            typename decltype(ref_to_ptr(_index).as_lvalue_reference())::type {
            constexpr auto type = list.at(index);
            if constexpr(_index == 0_value) {
                return ptr_to_reference(type, m_one);
            }
            else {
                return ptr_to_reference(type, m_two);
            }
        }

        constexpr auto get(mpl::MetaValue auto _index) const& noexcept ->
            typename decltype(ref_to_ptr(_index).as_const().as_lvalue_reference())::type {
            constexpr auto type = list.at(index);
            if constexpr(_index == 0_value) {
                return ptr_to_reference(type, m_one);
            }
            else {
                return ptr_to_reference(type, m_two);
            }
        }

        constexpr auto get(mpl::MetaValue auto _index) && noexcept ->
            typename decltype(ref_to_ptr(_index).as_rvalue_reference())::type {
            constexpr auto type = list.at(index);
            if constexpr(_index == 0_value) {
                return ptr_to_reference(type, std::move(m_one));
            }
            else {
                return ptr_to_reference(type, std::move(m_two));
            }
        }

        constexpr auto get(mpl::MetaValue auto _index) const&& noexcept ->
            typename decltype(ref_to_ptr(_index).as_const().as_rvalue_reference())::type {
            constexpr auto type = list.at(index);
            if constexpr(_index == 0_value) {
                return ptr_to_reference(type, std::move(m_one));
            }
            else {
                return ptr_to_reference(type, std::move(m_two));
            }
        }
    };

    template<usize TIndex, typename TType1, typename TType2>
        requires(not mpl::List<TType1, TType2>{}.all_of(mpl::trivially_destructible))
                and (mpl::List<TType1, TType2>{}.any_of([](mpl::MetaType auto type) {
                        return not type.template satisfies<std::is_empty>();
                    }))
    union VariantUnion<TIndex, TType1, TType2> {
      public:
        using meta_info = MetaInfo<TType1, TType2>;
        static constexpr auto index = mpl::Value<TIndex>{};
        static constexpr auto list = meta_info::list;

      private:
        static constexpr auto ref_to_ptr(mpl::MetaValue auto idx) {
            return reference_to_ptr(list.at(idx));
        }

        using t_one = typename decltype(ref_to_ptr(0_value))::type;
        using t_two = typename decltype(ref_to_ptr(1_value))::type;

      public:
        constexpr VariantUnion() noexcept = default;
        constexpr ~VariantUnion() noexcept {
        }

        t_one m_one;
        t_two m_two;
        None m_none{};

        constexpr auto get(mpl::MetaValue auto _index) & noexcept ->
            typename decltype(ref_to_ptr(_index).as_lvalue_reference())::type {
            constexpr auto type = list.at(index);
            if constexpr(_index == 0_value) {
                return ptr_to_reference(type, m_one);
            }
            else {
                return ptr_to_reference(type, m_two);
            }
        }

        constexpr auto get(mpl::MetaValue auto _index) const& noexcept ->
            typename decltype(ref_to_ptr(_index).as_const().as_lvalue_reference())::type {
            constexpr auto type = list.at(index);
            if constexpr(_index == 0_value) {
                return ptr_to_reference(type, m_one);
            }
            else {
                return ptr_to_reference(type, m_two);
            }
        }

        constexpr auto get(mpl::MetaValue auto _index) && noexcept ->
            typename decltype(ref_to_ptr(_index).as_rvalue_reference())::type {
            constexpr auto type = list.at(index);
            if constexpr(_index == 0_value) {
                return ptr_to_reference(type, std::move(m_one));
            }
            else {
                return ptr_to_reference(type, std::move(m_two));
            }
        }

        constexpr auto get(mpl::MetaValue auto _index) const&& noexcept ->
            typename decltype(ref_to_ptr(_index).as_const().as_rvalue_reference())::type {
            constexpr auto type = list.at(index);
            if constexpr(_index == 0_value) {
                return ptr_to_reference(type, std::move(m_one));
            }
            else {
                return ptr_to_reference(type, std::move(m_two));
            }
        }
    };

    template<usize TIndex, typename TType1, typename TType2, typename TType3>
        requires(mpl::List<TType1, TType2, TType3>{}.all_of(mpl::trivially_destructible))
                and (mpl::List<TType1, TType2, TType3>{}.any_of([](mpl::MetaType auto type) {
                        return not type.template satisfies<std::is_empty>();
                    }))
    union VariantUnion<TIndex, TType1, TType2, TType3> {
      public:
        using meta_info = MetaInfo<TType1, TType2, TType3>;
        static constexpr auto index = mpl::Value<TIndex>{};
        static constexpr auto list = meta_info::list;

      private:
        static constexpr auto ref_to_ptr(mpl::MetaValue auto idx) {
            return reference_to_ptr(list.at(idx));
        }

        using t_one = typename decltype(ref_to_ptr(0_value))::type;
        using t_two = typename decltype(ref_to_ptr(1_value))::type;
        using t_three = typename decltype(ref_to_ptr(2_value))::type;

      public:
        constexpr VariantUnion() noexcept = default;
        constexpr ~VariantUnion() noexcept = default;

        t_one m_one;
        t_two m_two;
        t_three m_three;
        None m_none{};

        constexpr auto get(mpl::MetaValue auto _index) & noexcept ->
            typename decltype(ref_to_ptr(_index).as_lvalue_reference())::type {
            constexpr auto type = list.at(index);
            if constexpr(_index == 0_value) {
                return ptr_to_reference(type, m_one);
            }
            else if constexpr(_index == 1_value) {
                return ptr_to_reference(type, m_two);
            }
            else {
                return ptr_to_reference(type, m_three);
            }
        }

        constexpr auto get(mpl::MetaValue auto _index) const& noexcept ->
            typename decltype(ref_to_ptr(_index).as_const().as_lvalue_reference())::type {
            constexpr auto type = list.at(index);
            if constexpr(_index == 0_value) {
                return ptr_to_reference(type, m_one);
            }
            else if constexpr(_index == 1_value) {
                return ptr_to_reference(type, m_two);
            }
            else {
                return ptr_to_reference(type, m_three);
            }
        }

        constexpr auto get(mpl::MetaValue auto _index) && noexcept ->
            typename decltype(ref_to_ptr(_index).as_rvalue_reference())::type {
            constexpr auto type = list.at(index);
            if constexpr(_index == 0_value) {
                return ptr_to_reference(type, std::move(m_one));
            }
            else if constexpr(_index == 1_value) {
                return ptr_to_reference(type, std::move(m_two));
            }
            else {
                return ptr_to_reference(type, std::move(m_three));
            }
        }

        constexpr auto get(mpl::MetaValue auto _index) const&& noexcept ->
            typename decltype(ref_to_ptr(_index).as_const().as_rvalue_reference())::type {
            constexpr auto type = list.at(index);
            if constexpr(_index == 0_value) {
                return ptr_to_reference(type, std::move(m_one));
            }
            else if constexpr(_index == 1_value) {
                return ptr_to_reference(type, std::move(m_two));
            }
            else {
                return ptr_to_reference(type, std::move(m_three));
            }
        }
    };

    template<usize TIndex, typename TType1, typename TType2, typename TType3>
        requires(not mpl::List<TType1, TType2, TType3>{}.all_of(mpl::trivially_destructible))
                and (mpl::List<TType1, TType2, TType3>{}.any_of([](mpl::MetaType auto type) {
                        return not type.template satisfies<std::is_empty>();
                    }))
    union VariantUnion<TIndex, TType1, TType2, TType3> {
      public:
        using meta_info = MetaInfo<TType1, TType2, TType3>;
        static constexpr auto index = mpl::Value<TIndex>{};
        static constexpr auto list = meta_info::list;

      private:
        static constexpr auto ref_to_ptr(mpl::MetaValue auto idx) {
            return reference_to_ptr(list.at(idx));
        }

        using t_one = typename decltype(ref_to_ptr(0_value))::type;
        using t_two = typename decltype(ref_to_ptr(1_value))::type;
        using t_three = typename decltype(ref_to_ptr(2_value))::type;

      public:
        constexpr VariantUnion() noexcept = default;
        constexpr ~VariantUnion() noexcept {
        }

        t_one m_one;
        t_two m_two;
        t_three m_three;
        None m_none{};

        constexpr auto get(mpl::MetaValue auto _index) & noexcept ->
            typename decltype(ref_to_ptr(_index).as_lvalue_reference())::type {
            constexpr auto type = list.at(index);
            if constexpr(_index == 0_value) {
                return ptr_to_reference(type, m_one);
            }
            else if constexpr(_index == 1_value) {
                return ptr_to_reference(type, m_two);
            }
            else {
                return ptr_to_reference(type, m_three);
            }
        }

        constexpr auto get(mpl::MetaValue auto _index) const& noexcept ->
            typename decltype(ref_to_ptr(_index).as_const().as_lvalue_reference())::type {
            constexpr auto type = list.at(index);
            if constexpr(_index == 0_value) {
                return ptr_to_reference(type, m_one);
            }
            else if constexpr(_index == 1_value) {
                return ptr_to_reference(type, m_two);
            }
            else {
                return ptr_to_reference(type, m_three);
            }
        }

        constexpr auto get(mpl::MetaValue auto _index) && noexcept ->
            typename decltype(ref_to_ptr(_index).as_rvalue_reference())::type {
            constexpr auto type = list.at(index);
            if constexpr(_index == 0_value) {
                return ptr_to_reference(type, std::move(m_one));
            }
            else if constexpr(_index == 1_value) {
                return ptr_to_reference(type, std::move(m_two));
            }
            else {
                return ptr_to_reference(type, std::move(m_three));
            }
        }

        constexpr auto get(mpl::MetaValue auto _index) const&& noexcept ->
            typename decltype(ref_to_ptr(_index).as_const().as_rvalue_reference())::type {
            constexpr auto type = list.at(index);
            if constexpr(_index == 0_value) {
                return ptr_to_reference(type, std::move(m_one));
            }
            else if constexpr(_index == 1_value) {
                return ptr_to_reference(type, std::move(m_two));
            }
            else {
                return ptr_to_reference(type, std::move(m_three));
            }
        }
    };

    template<usize TIndex, typename TType1, typename TType2, typename TType3, typename TType4>
        requires(mpl::List<TType1, TType2, TType3, TType4>{}.all_of(mpl::trivially_destructible))
                and (mpl::List<TType1, TType2, TType3, TType4>{}.any_of(
                    [](mpl::MetaType auto type) {
                        return not type.template satisfies<std::is_empty>();
                    }))
    union VariantUnion<TIndex, TType1, TType2, TType3, TType4> {
      public:
        using meta_info = MetaInfo<TType1, TType2, TType3, TType4>;
        static constexpr auto index = mpl::Value<TIndex>{};
        static constexpr auto list = meta_info::list;

      private:
        static constexpr auto ref_to_ptr(mpl::MetaValue auto idx) {
            return reference_to_ptr(list.at(idx));
        }

        using t_one = typename decltype(ref_to_ptr(0_value))::type;
        using t_two = typename decltype(ref_to_ptr(1_value))::type;
        using t_three = typename decltype(ref_to_ptr(2_value))::type;
        using t_four = typename decltype(ref_to_ptr(3_value))::type;

      public:
        constexpr VariantUnion() noexcept = default;
        constexpr ~VariantUnion() noexcept = default;

        t_one m_one;
        t_two m_two;
        t_three m_three;
        t_four m_four;
        None m_none{};

        constexpr auto get(mpl::MetaValue auto _index) & noexcept ->
            typename decltype(ref_to_ptr(_index).as_lvalue_reference())::type {
            constexpr auto type = list.at(index);
            if constexpr(_index == 0_value) {
                return ptr_to_reference(type, m_one);
            }
            else if constexpr(_index == 1_value) {
                return ptr_to_reference(type, m_two);
            }
            else if constexpr(_index == 2_value) {
                return ptr_to_reference(type, m_three);
            }
            else {
                return ptr_to_reference(type, m_four);
            }
        }

        constexpr auto get(mpl::MetaValue auto _index) const& noexcept ->
            typename decltype(ref_to_ptr(_index).as_const().as_lvalue_reference())::type {
            constexpr auto type = list.at(index);
            if constexpr(_index == 0_value) {
                return ptr_to_reference(type, m_one);
            }
            else if constexpr(_index == 1_value) {
                return ptr_to_reference(type, m_two);
            }
            else if constexpr(_index == 2_value) {
                return ptr_to_reference(type, m_three);
            }
            else {
                return ptr_to_reference(type, m_four);
            }
        }

        constexpr auto get(mpl::MetaValue auto _index) && noexcept ->
            typename decltype(ref_to_ptr(_index).as_rvalue_reference())::type {
            constexpr auto type = list.at(index);
            if constexpr(_index == 0_value) {
                return ptr_to_reference(type, std::move(m_one));
            }
            else if constexpr(_index == 1_value) {
                return ptr_to_reference(type, std::move(m_two));
            }
            else if constexpr(_index == 2_value) {
                return ptr_to_reference(type, std::move(m_three));
            }
            else {
                return ptr_to_reference(type, std::move(m_four));
            }
        }

        constexpr auto get(mpl::MetaValue auto _index) const&& noexcept ->
            typename decltype(ref_to_ptr(_index).as_const().as_rvalue_reference())::type {
            constexpr auto type = list.at(index);
            if constexpr(_index == 0_value) {
                return ptr_to_reference(type, std::move(m_one));
            }
            else if constexpr(_index == 1_value) {
                return ptr_to_reference(type, std::move(m_two));
            }
            else if constexpr(_index == 2_value) {
                return ptr_to_reference(type, std::move(m_three));
            }
            else {
                return ptr_to_reference(type, std::move(m_four));
            }
        }
    };

    template<usize TIndex, typename TType1, typename TType2, typename TType3, typename TType4>
        requires(not mpl::List<TType1, TType2, TType3, TType4>{}.all_of(
                    mpl::trivially_destructible))
                and (mpl::List<TType1, TType2, TType3, TType4>{}.any_of(
                    [](mpl::MetaType auto type) {
                        return not type.template satisfies<std::is_empty>();
                    }))
    union VariantUnion<TIndex, TType1, TType2, TType3, TType4> {
      public:
        using meta_info = MetaInfo<TType1, TType2, TType3, TType4>;
        static constexpr auto index = mpl::Value<TIndex>{};
        static constexpr auto list = meta_info::list;

      private:
        static constexpr auto ref_to_ptr(mpl::MetaValue auto idx) {
            return reference_to_ptr(list.at(idx));
        }

        using t_one = typename decltype(ref_to_ptr(0_value))::type;
        using t_two = typename decltype(ref_to_ptr(1_value))::type;
        using t_three = typename decltype(ref_to_ptr(2_value))::type;
        using t_four = typename decltype(ref_to_ptr(3_value))::type;

      public:
        constexpr VariantUnion() noexcept = default;
        constexpr ~VariantUnion() noexcept {
        }

        t_one m_one;
        t_two m_two;
        t_three m_three;
        t_four m_four;
        None m_none{};

        constexpr auto get(mpl::MetaValue auto _index) & noexcept ->
            typename decltype(ref_to_ptr(_index).as_lvalue_reference())::type {
            constexpr auto type = list.at(index);
            if constexpr(_index == 0_value) {
                return ptr_to_reference(type, m_one);
            }
            else if constexpr(_index == 1_value) {
                return ptr_to_reference(type, m_two);
            }
            else if constexpr(_index == 2_value) {
                return ptr_to_reference(type, m_three);
            }
            else {
                return ptr_to_reference(type, m_four);
            }
        }

        constexpr auto get(mpl::MetaValue auto _index) const& noexcept ->
            typename decltype(ref_to_ptr(_index).as_const().as_lvalue_reference())::type {
            constexpr auto type = list.at(index);
            if constexpr(_index == 0_value) {
                return ptr_to_reference(type, m_one);
            }
            else if constexpr(_index == 1_value) {
                return ptr_to_reference(type, m_two);
            }
            else if constexpr(_index == 2_value) {
                return ptr_to_reference(type, m_three);
            }
            else {
                return ptr_to_reference(type, m_four);
            }
        }

        constexpr auto get(mpl::MetaValue auto _index) && noexcept ->
            typename decltype(ref_to_ptr(_index).as_rvalue_reference())::type {
            constexpr auto type = list.at(index);
            if constexpr(_index == 0_value) {
                return ptr_to_reference(type, std::move(m_one));
            }
            else if constexpr(_index == 1_value) {
                return ptr_to_reference(type, std::move(m_two));
            }
            else if constexpr(_index == 2_value) {
                return ptr_to_reference(type, std::move(m_three));
            }
            else {
                return ptr_to_reference(type, std::move(m_four));
            }
        }

        constexpr auto get(mpl::MetaValue auto _index) const&& noexcept ->
            typename decltype(ref_to_ptr(_index).as_const().as_rvalue_reference())::type {
            constexpr auto type = list.at(index);
            if constexpr(_index == 0_value) {
                return ptr_to_reference(type, std::move(m_one));
            }
            else if constexpr(_index == 1_value) {
                return ptr_to_reference(type, std::move(m_two));
            }
            else if constexpr(_index == 2_value) {
                return ptr_to_reference(type, std::move(m_three));
            }
            else {
                return ptr_to_reference(type, std::move(m_four));
            }
        }
    };

    template<usize TIndex,
             typename TType1,
             typename TType2,
             typename TType3,
             typename TType4,
             typename TType5>
        requires(mpl::List<TType1, TType2, TType3, TType4, TType5>{}.all_of(
                    mpl::trivially_destructible))
                and (mpl::List<TType1, TType2, TType3, TType4, TType5>{}.any_of(
                    [](mpl::MetaType auto type) {
                        return not type.template satisfies<std::is_empty>();
                    }))
    union VariantUnion<TIndex, TType1, TType2, TType3, TType4, TType5> {
      public:
        using meta_info = MetaInfo<TType1, TType2, TType3, TType4, TType5>;
        static constexpr auto index = mpl::Value<TIndex>{};
        static constexpr auto list = meta_info::list;

      private:
        static constexpr auto ref_to_ptr(mpl::MetaValue auto idx) {
            return reference_to_ptr(list.at(idx));
        }

        using t_one = typename decltype(ref_to_ptr(0_value))::type;
        using t_two = typename decltype(ref_to_ptr(1_value))::type;
        using t_three = typename decltype(ref_to_ptr(2_value))::type;
        using t_four = typename decltype(ref_to_ptr(3_value))::type;
        using t_five = typename decltype(ref_to_ptr(4_value))::type;

      public:
        constexpr VariantUnion() noexcept = default;
        constexpr ~VariantUnion() noexcept = default;

        t_one m_one;
        t_two m_two;
        t_three m_three;
        t_four m_four;
        t_five m_five;
        None m_none{};

        constexpr auto get(mpl::MetaValue auto _index) & noexcept ->
            typename decltype(ref_to_ptr(_index).as_lvalue_reference())::type {
            constexpr auto type = list.at(index);
            if constexpr(_index == 0_value) {
                return ptr_to_reference(type, m_one);
            }
            else if constexpr(_index == 1_value) {
                return ptr_to_reference(type, m_two);
            }
            else if constexpr(_index == 2_value) {
                return ptr_to_reference(type, m_three);
            }
            else if constexpr(_index == 3_value) {
                return ptr_to_reference(type, m_four);
            }
            else {
                return ptr_to_reference(type, m_five);
            }
        }

        constexpr auto get(mpl::MetaValue auto _index) const& noexcept ->
            typename decltype(ref_to_ptr(_index).as_const().as_lvalue_reference())::type {
            constexpr auto type = list.at(index);
            if constexpr(_index == 0_value) {
                return ptr_to_reference(type, m_one);
            }
            else if constexpr(_index == 1_value) {
                return ptr_to_reference(type, m_two);
            }
            else if constexpr(_index == 2_value) {
                return ptr_to_reference(type, m_three);
            }
            else if constexpr(_index == 3_value) {
                return ptr_to_reference(type, m_four);
            }
            else {
                return ptr_to_reference(type, m_five);
            }
        }

        constexpr auto get(mpl::MetaValue auto _index) && noexcept ->
            typename decltype(ref_to_ptr(_index).as_rvalue_reference())::type {
            constexpr auto type = list.at(index);
            if constexpr(_index == 0_value) {
                return ptr_to_reference(type, std::move(m_one));
            }
            else if constexpr(_index == 1_value) {
                return ptr_to_reference(type, std::move(m_two));
            }
            else if constexpr(_index == 2_value) {
                return ptr_to_reference(type, std::move(m_three));
            }
            else if constexpr(_index == 3_value) {
                return ptr_to_reference(type, std::move(m_four));
            }
            else {
                return ptr_to_reference(type, std::move(m_five));
            }
        }

        constexpr auto get(mpl::MetaValue auto _index) const&& noexcept ->
            typename decltype(ref_to_ptr(_index).as_const().as_rvalue_reference())::type {
            constexpr auto type = list.at(index);
            if constexpr(_index == 0_value) {
                return ptr_to_reference(type, std::move(m_one));
            }
            else if constexpr(_index == 1_value) {
                return ptr_to_reference(type, std::move(m_two));
            }
            else if constexpr(_index == 2_value) {
                return ptr_to_reference(type, std::move(m_three));
            }
            else if constexpr(_index == 3_value) {
                return ptr_to_reference(type, std::move(m_four));
            }
            else {
                return ptr_to_reference(type, std::move(m_five));
            }
        }
    };

    template<usize TIndex,
             typename TType1,
             typename TType2,
             typename TType3,
             typename TType4,
             typename TType5>
        requires(not mpl::List<TType1, TType2, TType3, TType4, TType5>{}.all_of(
                    mpl::trivially_destructible))
                and (mpl::List<TType1, TType2, TType3, TType4, TType5>{}.any_of(
                    [](mpl::MetaType auto type) {
                        return not type.template satisfies<std::is_empty>();
                    }))
    union VariantUnion<TIndex, TType1, TType2, TType3, TType4, TType5> {
      public:
        using meta_info = MetaInfo<TType1, TType2, TType3, TType4, TType5>;
        static constexpr auto index = mpl::Value<TIndex>{};
        static constexpr auto list = meta_info::list;

      private:
        static constexpr auto ref_to_ptr(mpl::MetaValue auto idx) {
            return reference_to_ptr(list.at(idx));
        }

        using t_one = typename decltype(ref_to_ptr(0_value))::type;
        using t_two = typename decltype(ref_to_ptr(1_value))::type;
        using t_three = typename decltype(ref_to_ptr(2_value))::type;
        using t_four = typename decltype(ref_to_ptr(3_value))::type;
        using t_five = typename decltype(ref_to_ptr(4_value))::type;

      public:
        constexpr VariantUnion() noexcept = default;
        constexpr ~VariantUnion() noexcept {
        }

        t_one m_one;
        t_two m_two;
        t_three m_three;
        t_four m_four;
        t_five m_five;
        None m_none{};

        constexpr auto get(mpl::MetaValue auto _index) & noexcept ->
            typename decltype(ref_to_ptr(_index).as_lvalue_reference())::type {
            constexpr auto type = list.at(index);
            if constexpr(_index == 0_value) {
                return ptr_to_reference(type, m_one);
            }
            else if constexpr(_index == 1_value) {
                return ptr_to_reference(type, m_two);
            }
            else if constexpr(_index == 2_value) {
                return ptr_to_reference(type, m_three);
            }
            else if constexpr(_index == 3_value) {
                return ptr_to_reference(type, m_four);
            }
            else {
                return ptr_to_reference(type, m_five);
            }
        }

        constexpr auto get(mpl::MetaValue auto _index) const& noexcept ->
            typename decltype(ref_to_ptr(_index).as_const().as_lvalue_reference())::type {
            constexpr auto type = list.at(index);
            if constexpr(_index == 0_value) {
                return ptr_to_reference(type, m_one);
            }
            else if constexpr(_index == 1_value) {
                return ptr_to_reference(type, m_two);
            }
            else if constexpr(_index == 2_value) {
                return ptr_to_reference(type, m_three);
            }
            else if constexpr(_index == 3_value) {
                return ptr_to_reference(type, m_four);
            }
            else {
                return ptr_to_reference(type, m_five);
            }
        }

        constexpr auto get(mpl::MetaValue auto _index) && noexcept ->
            typename decltype(ref_to_ptr(_index).as_rvalue_reference())::type {
            constexpr auto type = list.at(index);
            if constexpr(_index == 0_value) {
                return ptr_to_reference(type, std::move(m_one));
            }
            else if constexpr(_index == 1_value) {
                return ptr_to_reference(type, std::move(m_two));
            }
            else if constexpr(_index == 2_value) {
                return ptr_to_reference(type, std::move(m_three));
            }
            else if constexpr(_index == 3_value) {
                return ptr_to_reference(type, std::move(m_four));
            }
            else {
                return ptr_to_reference(type, std::move(m_five));
            }
        }

        constexpr auto get(mpl::MetaValue auto _index) const&& noexcept ->
            typename decltype(ref_to_ptr(_index).as_const().as_rvalue_reference())::type {
            constexpr auto type = list.at(index);
            if constexpr(_index == 0_value) {
                return ptr_to_reference(type, std::move(m_one));
            }
            else if constexpr(_index == 1_value) {
                return ptr_to_reference(type, std::move(m_two));
            }
            else if constexpr(_index == 2_value) {
                return ptr_to_reference(type, std::move(m_three));
            }
            else if constexpr(_index == 3_value) {
                return ptr_to_reference(type, std::move(m_four));
            }
            else {
                return ptr_to_reference(type, std::move(m_five));
            }
        }
    };

    template<usize TIndex, typename... TTypes>
        requires(mpl::List<TTypes...>{}.size() > variant_num_unrolled_instantiations)
                and (mpl::List<TTypes...>{}.all_of(mpl::trivially_destructible))
                and (mpl::List<TTypes...>{}.any_of([](mpl::MetaType auto type) {
                        return not type.template satisfies<std::is_empty>();
                    }))
    union VariantUnion<TIndex, TTypes...> {
      public:
        using meta_info = MetaInfo<TTypes...>;
        static constexpr auto info = meta_info{};
        static constexpr auto index = mpl::Value<TIndex>{};
        static constexpr auto list = meta_info::list;

      private:
        static constexpr auto ref_to_ptr(mpl::MetaValue auto idx) {
            return reference_to_ptr(list.at(idx));
        }

        using t_one = typename decltype(ref_to_ptr(0_value))::type;
        constexpr auto next = info.next_variant(index);
        using t_next = typename std::remove_cvref_t<decltype(next)>::type;

      public:
        constexpr VariantUnion() noexcept = default;
        constexpr ~VariantUnion() noexcept = default;

        t_one m_self;
        t_next m_next;
        None m_none{};

        constexpr auto get(mpl::MetaValue auto _index) & noexcept ->
            typename decltype(ref_to_ptr(_index).as_lvalue_reference())::type {
            constexpr auto type = list.at(index);
            if constexpr(_index == index) {
                return ptr_to_reference(type, m_self);
            }
            else {
                return m_next.get(_index);
            }
        }

        constexpr auto get(mpl::MetaValue auto _index) const& noexcept ->
            typename decltype(ref_to_ptr(_index).as_const().as_lvalue_reference())::type {
            constexpr auto type = list.at(index);
            if constexpr(_index == index) {
                return ptr_to_reference(type, m_self);
            }
            else {
                return m_next.get(_index);
            }
        }

        constexpr auto get(mpl::MetaValue auto _index) && noexcept ->
            typename decltype(ref_to_ptr(_index).as_rvalue_reference())::type {
            constexpr auto type = list.at(index);
            if constexpr(_index == index) {
                return ptr_to_reference(type, std::move(m_self));
            }
            else {
                return std::move(m_next).get(_index);
            }
        }

        constexpr auto get(mpl::MetaValue auto _index) const&& noexcept ->
            typename decltype(ref_to_ptr(_index).as_const().as_rvalue_reference())::type {
            constexpr auto type = list.at(index);
            if constexpr(_index == index) {
                return ptr_to_reference(type, std::move(m_self));
            }
            else {
                return std::move(m_next).get(_index);
            }
        }
    };
} // namespace hyperion::enum_::detail

#undef DECLTYPE

#endif // HYPERION_STD_ENUM_STORAGE_H
