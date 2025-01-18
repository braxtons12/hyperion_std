/// @file storage.h
/// @author Braxton Salyer <braxtonsalyer@gmail.com>
/// @brief Storage implementation for hyperion::Variant.
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

#ifndef HYPERION_STD_VARIANT_STORAGE_H
#define HYPERION_STD_VARIANT_STORAGE_H

#include <hyperion/detail/indexed_call.h>
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

namespace hyperion::variant::detail {
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

    static constexpr auto ptr_to_reference([[maybe_unused]] mpl::MetaType auto _type, auto&& ptr) {
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

    static constexpr auto enable_ebo(mpl::MetaList auto list) -> bool {
        return list.all_of([](mpl::MetaType auto type) {
            return type.apply(reference_to_ptr).template satisfies<std::is_empty>()
                   and type.apply(reference_to_ptr).template satisfies<std::is_trivial>();
        });
    }

    static constexpr auto disable_ebo(mpl::MetaList auto list) -> bool {
        return list.any_of([](mpl::MetaType auto type) {
            return not type.apply(reference_to_ptr).template satisfies<std::is_empty>()
                   or not type.apply(reference_to_ptr).template satisfies<std::is_trivial>();
        });
    }

    template<usize TIndex, typename... TTypes>
        requires(disable_ebo(mpl::List<TTypes...>{}))
    union VariantUnion;

    template<typename... TTypes>
    struct MetaInfo {
        static constexpr auto list = mpl::List<TTypes...>{}.apply(reference_to_ptr);
        static constexpr auto index_type = calcualte_index_type(list);
        using size_type = typename decltype(index_type.self())::type;
        static constexpr auto size = list.size();

        auto variant(mpl::MetaValue auto _index)
            requires(_index < size)
        {
            return mpl::decltype_<VariantUnion<decltype(_index){}, TTypes...>>();
        }

        auto next_variant(mpl::MetaValue auto _index) {
            if constexpr(_index + 1 < size) {
                return variant(_index + 1_value);
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
        requires(enable_ebo(mpl::List<TTypes...>{}))
    struct VariantEBO : TTypes... {
        using meta_info = MetaInfo<TTypes...>;
        static constexpr auto list = meta_info::list;

        constexpr auto get(mpl::MetaValue auto _index) & noexcept ->
            typename decltype(list.at(_index).as_lvalue_reference())::type
            requires(_index < list.size())
        {
            using type = typename decltype(meta_info::list.at(_index).as_lvalue_reference())::type;
            return static_cast<type>(*this);
        }

        constexpr auto get(mpl::MetaValue auto _index) const& noexcept ->
            typename decltype(list.at(_index).as_const().as_lvalue_reference())::type
            requires(_index < list.size())
        {
            using type = typename decltype(list.at(_index).as_const().as_lvalue_reference())::type;
            return static_cast<type>(*this);
        }

        constexpr auto get(mpl::MetaValue auto _index) && noexcept ->
            typename decltype(list.at(_index).as_rvalue_reference())::type
            requires(_index < list.size())
        {
            using type = typename decltype(list.at(_index).as_rvalue_reference())::type;
            return static_cast<type>(std::move(*this));
        }

        constexpr auto get(mpl::MetaValue auto _index) const&& noexcept ->
            typename decltype(list.at(_index).as_const().as_rvalue_reference())::type
            requires(_index < list.size())
        {
            using type = typename decltype(list.at(_index).as_const().as_rvalue_reference())::type;
            return static_cast<type>(std::move(*this));
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    /////// unroll the union for small list sizes to optimize compile times for common cases ///////
    ////////////////////////////////////////////////////////////////////////////////////////////////

    static inline constexpr auto variant_num_unrolled_instantiations = 5_usize;

    template<usize TIndex, typename TType>
        requires(mpl::decltype_<TType>().apply(reference_to_ptr).is_trivially_destructible())
                and (disable_ebo(mpl::List<TType>{}))
    union VariantUnion<TIndex, TType> {
      public:
        using meta_info = MetaInfo<TType>;
        static constexpr auto index = mpl::Value<TIndex>{};
        static constexpr auto list = meta_info::list;

      private:
        static constexpr auto one = list.at(0_value);
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
        requires(not mpl::decltype_<TType>().apply(reference_to_ptr).is_trivially_destructible())
                and (disable_ebo(mpl::List<TType>{}))
    union VariantUnion<TIndex, TType> {
      public:
        using meta_info = MetaInfo<TType>;
        static constexpr auto index = mpl::Value<TIndex>{};
        static constexpr auto list = meta_info::list;

      private:
        static constexpr auto one = list.at(0_value);
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
        requires(mpl::List<TType1, TType2>{}
                     .apply(reference_to_ptr)
                     .all_of(mpl::trivially_destructible))
                and (disable_ebo(mpl::List<TType1, TType2>{}))
    union VariantUnion<TIndex, TType1, TType2> {
      public:
        using meta_info = MetaInfo<TType1, TType2>;
        static constexpr auto index = mpl::Value<TIndex>{};
        static constexpr auto list = meta_info::list;

      private:
        using t_one = typename decltype(list.at(0_value))::type;
        using t_two = typename decltype(list.at(1_value))::type;

      public:
        constexpr VariantUnion() noexcept = default;
        constexpr ~VariantUnion() noexcept = default;

        t_one m_one;
        t_two m_two;
        None m_none{};

        constexpr auto get(mpl::MetaValue auto _index) & noexcept ->
            typename decltype(ref_to_ptr(_index).as_lvalue_reference())::type {
            constexpr auto type = list.at(_index);
            if constexpr(_index == 0_value) {
                return ptr_to_reference(type, m_one);
            }
            else {
                return ptr_to_reference(type, m_two);
            }
        }

        constexpr auto get(mpl::MetaValue auto _index) const& noexcept ->
            typename decltype(ref_to_ptr(_index).as_const().as_lvalue_reference())::type {
            constexpr auto type = list.at(_index);
            if constexpr(_index == 0_value) {
                return ptr_to_reference(type, m_one);
            }
            else {
                return ptr_to_reference(type, m_two);
            }
        }

        constexpr auto get(mpl::MetaValue auto _index) && noexcept ->
            typename decltype(ref_to_ptr(_index).as_rvalue_reference())::type {
            constexpr auto type = list.at(_index);
            if constexpr(_index == 0_value) {
                return ptr_to_reference(type, std::move(m_one));
            }
            else {
                return ptr_to_reference(type, std::move(m_two));
            }
        }

        constexpr auto get(mpl::MetaValue auto _index) const&& noexcept ->
            typename decltype(ref_to_ptr(_index).as_const().as_rvalue_reference())::type {
            constexpr auto type = list.at(_index);
            if constexpr(_index == 0_value) {
                return ptr_to_reference(type, std::move(m_one));
            }
            else {
                return ptr_to_reference(type, std::move(m_two));
            }
        }
    };

    template<usize TIndex, typename TType1, typename TType2>
        requires(not mpl::List<TType1, TType2>{}
                         .apply(reference_to_ptr)
                         .all_of(mpl::trivially_destructible))
                and (disable_ebo(mpl::List<TType1, TType2>{}))
    union VariantUnion<TIndex, TType1, TType2> {
      public:
        using meta_info = MetaInfo<TType1, TType2>;
        static constexpr auto index = mpl::Value<TIndex>{};
        static constexpr auto list = meta_info::list;

      private:
        using t_one = typename decltype(list.at(0_value))::type;
        using t_two = typename decltype(list.at(1_value))::type;

      public:
        constexpr VariantUnion() noexcept = default;
        constexpr ~VariantUnion() noexcept {
        }

        t_one m_one;
        t_two m_two;
        None m_none{};

        constexpr auto get(mpl::MetaValue auto _index) & noexcept ->
            typename decltype(ref_to_ptr(_index).as_lvalue_reference())::type {
            constexpr auto type = list.at(_index);
            if constexpr(_index == 0_value) {
                return ptr_to_reference(type, m_one);
            }
            else {
                return ptr_to_reference(type, m_two);
            }
        }

        constexpr auto get(mpl::MetaValue auto _index) const& noexcept ->
            typename decltype(ref_to_ptr(_index).as_const().as_lvalue_reference())::type {
            constexpr auto type = list.at(_index);
            if constexpr(_index == 0_value) {
                return ptr_to_reference(type, m_one);
            }
            else {
                return ptr_to_reference(type, m_two);
            }
        }

        constexpr auto get(mpl::MetaValue auto _index) && noexcept ->
            typename decltype(ref_to_ptr(_index).as_rvalue_reference())::type {
            constexpr auto type = list.at(_index);
            if constexpr(_index == 0_value) {
                return ptr_to_reference(type, std::move(m_one));
            }
            else {
                return ptr_to_reference(type, std::move(m_two));
            }
        }

        constexpr auto get(mpl::MetaValue auto _index) const&& noexcept ->
            typename decltype(ref_to_ptr(_index).as_const().as_rvalue_reference())::type {
            constexpr auto type = list.at(_index);
            if constexpr(_index == 0_value) {
                return ptr_to_reference(type, std::move(m_one));
            }
            else {
                return ptr_to_reference(type, std::move(m_two));
            }
        }
    };

    template<usize TIndex, typename TType1, typename TType2, typename TType3>
        requires(mpl::List<TType1, TType2, TType3>{}
                     .apply(reference_to_ptr)
                     .all_of(mpl::trivially_destructible))
                and (disable_ebo(mpl::List<TType1, TType2, TType3>{}))
    union VariantUnion<TIndex, TType1, TType2, TType3> {
      public:
        using meta_info = MetaInfo<TType1, TType2, TType3>;
        static constexpr auto index = mpl::Value<TIndex>{};
        static constexpr auto list = meta_info::list;

      private:
        using t_one = typename decltype(list.at(0_value))::type;
        using t_two = typename decltype(list.at(1_value))::type;
        using t_three = typename decltype(list.at(2_value))::type;

      public:
        constexpr VariantUnion() noexcept = default;
        constexpr ~VariantUnion() noexcept = default;

        t_one m_one;
        t_two m_two;
        t_three m_three;
        None m_none{};

        constexpr auto get(mpl::MetaValue auto _index) & noexcept ->
            typename decltype(ref_to_ptr(_index).as_lvalue_reference())::type {
            constexpr auto type = list.at(_index);
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
            constexpr auto type = list.at(_index);
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
            constexpr auto type = list.at(_index);
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
            constexpr auto type = list.at(_index);
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
        requires(not mpl::List<TType1, TType2, TType3>{}
                         .apply(reference_to_ptr)
                         .all_of(mpl::trivially_destructible))
                and (disable_ebo(mpl::List<TType1, TType2, TType3>{}))
    union VariantUnion<TIndex, TType1, TType2, TType3> {
      public:
        using meta_info = MetaInfo<TType1, TType2, TType3>;
        static constexpr auto index = mpl::Value<TIndex>{};
        static constexpr auto list = meta_info::list;

      private:
        using t_one = typename decltype(list.at(0_value))::type;
        using t_two = typename decltype(list.at(1_value))::type;
        using t_three = typename decltype(list.at(2_value))::type;

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
            constexpr auto type = list.at(_index);
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
            constexpr auto type = list.at(_index);
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
            constexpr auto type = list.at(_index);
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
            constexpr auto type = list.at(_index);
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
        requires(mpl::List<TType1, TType2, TType3, TType4>{}
                     .apply(reference_to_ptr)
                     .all_of(mpl::trivially_destructible))
                and (disable_ebo(mpl::List<TType1, TType2, TType3, TType4>{}))
    union VariantUnion<TIndex, TType1, TType2, TType3, TType4> {
      public:
        using meta_info = MetaInfo<TType1, TType2, TType3, TType4>;
        static constexpr auto index = mpl::Value<TIndex>{};
        static constexpr auto list = meta_info::list;

      private:
        using t_one = typename decltype(list.at(0_value))::type;
        using t_two = typename decltype(list.at(1_value))::type;
        using t_three = typename decltype(list.at(2_value))::type;
        using t_four = typename decltype(list.at(3_value))::type;

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
            constexpr auto type = list.at(_index);
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
            constexpr auto type = list.at(_index);
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
            constexpr auto type = list.at(_index);
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
            constexpr auto type = list.at(_index);
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
        requires(not mpl::List<TType1, TType2, TType3, TType4>{}
                         .apply(reference_to_ptr)
                         .all_of(mpl::trivially_destructible))
                and (disable_ebo(mpl::List<TType1, TType2, TType3, TType4>{}))
    union VariantUnion<TIndex, TType1, TType2, TType3, TType4> {
      public:
        using meta_info = MetaInfo<TType1, TType2, TType3, TType4>;
        static constexpr auto index = mpl::Value<TIndex>{};
        static constexpr auto list = meta_info::list;

      private:
        using t_one = typename decltype(list.at(0_value))::type;
        using t_two = typename decltype(list.at(1_value))::type;
        using t_three = typename decltype(list.at(2_value))::type;
        using t_four = typename decltype(list.at(3_value))::type;

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
            constexpr auto type = list.at(_index);
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
            constexpr auto type = list.at(_index);
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
            constexpr auto type = list.at(_index);
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
            constexpr auto type = list.at(_index);
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
        requires(mpl::List<TType1, TType2, TType3, TType4, TType5>{}
                     .apply(reference_to_ptr)
                     .all_of(mpl::trivially_destructible))
                and (disable_ebo(mpl::List<TType1, TType2, TType3, TType4, TType5>{}))
    union VariantUnion<TIndex, TType1, TType2, TType3, TType4, TType5> {
      public:
        using meta_info = MetaInfo<TType1, TType2, TType3, TType4, TType5>;
        static constexpr auto index = mpl::Value<TIndex>{};
        static constexpr auto list = meta_info::list;

      private:
        using t_one = typename decltype(list.at(0_value))::type;
        using t_two = typename decltype(list.at(1_value))::type;
        using t_three = typename decltype(list.at(2_value))::type;
        using t_four = typename decltype(list.at(3_value))::type;
        using t_five = typename decltype(list.at(4_value))::type;

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
            constexpr auto type = list.at(_index);
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
            constexpr auto type = list.at(_index);
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
            constexpr auto type = list.at(_index);
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
            constexpr auto type = list.at(_index);
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
        requires(not mpl::List<TType1, TType2, TType3, TType4, TType5>{}
                         .apply(reference_to_ptr)
                         .all_of(mpl::trivially_destructible))
                and (disable_ebo(mpl::List<TType1, TType2, TType3, TType4, TType5>{}))
    union VariantUnion<TIndex, TType1, TType2, TType3, TType4, TType5> {
      public:
        using meta_info = MetaInfo<TType1, TType2, TType3, TType4, TType5>;
        static constexpr auto index = mpl::Value<TIndex>{};
        static constexpr auto list = meta_info::list;

      private:
        using t_one = typename decltype(list.at(0_value))::type;
        using t_two = typename decltype(list.at(1_value))::type;
        using t_three = typename decltype(list.at(2_value))::type;
        using t_four = typename decltype(list.at(3_value))::type;
        using t_five = typename decltype(list.at(4_value))::type;

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
            constexpr auto type = list.at(_index);
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
            constexpr auto type = list.at(_index);
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
            constexpr auto type = list.at(_index);
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
            constexpr auto type = list.at(_index);
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
                and (mpl::List<TTypes...>{}
                         .apply(reference_to_ptr)
                         .all_of(mpl::trivially_destructible))
                and (disable_ebo(mpl::List<TTypes...>{}))
    union VariantUnion<TIndex, TTypes...> {
      public:
        using meta_info = MetaInfo<TTypes...>;
        static constexpr auto info = meta_info{};
        static constexpr auto index = mpl::Value<TIndex>{};
        static constexpr auto list = meta_info::list;

      private:
        using t_one = typename decltype(list.at(0_value))::type;
        constexpr auto next = info.next_variant(index);
        using t_next = typename decltype(next)::type;

      public:
        constexpr VariantUnion() noexcept = default;
        constexpr ~VariantUnion() noexcept = default;

        t_one m_self;
        t_next m_next;
        None m_none{};

        constexpr auto get(mpl::MetaValue auto _index) & noexcept ->
            typename decltype(ref_to_ptr(_index).as_lvalue_reference())::type {
            constexpr auto type = list.at(_index);
            if constexpr(_index == index) {
                return ptr_to_reference(type, m_self);
            }
            else {
                return m_next.get(_index);
            }
        }

        constexpr auto get(mpl::MetaValue auto _index) const& noexcept ->
            typename decltype(ref_to_ptr(_index).as_const().as_lvalue_reference())::type {
            constexpr auto type = list.at(_index);
            if constexpr(_index == index) {
                return ptr_to_reference(type, m_self);
            }
            else {
                return m_next.get(_index);
            }
        }

        constexpr auto get(mpl::MetaValue auto _index) && noexcept ->
            typename decltype(ref_to_ptr(_index).as_rvalue_reference())::type {
            constexpr auto type = list.at(_index);
            if constexpr(_index == index) {
                return ptr_to_reference(type, std::move(m_self));
            }
            else {
                return std::move(m_next).get(_index);
            }
        }

        constexpr auto get(mpl::MetaValue auto _index) const&& noexcept ->
            typename decltype(ref_to_ptr(_index).as_const().as_rvalue_reference())::type {
            constexpr auto type = list.at(_index);
            if constexpr(_index == index) {
                return ptr_to_reference(type, std::move(m_self));
            }
            else {
                return std::move(m_next).get(_index);
            }
        }
    };

    template<typename TType>
    concept BaseStorage = requires(mpl::MetaValue auto _index, TType value) {
        {
            value.storage()
        } -> std::convertible_to<const TType&>;
        requires value.get(_index);
    };

    static constexpr auto make_ref_qualified_like([[maybe_unused]] mpl::MetaType auto current,
                                                  [[maybe_unused]] mpl::MetaType auto desired) {
        using res = decltype(desired);
        using cur = decltype(current);
        if constexpr(res{}.is_lvalue_reference()) {
            return cur{}.as_lvalue_reference();
        }
        else if constexpr(res{}.is_rvalue_reference()) {
            return cur{}.as_rvalue_reference();
        }
        else {
            return cur{};
        }
    }

    static constexpr auto make_qualified_like([[maybe_unused]] mpl::MetaType auto current,
                                              [[maybe_unused]] mpl::MetaType auto desired) {
        using res = decltype(desired);
        using cur = decltype(current);
        if constexpr(res{}.is_const()) {
            return make_ref_qualified_like(cur{}.as_const(), res{});
        }
        else {
            return make_ref_qualified_like(cur{}, res{});
        }
    }

    static constexpr auto get(BaseStorage auto&& self, mpl::MetaValue auto _index) ->
        typename decltype(make_qualified_like(DECLTYPE(self.get(_index)){},
                                              DECLTYPE(self){}))::type {
        return std::forward<decltype(self)>(self).get(_index);
    }

    template<typename... TTypes>
    struct VariantStorageBase;

    template<typename... TTypes>
        requires(enable_ebo(mpl::List<TTypes...>{}))
    struct VariantStorageBase<TTypes...> : public VariantEBO<TTypes...> {
        using impl = VariantEBO<TTypes...>;
        using meta_info = typename impl::meta_info;
        static constexpr auto list = impl::list;
        using size_type = typename meta_info::size_type;
        static constexpr auto size = meta_info::SIZE;
        static constexpr size_type invalid_index = static_cast<size_type>(-1);

        size_type m_index = invalid_index;

        constexpr auto set_index([[maybe_unused]] size_type _index) noexcept -> void {
            m_index = _index;
        }

        constexpr auto index() const noexcept -> size_type {
            return m_index;
        }

        constexpr auto storage() & noexcept -> impl& {
            return *this;
        }
        constexpr auto storage() const& noexcept -> const impl& {
            return *this;
        }
        constexpr auto storage() && noexcept -> impl&& {
            return std::move(*this);
        }
        constexpr auto storage() const&& noexcept -> const impl&& {
            return std::move(*this);
        }
    };

    template<typename... TTypes>
        requires(disable_ebo(mpl::List<TTypes...>{}))
    struct VariantStorageBase<TTypes...> {
        using impl = VariantUnion<0_usize, TTypes...>;
        using meta_info = typename impl::meta_info;
        static constexpr auto list = impl::list;
        using size_type = typename meta_info::size_type;
        static constexpr auto size = meta_info::SIZE;
        static constexpr size_type invalid_index = static_cast<size_type>(-1);

        impl m_union;
        size_type m_index = invalid_index;

        constexpr auto set_index([[maybe_unused]] size_type _index) noexcept -> void {
            m_index = _index;
        }

        constexpr auto index() const noexcept -> size_type {
            return m_index;
        }

        constexpr auto storage() & noexcept -> impl& {
            return m_union;
        }
        constexpr auto storage() const& noexcept -> const impl& {
            return m_union;
        }
        constexpr auto storage() && noexcept -> impl&& {
            return std::move(*this).m_union;
        }
        constexpr auto storage() const&& noexcept -> const impl&& {
            return std::move(*this).m_union;
        }
    };

    template<typename TType1>
        requires(disable_ebo(mpl::List<TType1, None>{}))
                and (mpl::decltype_<TType1>().is_lvalue_reference()
                     or mpl::decltype_<TType1>().template satisfies<std::is_pointer>())
    struct VariantStorageBase<TType1, None> {
        using impl = VariantUnion<0_usize, TType1, None>;
        using meta_info = typename impl::meta_info;
        static constexpr auto list = impl::list;
        using size_type = typename meta_info::size_type;
        static constexpr auto size = meta_info::SIZE;
        static constexpr size_type invalid_index = static_cast<size_type>(-1);

        impl m_union;

        constexpr auto set_index([[maybe_unused]] size_type _index) noexcept -> void {
            if(_index == 1) {
                m_union.get(0_value) = nullptr;
            }
        }

        constexpr auto index() const noexcept -> size_type {
            return m_union.get(0_value) == nullptr ? 1 : 0;
        }

        constexpr auto storage() & noexcept -> impl& {
            return m_union;
        }
        constexpr auto storage() const& noexcept -> const impl& {
            return m_union;
        }
        constexpr auto storage() && noexcept -> impl&& {
            return std::move(*this).m_union;
        }
        constexpr auto storage() const&& noexcept -> const impl&& {
            return std::move(*this).m_union;
        }
    };

    template<typename... TTypes>
    struct VariantStorage : public VariantStorageBase<TTypes...> {
        using impl = VariantStorageBase<TTypes...>;
        using meta_info = typename impl::meta_info;
        static constexpr auto list = impl::list;
        using size_type = typename meta_info::size_type;
        static constexpr auto size = meta_info::SIZE;
        static constexpr size_type invalid_index = static_cast<size_type>(-1);

        static constexpr auto variant_alternative(mpl::MetaValue auto _index) {
            return list.at(_index);
        }

        static constexpr auto alternative_index(mpl::MetaType auto type) {
            return list.index_of(type);
        }

        constexpr auto set_index(size_type _index) noexcept -> void {
            static_cast<impl*>(this)->set_index(_index);
        }

        [[nodiscard]] constexpr auto get(mpl::MetaValue auto _index) & noexcept ->
            typename DECLTYPE(get((*this).storage(), _index))::type
            requires(_index < size)
        {
            return get((*this).storage(), _index);
        }
        [[nodiscard]] constexpr auto get(mpl::MetaValue auto _index) const& noexcept ->
            typename DECLTYPE(get((*this).storage(), _index))::type
            requires(_index < size)
        {
            return get((*this).storage(), _index);
        }
        [[nodiscard]] constexpr auto get(mpl::MetaValue auto _index) && noexcept ->
            typename DECLTYPE(get(std::move(*this).storage(), _index))::type
            requires(_index < size)
        {
            return get(std::move(*this).storage(), _index);
        }
        [[nodiscard]] constexpr auto get(mpl::MetaValue auto _index) const&& noexcept ->
            typename DECLTYPE(get(std::move(*this).storage(), _index))::type
            requires(_index < size)
        {
            return get(std::move(*this).storage(), _index);
        }

        [[nodiscard]] constexpr auto get(mpl::MetaType auto type) & noexcept ->
            typename DECLTYPE((*this).get(list.index_of(decltype(type){})))::type
            requires(list.contains(type))
        {
            return (*this).get(list.index_of(decltype(type){}));
        }
        [[nodiscard]] constexpr auto get(mpl::MetaType auto type) const& noexcept ->
            typename DECLTYPE((*this).get(list.index_of(decltype(type){})))::type
            requires(list.contains(type))
        {
            return (*this).get(list.index_of(decltype(type){}));
        }
        [[nodiscard]] constexpr auto get(mpl::MetaType auto type) && noexcept ->
            typename DECLTYPE(std::move(*this).get(list.index_of(decltype(type){})))::type
            requires(list.contains(type))
        {
            return std::move(*this).get(list.index_of(decltype(type){}));
        }
        [[nodiscard]] constexpr auto get(mpl::MetaType auto type) const&& noexcept ->
            typename DECLTYPE(std::move(*this).get(list.index_of(decltype(type){})))::type
            requires(list.contains(type))
        {
            return std::move(*this).get(list.index_of(decltype(type){}));
        }

        [[nodiscard]] static consteval auto
        should_destruct([[maybe_unused]] mpl::MetaValue auto _index) noexcept -> bool {
            if constexpr(decltype(_index){} >= size) {
                return false;
            }
            else {
                constexpr auto type = list.at(decltype(_index){});
                return type.apply(reference_to_ptr).is_destructible()
                       and not type.apply(reference_to_ptr).is_trivially_destructible();
            }
        }

        constexpr auto destruct([[maybe_unused]] mpl::MetaValue auto _index) noexcept(
            list.at(decltype(_index){}.is_noexcept_destructible())) -> void {
            if constexpr(should_destruct(decltype(_index){})) {
                using type = typename decltype(variant_alternative(decltype(_index){}))::type;
                get(decltype(_index){}).~type();
            }
        }

        constexpr auto
        destruct(size_type current_index) noexcept(list.all_of(mpl::noexcept_destructible))
            -> void {
            if(current_index != invalid_index) {
                hyperion::detail::indexed_call(
                    current_index,
                    mpl::Value<size>{},
                    [this](mpl::MetaValue auto _index) { this->destruct(_index); });
                set_index(invalid_index);
            }
        }

        template<typename... TArgs>
        constexpr auto construct(mpl::MetaValue auto _index, TArgs&&... args) noexcept(
            list.at(decltype(_index){}).is_noexcept_constructible_from(mpl::List<TArgs...>{}))
            -> void
            requires(decltype(_index){} < size)
                    and (list.at(decltype(_index){}).is_constructible_from(mpl::List<TArgs...>{}))
                    and (not(list.at(0_value).is_lvalue_reference()
                             and list
                                     == mpl::List<typename decltype(list.at(0_value))::type,
                                                  None>{})
                         or mpl::List<TArgs...>{}.size() == 1_value)
        {
            if constexpr(list.at(0_value).is_lvalue_reference()
                         and list == mpl::List<typename decltype(list.at(0_value))::type, None>{})
            {
                if constexpr(decltype(_index){} == 0_value) {
                    std::construct_at(std::addressof(this->get(0_value)), std::addressof(args)...);
                }
                else {
                    std::construct_at(std::addressof(this->get(0_value)), nullptr);
                }
            }
            else {
                try {
                    if(this->index() != invalid_index) {
                        destruct(this->index());
                    }

                    std::construct_at(std::addressof(this->get(_index)),
                                      std::forward<TArgs>(args)...);
                    set_index(static_cast<size_type>(_index));
                }
                catch(...) {
                    set_index(invalid_index);
                    if constexpr(not list.at(decltype(_index){})
                                         .is_noexcept_constructible_from(mpl::List<TArgs...>{}))
                    {
                        throw;
                    }
                }
            }
        }

        template<typename TArg>
        static constexpr auto nothrow_assignable
            = []([[maybe_unused]] mpl::MetaType auto type) -> bool {
            return std::is_nothrow_assignable_v<typename decltype(type.as_lvalue_reference())::type,
                                                TArg>;
        };
        template<typename TArg>
        static constexpr auto assignable = []([[maybe_unused]] mpl::MetaType auto type) -> bool {
            return std::is_assignable_v<typename decltype(type.as_lvalue_reference())::type, TArg>;
        };

        template<typename TArg>
        constexpr auto
        assign(mpl::MetaValue auto _index,
               TArg&& arg) noexcept(list.at(_index).satisfies(nothrow_assignable<TArg>)) -> void
            requires(list.at(_index).satisfies(assignable<TArg>))
        {
            static constexpr auto new_variant = list.at(decltype(_index){});

            if constexpr(list.at(0_value).is_lvalue_reference()
                         and list == mpl::List<typename decltype(list.at(0_value))::type, None>{})
            {
                if constexpr(decltype(_index){} == 0_value) {
                    if(this->get(0_value) == nullptr) {
                        construct(0_value, std::forward<TArg>(arg));
                    }
                    else {
                        this->get(0_value) = std::addressof(std::forward<TArg>(arg));
                    }
                }
                else {
                    this->get(0_value) = nullptr;
                }
            }
            else {
                try {
                    if(this->index() != _index) {
                        destruct(this->index());

                        if constexpr(new_variant.is_constructible_from(mpl::List<TArg>{})) {
                            std::construct_at(
                                std::addressof(this->get(_index), std::forward<TArg>(arg)));
                            set_index(static_cast<size_type>(_index));
                            return;
                        }
                        else {
                            static_assert(new_variant.is_default_constructible());
                            std::construct_at(std::addressof(this->get(_index)));
                        }
                    }

                    this->get(_index) = std::forward<TArg>(arg);
                    set_index(static_cast<size_type>(_index));
                }
                catch(...) {
                    set_index(invalid_index);
                    if constexpr(not nothrow_assignable<TArg>) {
                        throw;
                    }
                }
            }
        }
    };

    template<typename... TTypes>
    struct VariantDestructor;

    template<typename... TTypes>
        requires(mpl::List<TTypes...>{}.apply(reference_to_ptr).all_of(mpl::trivially_destructible))
    struct VariantDestructor<TTypes...> : public VariantStorage<TTypes...> {
        using storage = VariantStorage<TTypes...>;
        using storage::storage;
        using storage::operator=;

        VariantDestructor() = default;
        VariantDestructor(const VariantDestructor&) = default;
        VariantDestructor(VariantDestructor&&) noexcept = default;
        auto operator=(const VariantDestructor&) -> VariantDestructor& = default;
        auto operator=(VariantDestructor&&) noexcept -> VariantDestructor& = default;

        ~VariantDestructor() noexcept(storage::list.all_of(mpl::noexcept_destructible)) = default;
    };

    template<typename... TTypes>
        requires(
            not mpl::List<TTypes...>{}.apply(reference_to_ptr).all_of(mpl::trivially_destructible))
    struct VariantDestructor<TTypes...> : public VariantStorage<TTypes...> {
        using storage = VariantStorage<TTypes...>;
        using base = storage;
        using base::base;
        using base::operator=;

        VariantDestructor() = default;
        VariantDestructor(const VariantDestructor&) = default;
        VariantDestructor(VariantDestructor&&) noexcept = default;
        auto operator=(const VariantDestructor&) -> VariantDestructor& = default;
        auto operator=(VariantDestructor&&) noexcept -> VariantDestructor& = default;

        ~VariantDestructor() noexcept(storage::list.all_of(mpl::noexcept_destructible)) {
            this->destruct(this->index());
        }
    };

    template<typename... TTypes>
    struct VariantCopyConstructor;

    template<typename... TTypes>
        requires(mpl::List<TTypes...>{}.apply(reference_to_ptr).none_of(mpl::copy_constructible))
    struct VariantCopyConstructor<TTypes...> : public VariantDestructor<TTypes...> {
        using storage = typename VariantDestructor<TTypes...>::storage;
        using base = VariantDestructor<TTypes...>;
        using base::base;
        using base::operator=;

        VariantCopyConstructor() = default;
        VariantCopyConstructor(const VariantCopyConstructor&) = delete;
        VariantCopyConstructor(VariantCopyConstructor&&) noexcept = default;
        auto operator=(const VariantCopyConstructor&) -> VariantCopyConstructor& = default;
        auto operator=(VariantCopyConstructor&&) noexcept -> VariantCopyConstructor& = default;
    };

    template<typename... TTypes>
        requires(mpl::List<TTypes...>{}
                     .apply(reference_to_ptr)
                     .all_of(mpl::trivially_copy_constructible))
    struct VariantCopyConstructor<TTypes...> : public VariantDestructor<TTypes...> {
        using storage = typename VariantDestructor<TTypes...>::storage;
        using base = VariantDestructor<TTypes...>;
        using base::base;
        using base::operator=;

        VariantCopyConstructor() = default;
        VariantCopyConstructor(const VariantCopyConstructor&) noexcept = default;
        VariantCopyConstructor(VariantCopyConstructor&&) noexcept = default;
        auto operator=(const VariantCopyConstructor&) -> VariantCopyConstructor& = default;
        auto operator=(VariantCopyConstructor&&) noexcept -> VariantCopyConstructor& = default;
    };

    template<typename... TTypes>
        requires(mpl::List<TTypes...>{}.apply(reference_to_ptr).all_of(mpl::copy_constructible))
                and (not mpl::List<TTypes...>{}.all_of(mpl::trivially_copy_constructible))
    struct VariantCopyConstructor<TTypes...> : public VariantDestructor<TTypes...> {
        using storage = typename VariantDestructor<TTypes...>::storage;
        using base = VariantDestructor<TTypes...>;
        using base::base;
        using base::operator=;

        VariantCopyConstructor() = default;
        VariantCopyConstructor(VariantCopyConstructor&&) noexcept = default;
        auto operator=(const VariantCopyConstructor&) -> VariantCopyConstructor& = default;
        auto operator=(VariantCopyConstructor&&) noexcept -> VariantCopyConstructor& = default;

        VariantCopyConstructor(const VariantCopyConstructor& var) noexcept(
            storage::list.all_of(mpl::noexcept_copy_constructible)) {
            if(var.index() != storage::invalid_index) {
                hyperion::detail::indexed_call(
                    var.index(),
                    mpl::Value<storage::size>{},
                    [this, &var](mpl::MetaValue auto index) noexcept(
                        storage::list.all_of(mpl::noexcept_copy_constructible)) -> void {
                        this->construct(index, var.get(index));
                    });
            }
        }
    };

    template<typename... TTypes>
    struct VariantCopyAssignment;

    template<typename... TTypes>
        requires(mpl::List<TTypes>{}.apply(reference_to_ptr).none_of(mpl::copy_assignable))
    struct VariantCopyAssignment : public VariantCopyConstructor<TTypes...> {
        using storage = typename VariantCopyConstructor<TTypes...>::storage;
        using base = VariantCopyConstructor<TTypes...>;
        using base::base;
        using base::operator=;

        VariantCopyAssignment() = default;
        VariantCopyAssignment(const VariantCopyAssignment&) = default;
        VariantCopyAssignment(VariantCopyAssignment&&) noexcept = default;
        auto operator=(const VariantCopyAssignment&) -> VariantCopyAssignment& = delete;
        auto operator=(VariantCopyAssignment&&) noexcept -> VariantCopyAssignment& = default;
    };

    template<typename... TTypes>
        requires(mpl::List<TTypes>{}.apply(reference_to_ptr).all_of(mpl::trivially_copy_assignable))
    struct VariantCopyAssignment : public VariantCopyConstructor<TTypes...> {
        using storage = typename VariantCopyConstructor<TTypes...>::storage;
        using base = VariantCopyConstructor<TTypes...>;
        using base::base;
        using base::operator=;

        VariantCopyAssignment() = default;
        VariantCopyAssignment(const VariantCopyAssignment&) = default;
        VariantCopyAssignment(VariantCopyAssignment&&) noexcept = default;
        auto operator=(const VariantCopyAssignment&) -> VariantCopyAssignment& = default;
        auto operator=(VariantCopyAssignment&&) noexcept -> VariantCopyAssignment& = default;
    };

    template<typename... TTypes>
        requires(mpl::List<TTypes>{}.apply(reference_to_ptr).all_of(mpl::copy_assignable)
                 and not mpl::List<TTypes>{}
                             .apply(reference_to_ptr)
                             .all_of(mpl::trivially_copy_assignable))
    struct VariantCopyAssignment : public VariantCopyConstructor<TTypes...> {
        using storage = typename VariantCopyConstructor<TTypes...>::storage;
        using base = VariantCopyConstructor<TTypes...>;
        using base::base;
        using base::operator=;

        VariantCopyAssignment() = default;
        VariantCopyAssignment(const VariantCopyAssignment&) = default;
        VariantCopyAssignment(VariantCopyAssignment&&) noexcept = default;
        auto operator=(VariantCopyAssignment&&) noexcept -> VariantCopyAssignment& = default;

        auto operator=(const VariantCopyAssignment& var) noexcept(
            storage::list.all_of(mpl::noexcept_copy_assignable)) -> VariantCopyAssignment& {
            if(this == &var) {
                return *this;
            }

            hyperion::detail::indexed_call(
                var.index(),
                mpl::Value<storage::size>{},
                [this, &var](mpl::MetaValue auto index) noexcept(
                    storage::list.all_of(mpl::noexcept_copy_assignable)) -> void {
                    this->assign(index, var.get(index));
                });

            return *this;
        }
    };

    template<typename... TTypes>
    struct VariantMoveConstructor;

    template<typename... TTypes>
        requires(mpl::List<TTypes>{}.apply(reference_to_ptr).none_of(mpl::move_constructible))
    struct VariantMoveConstructor : public VariantCopyAssignment<TTypes...> {
        using storage = typename VariantCopyAssignment<TTypes...>::storage;
        using base = VariantCopyAssignment<TTypes...>;
        using base::base;
        using base::operator=;

        VariantMoveConstructor() = default;
        VariantMoveConstructor(const VariantMoveConstructor&) = default;
        VariantMoveConstructor(VariantMoveConstructor&&) noexcept = delete;
        auto operator=(const VariantMoveConstructor&) -> VariantMoveConstructor& = default;
        auto operator=(VariantMoveConstructor&&) noexcept -> VariantMoveConstructor& = default;
    };

    template<typename... TTypes>
        requires(
            mpl::List<TTypes>{}.apply(reference_to_ptr).all_of(mpl::trivially_move_constructible))
    struct VariantMoveConstructor : public VariantCopyAssignment<TTypes...> {
        using storage = typename VariantCopyAssignment<TTypes...>::storage;
        using base = VariantCopyAssignment<TTypes...>;
        using base::base;
        using base::operator=;

        VariantMoveConstructor() = default;
        VariantMoveConstructor(const VariantMoveConstructor&) = default;
        VariantMoveConstructor(VariantMoveConstructor&&) noexcept = default;
        auto operator=(const VariantMoveConstructor&) -> VariantMoveConstructor& = default;
        auto operator=(VariantMoveConstructor&&) noexcept -> VariantMoveConstructor& = default;
    };

    template<typename... TTypes>
        requires(mpl::List<TTypes>{}.apply(reference_to_ptr).all_of(mpl::move_constructible)
                 and not mpl::List<TTypes>{}
                             .apply(reference_to_ptr)
                             .all_of(mpl::trivially_move_constructible))
    struct VariantMoveConstructor : public VariantCopyAssignment<TTypes...> {
        using storage = typename VariantCopyAssignment<TTypes...>::storage;
        using base = VariantCopyAssignment<TTypes...>;
        using base::base;
        using base::operator=;

        VariantMoveConstructor() = default;
        VariantMoveConstructor(const VariantMoveConstructor&) = default;
        auto operator=(const VariantMoveConstructor&) -> VariantMoveConstructor& = default;
        auto operator=(VariantMoveConstructor&&) noexcept -> VariantMoveConstructor& = default;

        VariantMoveConstructor(VariantMoveConstructor&& var) noexcept(
            storage::list.all_of(mpl::noexcept_move_constructible)) {
            if(var.index() != storage::invalid_index) {
                hyperion::detail::indexed_call(
                    var.index(),
                    mpl::Value<storage::size>{},
                    [this, &var](mpl::MetaValue auto index) noexcept(
                        storage::list.all_of(mpl::noexcept_move_constructible)) -> void {
                        this->construct(index, std::move(var).get(index));
                        var.set_index(storage::invalid_index);
                    });
            }
        }
    };

    template<typename TReturn, typename TFunc, typename TVariant, typename TIndexSequence>
    struct JumpTableGenerator;

    template<typename TReturn, typename TFunc, typename TVariant, std::size_t... indices>
    struct JumpTableGenerator<TReturn, TFunc, TVariant, std::index_sequence<indices...>> {
        using array = std::array<TReturn (*)(TFunc, TVariant), sizeof...(indices)>;

      private:
        static constexpr auto generate_elements([[maybe_unused]] array& _array,
                                                [[maybe_unused]] mpl::MetaValue auto _index) {
            _array[_index] = []<typename TCall, typename TVar>(TCall&& func, TVar&& var)
                requires(mpl::decltype_<TCall>().is_qualification_of(mpl::decltype_<TFunc>())
                         and mpl::decltype_<TVar>().is_qualification_of(mpl::decltype_<TVariant>()))
            {
                return std::forward<TCall>(func)(std::forward<TVar>(var).get(decltype(_index){}));
            };

            if constexpr((decltype(_index){} + 1_value) < sizeof...(indices)) {
                generate_elements(_array, _index + 1_value);
            }
        }

      public:
        static constexpr auto generate_table() -> array {
            array _array;
            generate_elements(_array, 0_value);
            return _array;
        }
    };
} // namespace hyperion::variant::detail

#undef DECLTYPE

#endif // HYPERION_STD_VARIANT_STORAGE_H
