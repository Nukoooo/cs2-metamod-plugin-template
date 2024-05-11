#pragma once
#include <emmintrin.h>
#include <immintrin.h>
#include <type_traits>

namespace simd
{
#if defined(GCC)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wignored-attributes"
#elif defined(CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wignored-attributes"
#endif
    namespace impl
    {
        template <typename T>
        struct simd_wrapper;

        template <>
        struct simd_wrapper<__m128i>
        {
            using type = __m128i;
        };

        template <>
        struct simd_wrapper<__m256i>
        {
            using type = __m256i;
        };

        template <typename T>
        class simd
        {
          public:
            using simd_type = typename T::type;

            static constexpr int simd_length = std::is_same_v<simd_type, __m128i> ? 16 : 32;

            template <typename U>
            static simd_type* cast(const U* data)
            {
                return const_cast<simd_type*>(reinterpret_cast<const simd_type*>(data));
            }

            template <typename U>
            static simd_type load_unaligned(const U* data)
            {
                if constexpr (std::is_same_v<simd_type, __m128i>)
                {
                    return _mm_loadu_si128(reinterpret_cast<const __m128i*>(data));
                }
                else if constexpr (std::is_same_v<simd_type, __m256i>)
                {
                    return _mm256_loadu_si256(reinterpret_cast<const __m256i*>(data));
                }
            }

            template <typename U>
            static simd_type load_aligned(const U* data)
            {
                if constexpr (std::is_same_v<simd_type, __m128i>)
                {
                    return _mm_load_si128(reinterpret_cast<const __m128i*>(data));
                }
                else if constexpr (std::is_same_v<simd_type, __m256i>)
                {
                    return _mm256_load_si256(reinterpret_cast<const __m256i*>(data));
                }
            }

            static simd_type cmpeq_epi8(simd_type a, simd_type b)
            {
                if constexpr (std::is_same_v<simd_type, __m128i>)
                {
                    return _mm_cmpeq_epi8(a, b);
                }
                else if constexpr (std::is_same_v<simd_type, __m256i>)
                {
                    return _mm256_cmpeq_epi8(a, b);
                }
            }

            static int movemask_epi8(simd_type a)
            {
                if constexpr (std::is_same_v<simd_type, __m128i>)
                {
                    return _mm_movemask_epi8(a);
                }
                else if constexpr (std::is_same_v<simd_type, __m256i>)
                {
                    return _mm256_movemask_epi8(a);
                }
            }

            static simd_type set1_epi8(uint8_t value)
            {
                if constexpr (std::is_same_v<simd_type, __m128i>)
                {
                    return _mm_set1_epi8(static_cast<int8_t>(value));
                }
                else if constexpr (std::is_same_v<simd_type, __m256i>)
                {
                    return _mm256_set1_epi8(static_cast<int8_t>(value));
                }
            }

            static simd_type and_si(simd_type a, simd_type b)
            {
                if constexpr (std::is_same_v<simd_type, __m128i>)
                {
                    return _mm_and_si128(a, b);
                }
                else if constexpr (std::is_same_v<simd_type, __m256i>)
                {
                    return _mm256_and_si256(a, b);
                }
            }

            static int test(simd_type a, simd_type b)
            {
                if constexpr (std::is_same_v<simd_type, __m128i>)
                {
                    return _mm_testc_si128(a, b);
                }
                else if constexpr (std::is_same_v<simd_type, __m256i>)
                {
                    return _mm256_testc_si256(a, b);
                }
            }
        };
    }

    using iSSE  = impl::simd<impl::simd_wrapper<__m128i>>;
    using iAVX2 = impl::simd<impl::simd_wrapper<__m256i>>;
#if defined(GCC)
    #pragma GCC diagnostic pop
#elif defined(CLANG)
    #pragma clang diagnostic pop
#endif
}
