/// @file storage.h
/// @author Braxton Salyer <braxtonsalyer@gmail.com>
/// @brief Storage implementation for hyperion::Enum.
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
        mpl::decltype_<decltype(OverloadResolution<TTypes...>{}(std::declval<TArg>()))>();
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

    static constexpr auto ptr_to_reference(auto&& ptr) {
        using type = decltype(mpl::decltype_(ptr));
        if constexpr(type{}
                         .template apply<std::remove_cvref_t>()
                         .template satisfies<std::is_pointer>())
        {
            return *ptr;
        }
        else {
            return std::forward<decltype(ptr)>(ptr);
        }
    }

    template<typename... TTypes>
    struct MetaInfo {
        static constexpr auto list = mpl::List<TTypes...>{};
        static constexpr auto index_type = calcualte_index_type(list);
        using size_type = typename decltype(index_type.self())::type;
        static constexpr auto SIZE = list.size();

        auto variant(mpl::MetaValue auto index)
            requires(index < SIZE)
        {
            return list.at(index);
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
    ////////// hierarchy used to implement the overall storage of `Enum`. We also do the ///////////
    ////////// same special casing in order to optimize for empty types (clang + MSVC do ///////////
    ////////// not currently support the `[[no_unique_address]]` attribute on MS Windows ///////////
    ////////////////////////////////////////////////////////////////////////////////////////////////

    template<typename... TTypes>
        requires(mpl::List<TTypes...>{}.all_of(
            [](mpl::MetaType auto type) { return type.template satisfies<std::is_empty>(); }))
    struct EnumEBO : public TTypes... {
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

    template<usize TIndex, typename... TTypes>
        requires(mpl::List<TTypes...>{}.any_of(
            [](mpl::MetaType auto type) { return not type.template satisfies<std::is_empty>(); }))
    union EnumUnion;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    /////// unroll the union for small list sizes to optimize compile times for common cases ///////
    ////////////////////////////////////////////////////////////////////////////////////////////////

    static inline constexpr auto enum_num_unrolled_instantiations = 5_usize;

    template<usize TIndex, typename TType>
        requires(mpl::decltype_<TType>().is_trivially_destructible())
                and (mpl::List<TType>{}.any_of([](mpl::MetaType auto type) {
                        return not type.template satisfies<std::is_empty>();
                    }))
    union EnumUnion<TIndex, TType> {
      private:
        static constexpr auto ref_to_ptr(mpl::MetaValue auto idx) {
            return reference_to_ptr(meta_info::list.at(idx));
        }

        static constexpr auto one = ref_to_ptr(0_value);
        using t_one = typename decltype(one.self())::type;

      public:
        using meta_info = MetaInfo<TType>;
        static inline constexpr auto index = mpl::Value<TIndex>{};
        static inline constexpr auto list = meta_info::list;

        constexpr EnumUnion() noexcept = default;
        constexpr ~EnumUnion() noexcept = default;

        t_one m_one;
        None m_none{};

        constexpr auto get(mpl::MetaValue auto _index) & noexcept ->
            typename decltype(one.as_lvalue_reference())::type
            requires(_index < list.size())
        {
            return ptr_to_reference(m_one);
        }
        constexpr auto get(mpl::MetaValue auto _index) const& noexcept ->
            typename decltype(one.as_const().as_lvalue_reference())::type
            requires(_index < list.size())
        {
            return ptr_to_reference(m_one);
        }
        constexpr auto get(mpl::MetaValue auto _index) && noexcept ->
            typename decltype(one.as_rvalue_reference())::type
            requires(_index < list.size())
        {
            return ptr_to_reference(std::move(m_one));
        }
        constexpr auto get(mpl::MetaValue auto _index) const&& noexcept ->
            typename decltype(one.as_const().as_rvalue_reference())::type
            requires(_index < list.size())
        {
            return ptr_to_reference(std::move(m_one));
        }
    };

    template<usize TIndex, typename TType>
        requires(not mpl::decltype_<TType>().is_trivially_destructible())
                and (mpl::List<TType>{}.any_of([](mpl::MetaType auto type) {
                        return not type.template satisfies<std::is_empty>();
                    }))
    union EnumUnion<TIndex, TType> {
      private:
        static constexpr auto ref_to_ptr(mpl::MetaValue auto idx) {
            return reference_to_ptr(meta_info::list.at(idx));
        }

        static constexpr auto one = ref_to_ptr(0_value);
        using t_one = typename decltype(one.self())::type;

      public:
        using meta_info = MetaInfo<TType>;
        static inline constexpr auto index = mpl::Value<TIndex>{};
        static inline constexpr auto list = meta_info::list;

        constexpr EnumUnion() noexcept = default;
        constexpr ~EnumUnion() noexcept {
        }

        t_one m_one;
        None m_none{};

        constexpr auto
        get(mpl::MetaValue auto) & noexcept -> typename decltype(one.as_lvalue_reference())::type {
            return ptr_to_reference(m_one);
        }
        constexpr auto get(mpl::MetaValue auto) const& noexcept ->
            typename decltype(one.as_const().as_lvalue_reference())::type {
            return ptr_to_reference(m_one);
        }
        constexpr auto
        get(mpl::MetaValue auto) && noexcept -> typename decltype(one.as_rvalue_reference())::type {
            return ptr_to_reference(std::move(m_one));
        }
        constexpr auto get(mpl::MetaValue auto) const&& noexcept ->
            typename decltype(one.as_const().as_rvalue_reference())::type {
            return ptr_to_reference(std::move(m_one));
        }
    };

    template<usize TIndex, typename TType1, typename TType2>
        requires(mpl::List<TType1, TType2>{}.all_of(mpl::trivially_destructible))
                and (mpl::List<TType1, TType2>{}.any_of([](mpl::MetaType auto type) {
                        return not type.template satisfies<std::is_empty>();
                    }))
    union EnumUnion<TIndex, TType1, TType2> {
      public:
        using meta_info = MetaInfo<TType1, TType2>;

      private:
        static constexpr auto ref_to_ptr(mpl::MetaValue auto idx) {
            return reference_to_ptr(meta_info::list.at(idx));
        }

        using t_one = typename decltype(ref_to_ptr(0_value))::type;
        using t_two = typename decltype(ref_to_ptr(1_value))::type;

      public:
        static inline constexpr auto index = mpl::Value<TIndex>{};

        constexpr EnumUnion() noexcept = default;
        constexpr ~EnumUnion() noexcept = default;

        t_one m_one;
        t_two m_two;
        None m_none{};

        constexpr auto get(mpl::MetaValue auto val) & noexcept ->
            typename decltype(ref_to_ptr(val).as_lvalue_reference())::type {
            if constexpr(val == 0_value) {
                return ptr_to_reference(m_one);
            }
            else {
                return ptr_to_reference(m_two);
            }
        }

        constexpr auto get(mpl::MetaValue auto val) const& noexcept ->
            typename decltype(ref_to_ptr(val).as_const().as_lvalue_reference())::type {
            if constexpr(val == 0_value) {
                return ptr_to_reference(m_one);
            }
            else {
                return ptr_to_reference(m_two);
            }
        }

        constexpr auto get(mpl::MetaValue auto val) && noexcept ->
            typename decltype(ref_to_ptr(val).as_rvalue_reference())::type {
            if constexpr(val == 0_value) {
                return ptr_to_reference(std::move(m_one));
            }
            else {
                return ptr_to_reference(std::move(m_two));
            }
        }

        constexpr auto get(mpl::MetaValue auto val) const&& noexcept ->
            typename decltype(ref_to_ptr(val).as_const().as_rvalue_reference())::type {
            if constexpr(val == 0_value) {
                return ptr_to_reference(std::move(m_one));
            }
            else {
                return ptr_to_reference(std::move(m_two));
            }
        }
    };

    template<usize TIndex, typename TType1, typename TType2>
        requires(not mpl::List<TType1, TType2>{}.all_of(mpl::trivially_destructible))
                and (mpl::List<TType1, TType2>{}.any_of([](mpl::MetaType auto type) {
                        return not type.template satisfies<std::is_empty>();
                    }))
    union EnumUnion<TIndex, TType1, TType2> {
      public:
        using meta_info = MetaInfo<TType1, TType2>;

      private:
        static constexpr auto ref_to_ptr(mpl::MetaValue auto idx) {
            return reference_to_ptr(meta_info::list.at(idx));
        }

        using t_one = typename decltype(ref_to_ptr(0_value))::type;
        using t_two = typename decltype(ref_to_ptr(1_value))::type;

      public:
        static inline constexpr auto index = mpl::Value<TIndex>{};

        constexpr EnumUnion() noexcept = default;
        constexpr ~EnumUnion() noexcept {
        }

        t_one m_one;
        t_two m_two;
        None m_none{};

        constexpr auto get(mpl::MetaValue auto val) & noexcept ->
            typename decltype(ref_to_ptr(val).as_lvalue_reference())::type {
            if constexpr(val == 0_value) {
                return ptr_to_reference(m_one);
            }
            else {
                return ptr_to_reference(m_two);
            }
        }

        constexpr auto get(mpl::MetaValue auto val) const& noexcept ->
            typename decltype(ref_to_ptr(val).as_const().as_lvalue_reference())::type {
            if constexpr(val == 0_value) {
                return ptr_to_reference(m_one);
            }
            else {
                return ptr_to_reference(m_two);
            }
        }

        constexpr auto get(mpl::MetaValue auto val) && noexcept ->
            typename decltype(ref_to_ptr(val).as_rvalue_reference())::type {
            if constexpr(val == 0_value) {
                return ptr_to_reference(std::move(m_one));
            }
            else {
                return ptr_to_reference(std::move(m_two));
            }
        }

        constexpr auto get(mpl::MetaValue auto val) const&& noexcept ->
            typename decltype(ref_to_ptr(val).as_const().as_rvalue_reference())::type {
            if constexpr(val == 0_value) {
                return ptr_to_reference(std::move(m_one));
            }
            else {
                return ptr_to_reference(std::move(m_two));
            }
        }
    };

    template<usize TIndex, typename TType1, typename TType2, typename TType3>
        requires(mpl::List<TType1, TType2, TType3>{}.all_of(mpl::trivially_destructible))
                and (mpl::List<TType1, TType2, TType3>{}.any_of([](mpl::MetaType auto type) {
                        return not type.template satisfies<std::is_empty>();
                    }))
    union EnumUnion<TIndex, TType1, TType2, TType3> {
      public:
        using meta_info = MetaInfo<TType1, TType2, TType3>;

      private:
        static constexpr auto ref_to_ptr(mpl::MetaValue auto idx) {
            return reference_to_ptr(meta_info::list.at(idx));
        }

        using t_one = typename decltype(ref_to_ptr(0_value))::type;
        using t_two = typename decltype(ref_to_ptr(1_value))::type;
        using t_three = typename decltype(ref_to_ptr(2_value))::type;

      public:
        static inline constexpr auto index = mpl::Value<TIndex>{};

        constexpr EnumUnion() noexcept = default;
        constexpr ~EnumUnion() noexcept = default;

        t_one m_one;
        t_two m_two;
        t_three m_three;
        None m_none{};

        constexpr auto get(mpl::MetaValue auto val) & noexcept ->
            typename decltype(ref_to_ptr(val).as_lvalue_reference())::type {
            if constexpr(val == 0_value) {
                return ptr_to_reference(m_one);
            }
            else if constexpr(val == 1_value) {
                return ptr_to_reference(m_two);
            }
            else {
                return ptr_to_reference(m_three);
            }
        }

        constexpr auto get(mpl::MetaValue auto val) const& noexcept ->
            typename decltype(ref_to_ptr(val).as_const().as_lvalue_reference())::type {
            if constexpr(val == 0_value) {
                return ptr_to_reference(m_one);
            }
            else if constexpr(val == 1_value) {
                return ptr_to_reference(m_two);
            }
            else {
                return ptr_to_reference(m_three);
            }
        }

        constexpr auto get(mpl::MetaValue auto val) && noexcept ->
            typename decltype(ref_to_ptr(val).as_rvalue_reference())::type {
            if constexpr(val == 0_value) {
                return ptr_to_reference(std::move(m_one));
            }
            else if constexpr(val == 1_value) {
                return ptr_to_reference(std::move(m_two));
            }
            else {
                return ptr_to_reference(std::move(m_three));
            }
        }

        constexpr auto get(mpl::MetaValue auto val) const&& noexcept ->
            typename decltype(ref_to_ptr(val).as_const().as_rvalue_reference())::type {
            if constexpr(val == 0_value) {
                return ptr_to_reference(std::move(m_one));
            }
            else if constexpr(val == 1_value) {
                return ptr_to_reference(std::move(m_two));
            }
            else {
                return ptr_to_reference(std::move(m_three));
            }
        }
    };

    template<usize TIndex, typename TType1, typename TType2, typename TType3>
        requires(not mpl::List<TType1, TType2, TType3>{}.all_of(mpl::trivially_destructible))
                and (mpl::List<TType1, TType2, TType3>{}.any_of([](mpl::MetaType auto type) {
                        return not type.template satisfies<std::is_empty>();
                    }))
    union EnumUnion<TIndex, TType1, TType2, TType3> {
      public:
        using meta_info = MetaInfo<TType1, TType2, TType3>;

      private:
        static constexpr auto ref_to_ptr(mpl::MetaValue auto idx) {
            return reference_to_ptr(meta_info::list.at(idx));
        }

        using t_one = typename decltype(ref_to_ptr(0_value))::type;
        using t_two = typename decltype(ref_to_ptr(1_value))::type;
        using t_three = typename decltype(ref_to_ptr(2_value))::type;

      public:
        static inline constexpr auto index = mpl::Value<TIndex>{};

        constexpr EnumUnion() noexcept = default;
        constexpr ~EnumUnion() noexcept {
        }

        t_one m_one;
        t_two m_two;
        t_three m_three;
        None m_none{};

        constexpr auto get(mpl::MetaValue auto val) & noexcept ->
            typename decltype(ref_to_ptr(val).as_lvalue_reference())::type {
            if constexpr(val == 0_value) {
                return ptr_to_reference(m_one);
            }
            else if constexpr(val == 1_value) {
                return ptr_to_reference(m_two);
            }
            else {
                return ptr_to_reference(m_three);
            }
        }

        constexpr auto get(mpl::MetaValue auto val) const& noexcept ->
            typename decltype(ref_to_ptr(val).as_const().as_lvalue_reference())::type {
            if constexpr(val == 0_value) {
                return ptr_to_reference(m_one);
            }
            else if constexpr(val == 1_value) {
                return ptr_to_reference(m_two);
            }
            else {
                return ptr_to_reference(m_three);
            }
        }

        constexpr auto get(mpl::MetaValue auto val) && noexcept ->
            typename decltype(ref_to_ptr(val).as_rvalue_reference())::type {
            if constexpr(val == 0_value) {
                return ptr_to_reference(std::move(m_one));
            }
            else if constexpr(val == 1_value) {
                return ptr_to_reference(std::move(m_two));
            }
            else {
                return ptr_to_reference(std::move(m_three));
            }
        }

        constexpr auto get(mpl::MetaValue auto val) const&& noexcept ->
            typename decltype(ref_to_ptr(val).as_const().as_rvalue_reference())::type {
            if constexpr(val == 0_value) {
                return ptr_to_reference(std::move(m_one));
            }
            else if constexpr(val == 1_value) {
                return ptr_to_reference(std::move(m_two));
            }
            else {
                return ptr_to_reference(std::move(m_three));
            }
        }
    };

    template<usize TIndex, typename TType1, typename TType2, typename TType3, typename TType4>
        requires(mpl::List<TType1, TType2, TType3, TType4>{}.all_of(mpl::trivially_destructible))
                and (mpl::List<TType1, TType2, TType3, TType4>{}.any_of(
                    [](mpl::MetaType auto type) {
                        return not type.template satisfies<std::is_empty>();
                    }))
    union EnumUnion<TIndex, TType1, TType2, TType3, TType4> {
      public:
        using meta_info = MetaInfo<TType1, TType2, TType3, TType4>;

      private:
        static constexpr auto ref_to_ptr(mpl::MetaValue auto idx) {
            return reference_to_ptr(meta_info::list.at(idx));
        }

        using t_one = typename decltype(ref_to_ptr(0_value))::type;
        using t_two = typename decltype(ref_to_ptr(1_value))::type;
        using t_three = typename decltype(ref_to_ptr(2_value))::type;
        using t_four = typename decltype(ref_to_ptr(3_value))::type;

      public:
        static inline constexpr auto index = mpl::Value<TIndex>{};

        constexpr EnumUnion() noexcept = default;
        constexpr ~EnumUnion() noexcept = default;

        t_one m_one;
        t_two m_two;
        t_three m_three;
        t_four m_four;
        None m_none{};

        constexpr auto get(mpl::MetaValue auto val) & noexcept ->
            typename decltype(ref_to_ptr(val).as_lvalue_reference())::type {
            if constexpr(val == 0_value) {
                return ptr_to_reference(m_one);
            }
            else if constexpr(val == 1_value) {
                return ptr_to_reference(m_two);
            }
            else if constexpr(val == 2_value) {
                return ptr_to_reference(m_three);
            }
            else {
                return ptr_to_reference(m_four);
            }
        }

        constexpr auto get(mpl::MetaValue auto val) const& noexcept ->
            typename decltype(ref_to_ptr(val).as_const().as_lvalue_reference())::type {
            if constexpr(val == 0_value) {
                return ptr_to_reference(m_one);
            }
            else if constexpr(val == 1_value) {
                return ptr_to_reference(m_two);
            }
            else if constexpr(val == 2_value) {
                return ptr_to_reference(m_three);
            }
            else {
                return ptr_to_reference(m_four);
            }
        }

        constexpr auto get(mpl::MetaValue auto val) && noexcept ->
            typename decltype(ref_to_ptr(val).as_rvalue_reference())::type {
            if constexpr(val == 0_value) {
                return ptr_to_reference(std::move(m_one));
            }
            else if constexpr(val == 1_value) {
                return ptr_to_reference(std::move(m_two));
            }
            else if constexpr(val == 2_value) {
                return ptr_to_reference(std::move(m_three));
            }
            else {
                return ptr_to_reference(std::move(m_four));
            }
        }

        constexpr auto get(mpl::MetaValue auto val) const&& noexcept ->
            typename decltype(ref_to_ptr(val).as_const().as_rvalue_reference())::type {
            if constexpr(val == 0_value) {
                return ptr_to_reference(std::move(m_one));
            }
            else if constexpr(val == 1_value) {
                return ptr_to_reference(std::move(m_two));
            }
            else if constexpr(val == 2_value) {
                return ptr_to_reference(std::move(m_three));
            }
            else {
                return ptr_to_reference(std::move(m_four));
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
    union EnumUnion<TIndex, TType1, TType2, TType3, TType4> {
      public:
        using meta_info = MetaInfo<TType1, TType2, TType3, TType4>;

      private:
        static constexpr auto ref_to_ptr(mpl::MetaValue auto idx) {
            return reference_to_ptr(meta_info::list.at(idx));
        }

        using t_one = typename decltype(ref_to_ptr(0_value))::type;
        using t_two = typename decltype(ref_to_ptr(1_value))::type;
        using t_three = typename decltype(ref_to_ptr(2_value))::type;
        using t_four = typename decltype(ref_to_ptr(3_value))::type;

      public:
        static inline constexpr auto index = mpl::Value<TIndex>{};

        constexpr EnumUnion() noexcept = default;
        constexpr ~EnumUnion() noexcept {
        }

        t_one m_one;
        t_two m_two;
        t_three m_three;
        t_four m_four;
        None m_none{};

        constexpr auto get(mpl::MetaValue auto val) & noexcept ->
            typename decltype(ref_to_ptr(val).as_lvalue_reference())::type {
            if constexpr(val == 0_value) {
                return ptr_to_reference(m_one);
            }
            else if constexpr(val == 1_value) {
                return ptr_to_reference(m_two);
            }
            else if constexpr(val == 2_value) {
                return ptr_to_reference(m_three);
            }
            else {
                return ptr_to_reference(m_four);
            }
        }

        constexpr auto get(mpl::MetaValue auto val) const& noexcept ->
            typename decltype(ref_to_ptr(val).as_const().as_lvalue_reference())::type {
            if constexpr(val == 0_value) {
                return ptr_to_reference(m_one);
            }
            else if constexpr(val == 1_value) {
                return ptr_to_reference(m_two);
            }
            else if constexpr(val == 2_value) {
                return ptr_to_reference(m_three);
            }
            else {
                return ptr_to_reference(m_four);
            }
        }

        constexpr auto get(mpl::MetaValue auto val) && noexcept ->
            typename decltype(ref_to_ptr(val).as_rvalue_reference())::type {
            if constexpr(val == 0_value) {
                return ptr_to_reference(std::move(m_one));
            }
            else if constexpr(val == 1_value) {
                return ptr_to_reference(std::move(m_two));
            }
            else if constexpr(val == 2_value) {
                return ptr_to_reference(std::move(m_three));
            }
            else {
                return ptr_to_reference(std::move(m_four));
            }
        }

        constexpr auto get(mpl::MetaValue auto val) const&& noexcept ->
            typename decltype(ref_to_ptr(val).as_const().as_rvalue_reference())::type {
            if constexpr(val == 0_value) {
                return ptr_to_reference(std::move(m_one));
            }
            else if constexpr(val == 1_value) {
                return ptr_to_reference(std::move(m_two));
            }
            else if constexpr(val == 2_value) {
                return ptr_to_reference(std::move(m_three));
            }
            else {
                return ptr_to_reference(std::move(m_four));
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
    union EnumUnion<TIndex, TType1, TType2, TType3, TType4, TType5> {
      public:
        using meta_info = MetaInfo<TType1, TType2, TType3, TType4, TType5>;

      private:
        static constexpr auto ref_to_ptr(mpl::MetaValue auto idx) {
            return reference_to_ptr(meta_info::list.at(idx));
        }

        using t_one = typename decltype(ref_to_ptr(0_value))::type;
        using t_two = typename decltype(ref_to_ptr(1_value))::type;
        using t_three = typename decltype(ref_to_ptr(2_value))::type;
        using t_four = typename decltype(ref_to_ptr(3_value))::type;
        using t_five = typename decltype(ref_to_ptr(4_value))::type;

      public:
        static inline constexpr auto index = mpl::Value<TIndex>{};

        constexpr EnumUnion() noexcept = default;
        constexpr ~EnumUnion() noexcept = default;

        t_one m_one;
        t_two m_two;
        t_three m_three;
        t_four m_four;
        t_five m_five;
        None m_none{};

        constexpr auto get(mpl::MetaValue auto val) & noexcept ->
            typename decltype(ref_to_ptr(val).as_lvalue_reference())::type {
            if constexpr(val == 0_value) {
                return ptr_to_reference(m_one);
            }
            else if constexpr(val == 1_value) {
                return ptr_to_reference(m_two);
            }
            else if constexpr(val == 2_value) {
                return ptr_to_reference(m_three);
            }
            else if constexpr(val == 3_value) {
                return ptr_to_reference(m_four);
            }
            else {
                return ptr_to_reference(m_five);
            }
        }

        constexpr auto get(mpl::MetaValue auto val) const& noexcept ->
            typename decltype(ref_to_ptr(val).as_const().as_lvalue_reference())::type {
            if constexpr(val == 0_value) {
                return ptr_to_reference(m_one);
            }
            else if constexpr(val == 1_value) {
                return ptr_to_reference(m_two);
            }
            else if constexpr(val == 2_value) {
                return ptr_to_reference(m_three);
            }
            else if constexpr(val == 3_value) {
                return ptr_to_reference(m_four);
            }
            else {
                return ptr_to_reference(m_five);
            }
        }

        constexpr auto get(mpl::MetaValue auto val) && noexcept ->
            typename decltype(ref_to_ptr(val).as_rvalue_reference())::type {
            if constexpr(val == 0_value) {
                return ptr_to_reference(std::move(m_one));
            }
            else if constexpr(val == 1_value) {
                return ptr_to_reference(std::move(m_two));
            }
            else if constexpr(val == 2_value) {
                return ptr_to_reference(std::move(m_three));
            }
            else if constexpr(val == 3_value) {
                return ptr_to_reference(std::move(m_four));
            }
            else {
                return ptr_to_reference(std::move(m_five));
            }
        }

        constexpr auto get(mpl::MetaValue auto val) const&& noexcept ->
            typename decltype(ref_to_ptr(val).as_const().as_rvalue_reference())::type {
            if constexpr(val == 0_value) {
                return ptr_to_reference(std::move(m_one));
            }
            else if constexpr(val == 1_value) {
                return ptr_to_reference(std::move(m_two));
            }
            else if constexpr(val == 2_value) {
                return ptr_to_reference(std::move(m_three));
            }
            else if constexpr(val == 3_value) {
                return ptr_to_reference(std::move(m_four));
            }
            else {
                return ptr_to_reference(std::move(m_five));
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
    union EnumUnion<TIndex, TType1, TType2, TType3, TType4, TType5> {
      public:
        using meta_info = MetaInfo<TType1, TType2, TType3, TType4, TType5>;

      private:
        static constexpr auto ref_to_ptr(mpl::MetaValue auto idx) {
            return reference_to_ptr(meta_info::list.at(idx));
        }

        using t_one = typename decltype(ref_to_ptr(0_value))::type;
        using t_two = typename decltype(ref_to_ptr(1_value))::type;
        using t_three = typename decltype(ref_to_ptr(2_value))::type;
        using t_four = typename decltype(ref_to_ptr(3_value))::type;
        using t_five = typename decltype(ref_to_ptr(4_value))::type;

      public:
        static inline constexpr auto index = mpl::Value<TIndex>{};

        constexpr EnumUnion() noexcept = default;
        constexpr ~EnumUnion() noexcept {
        }

        t_one m_one;
        t_two m_two;
        t_three m_three;
        t_four m_four;
        t_five m_five;
        None m_none{};

        constexpr auto get(mpl::MetaValue auto val) & noexcept ->
            typename decltype(ref_to_ptr(val).as_lvalue_reference())::type {
            if constexpr(val == 0_value) {
                return ptr_to_reference(m_one);
            }
            else if constexpr(val == 1_value) {
                return ptr_to_reference(m_two);
            }
            else if constexpr(val == 2_value) {
                return ptr_to_reference(m_three);
            }
            else if constexpr(val == 3_value) {
                return ptr_to_reference(m_four);
            }
            else {
                return ptr_to_reference(m_five);
            }
        }

        constexpr auto get(mpl::MetaValue auto val) const& noexcept ->
            typename decltype(ref_to_ptr(val).as_const().as_lvalue_reference())::type {
            if constexpr(val == 0_value) {
                return ptr_to_reference(m_one);
            }
            else if constexpr(val == 1_value) {
                return ptr_to_reference(m_two);
            }
            else if constexpr(val == 2_value) {
                return ptr_to_reference(m_three);
            }
            else if constexpr(val == 3_value) {
                return ptr_to_reference(m_four);
            }
            else {
                return ptr_to_reference(m_five);
            }
        }

        constexpr auto get(mpl::MetaValue auto val) && noexcept ->
            typename decltype(ref_to_ptr(val).as_rvalue_reference())::type {
            if constexpr(val == 0_value) {
                return ptr_to_reference(std::move(m_one));
            }
            else if constexpr(val == 1_value) {
                return ptr_to_reference(std::move(m_two));
            }
            else if constexpr(val == 2_value) {
                return ptr_to_reference(std::move(m_three));
            }
            else if constexpr(val == 3_value) {
                return ptr_to_reference(std::move(m_four));
            }
            else {
                return ptr_to_reference(std::move(m_five));
            }
        }

        constexpr auto get(mpl::MetaValue auto val) const&& noexcept ->
            typename decltype(ref_to_ptr(val).as_const().as_rvalue_reference())::type {
            if constexpr(val == 0_value) {
                return ptr_to_reference(std::move(m_one));
            }
            else if constexpr(val == 1_value) {
                return ptr_to_reference(std::move(m_two));
            }
            else if constexpr(val == 2_value) {
                return ptr_to_reference(std::move(m_three));
            }
            else if constexpr(val == 3_value) {
                return ptr_to_reference(std::move(m_four));
            }
            else {
                return ptr_to_reference(std::move(m_five));
            }
        }
    };

} // namespace hyperion::enum_::detail

#endif // HYPERION_STD_ENUM_STORAGE_H
