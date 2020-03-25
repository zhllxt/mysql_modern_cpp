/*
 * COPYRIGHT (C) 2019-2020, zhllxt
 *
 * author   : zhllxt
 * email    : 37792738@qq.com
 *
 * Distributed under the GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
 * (See accompanying file LICENSE or see <http://www.gnu.org/licenses/>)
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <cassert>

#include <string>
#include <vector>
#include <memory>
#include <tuple>
#include <functional>
#include <type_traits>
#include <algorithm>
#include <utility>

#define FMT_HEADER_ONLY
#include <fmt/format.h>
#include <fmt/chrono.h>

#include <mysql.h>

#if !defined(NDEBUG) && !defined(_DEBUG) && !defined(DEBUG)
#define NDEBUG
#endif

// init
// prepare
// --bind_param
// execute
// attr_set
// store_result
// bindResult
// fetch
// getstring -> buffer length

namespace mysql
{

#define bit_uint1korr(A)	(*(((uint8_t*)(A))))

#define bit_uint2korr(A) ((uint16_t) (((uint16_t) (((unsigned char*) (A))[1])) +\
                                   ((uint16_t) (((unsigned char*) (A))[0]) << 8)))
#define bit_uint3korr(A) ((uint32_t) (((uint32_t) (((unsigned char*) (A))[2])) +\
                                   (((uint32_t) (((unsigned char*) (A))[1])) << 8) +\
                                   (((uint32_t) (((unsigned char*) (A))[0])) << 16)))
#define bit_uint4korr(A) ((uint32_t) (((uint32_t) (((unsigned char*) (A))[3])) +\
                                   (((uint32_t) (((unsigned char*) (A))[2])) << 8) +\
                                   (((uint32_t) (((unsigned char*) (A))[1])) << 16) +\
                                   (((uint32_t) (((unsigned char*) (A))[0])) << 24)))
#define bit_uint5korr(A) ((uint64_t)(((uint32_t) (((unsigned char*) (A))[4])) +\
                                    (((uint32_t) (((unsigned char*) (A))[3])) << 8) +\
                                    (((uint32_t) (((unsigned char*) (A))[2])) << 16) +\
                                   (((uint32_t) (((unsigned char*) (A))[1])) << 24)) +\
                                    (((uint64_t) (((unsigned char*) (A))[0])) << 32))
#define bit_uint6korr(A) ((uint64_t)(((uint32_t) (((unsigned char*) (A))[5])) +\
                                    (((uint32_t) (((unsigned char*) (A))[4])) << 8) +\
                                    (((uint32_t) (((unsigned char*) (A))[3])) << 16) +\
                                    (((uint32_t) (((unsigned char*) (A))[2])) << 24)) +\
                        (((uint64_t) (((uint32_t) (((unsigned char*) (A))[1])) +\
                                    (((uint32_t) (((unsigned char*) (A))[0]) << 8)))) <<\
                                     32))
#define bit_uint7korr(A) ((uint64_t)(((uint32_t) (((unsigned char*) (A))[6])) +\
                                    (((uint32_t) (((unsigned char*) (A))[5])) << 8) +\
                                    (((uint32_t) (((unsigned char*) (A))[4])) << 16) +\
                                   (((uint32_t) (((unsigned char*) (A))[3])) << 24)) +\
                        (((uint64_t) (((uint32_t) (((unsigned char*) (A))[2])) +\
                                    (((uint32_t) (((unsigned char*) (A))[1])) << 8) +\
                                    (((uint32_t) (((unsigned char*) (A))[0])) << 16))) <<\
                                     32))
#define bit_uint8korr(A) ((uint64_t)(((uint32_t) (((unsigned char*) (A))[7])) +\
                                    (((uint32_t) (((unsigned char*) (A))[6])) << 8) +\
                                    (((uint32_t) (((unsigned char*) (A))[5])) << 16) +\
                                    (((uint32_t) (((unsigned char*) (A))[4])) << 24)) +\
                        (((uint64_t) (((uint32_t) (((unsigned char*) (A))[3])) +\
                                    (((uint32_t) (((unsigned char*) (A))[2])) << 8) +\
                                    (((uint32_t) (((unsigned char*) (A))[1])) << 16) +\
                                    (((uint32_t) (((unsigned char*) (A))[0])) << 24))) <<\
                                    32))

	namespace detail
	{
		/*
		 * 1. function type								==>	Ret(Args...)
		 * 2. function pointer							==>	Ret(*)(Args...)
		 * 3. function reference						==>	Ret(&)(Args...)
		 * 4. pointer to non-static member function		==> Ret(T::*)(Args...)
		 * 5. function object and functor				==> &T::operator()
		 * 6. function with generic operator call		==> template <typeanme ... Args> &T::operator()
		 */
		template<typename, typename = void>
		struct function_traits { static constexpr bool is_callable = false; };

		template<typename Ret, typename... Args>
		struct function_traits<Ret(Args...)>
		{
		public:
			static constexpr std::size_t argc = sizeof...(Args);
			static constexpr bool is_callable = true;

			typedef Ret function_type(Args...);
			typedef Ret return_type;
			using stl_function_type = std::function<function_type>;
			typedef Ret(*pointer)(Args...);

			template<std::size_t I>
			struct args
			{
				static_assert(I < argc, "index is out of range, index must less than sizeof Args");
				using type = typename std::tuple_element<I, std::tuple<Args...>>::type;
			};

			typedef std::tuple<Args...> tuple_type;
			typedef std::tuple<std::remove_cv_t<std::remove_reference_t<Args>>...> pod_tuple_type;
		};

		// regular function pointer
		template<typename Ret, typename... Args>
		struct function_traits<Ret(*)(Args...)> : function_traits<Ret(Args...)> {};

		// non-static member function pointer
		template <typename Ret, typename Class, typename... Args>
		struct function_traits<Ret(Class::*)(Args...)> : function_traits<Ret(Args...)> {};

		template <typename Ret, typename Class, typename... Args>
		struct function_traits<Ret(Class::*)(Args...) const> : function_traits<Ret(Args...)> {};

		template <typename Ret, typename Class, typename... Args>
		struct function_traits<Ret(Class::*)(Args...) volatile> : function_traits<Ret(Args...)> {};

		template <typename Ret, typename Class, typename... Args>
		struct function_traits<Ret(Class::*)(Args...) const volatile> : function_traits<Ret(Args...)> {};

		template <typename Ret, typename Class, typename... Args>
		struct function_traits<Ret(Class::*)(Args...) &> : function_traits<Ret(Args...)> {};

		template <typename Ret, typename Class, typename... Args>
		struct function_traits<Ret(Class::*)(Args...) const &> : function_traits<Ret(Args...)> {};

		template <typename Ret, typename Class, typename... Args>
		struct function_traits<Ret(Class::*)(Args...) volatile &> : function_traits<Ret(Args...)> {};

		template <typename Ret, typename Class, typename... Args>
		struct function_traits<Ret(Class::*)(Args...) const volatile &> : function_traits<Ret(Args...)> {};

		template <typename Ret, typename Class, typename... Args>
		struct function_traits<Ret(Class::*)(Args...) && > : function_traits<Ret(Args...)> {};

		template <typename Ret, typename Class, typename... Args>
		struct function_traits<Ret(Class::*)(Args...) const &&> : function_traits<Ret(Args...)> {};

		template <typename Ret, typename Class, typename... Args>
		struct function_traits<Ret(Class::*)(Args...) volatile &&> : function_traits<Ret(Args...)> {};

		template <typename Ret, typename Class, typename... Args>
		struct function_traits<Ret(Class::*)(Args...) const volatile &&> : function_traits<Ret(Args...)> {};

		// non-static member function pointer -- noexcept versions for (C++17 and later)
		template <typename Ret, typename Class, typename... Args>
		struct function_traits<Ret(Class::*)(Args...) noexcept> : function_traits<Ret(Args...)> {};

		template <typename Ret, typename Class, typename... Args>
		struct function_traits<Ret(Class::*)(Args...) const noexcept> : function_traits<Ret(Args...)> {};

		template <typename Ret, typename Class, typename... Args>
		struct function_traits<Ret(Class::*)(Args...) volatile noexcept> : function_traits<Ret(Args...)> {};

		template <typename Ret, typename Class, typename... Args>
		struct function_traits<Ret(Class::*)(Args...) const volatile noexcept> : function_traits<Ret(Args...)> {};

		template <typename Ret, typename Class, typename... Args>
		struct function_traits<Ret(Class::*)(Args...) & noexcept> : function_traits<Ret(Args...)> {};

		template <typename Ret, typename Class, typename... Args>
		struct function_traits<Ret(Class::*)(Args...) const & noexcept> : function_traits<Ret(Args...)> {};

		template <typename Ret, typename Class, typename... Args>
		struct function_traits<Ret(Class::*)(Args...) volatile & noexcept> : function_traits<Ret(Args...)> {};

		template <typename Ret, typename Class, typename... Args>
		struct function_traits<Ret(Class::*)(Args...) const volatile & noexcept> : function_traits<Ret(Args...)> {};

		template <typename Ret, typename Class, typename... Args>
		struct function_traits<Ret(Class::*)(Args...) && noexcept> : function_traits<Ret(Args...)> {};

		template <typename Ret, typename Class, typename... Args>
		struct function_traits<Ret(Class::*)(Args...) const && noexcept> : function_traits<Ret(Args...)> {};

		template <typename Ret, typename Class, typename... Args>
		struct function_traits<Ret(Class::*)(Args...) volatile && noexcept> : function_traits<Ret(Args...)> {};

		template <typename Ret, typename Class, typename... Args>
		struct function_traits<Ret(Class::*)(Args...) const volatile && noexcept> : function_traits<Ret(Args...)> {};

		// functor lambda
		template<typename Callable>
		struct function_traits<Callable, std::void_t<decltype(&Callable::operator()), char>> : function_traits<decltype(&Callable::operator())> {};

		// std::function
		template <typename Ret, typename... Args>
		struct function_traits<std::function<Ret(Args...)>> : function_traits<Ret(Args...)> {};


		template <typename F>
		typename function_traits<F>::stl_function_type to_function(F&& lambda)
		{
			return static_cast<typename function_traits<F>::stl_function_type>(std::forward<F>(lambda));
		}

		template <typename F>
		typename function_traits<F>::pointer to_function_pointer(const F& lambda)
		{
			return static_cast<typename function_traits<F>::pointer>(lambda);
		}


		template< class T >
		inline constexpr bool is_callable_v = function_traits<std::decay_t<T>>::is_callable;
	}
	
	struct binder;

	namespace detail
	{
		template<typename = void>
		inline std::tuple<std::unique_ptr<char>, std::size_t> allocate_buffer_for_field(const MYSQL_FIELD * const field)
		{
			using type = std::tuple<std::unique_ptr<char>, std::size_t>;
			switch (field->type)
			{
			case MYSQL_TYPE_NULL:
				return type{ nullptr, 0 };
			case MYSQL_TYPE_TINY:
				return type{ new char[1], 1 };
			case MYSQL_TYPE_SHORT:
				return type{ new char[2], 2 };
			case MYSQL_TYPE_INT24:
			case MYSQL_TYPE_LONG:
			case MYSQL_TYPE_FLOAT:
				return type{ new char[4], 4 };
			case MYSQL_TYPE_DOUBLE:
			case MYSQL_TYPE_LONGLONG:
				return type{ new char[8], 8 };
			case MYSQL_TYPE_YEAR:
				return type{ new char[2], 2 };
			case MYSQL_TYPE_TIMESTAMP:
			case MYSQL_TYPE_DATE:
			case MYSQL_TYPE_TIME:
			case MYSQL_TYPE_DATETIME:
				return type{ new char[sizeof(MYSQL_TIME)], sizeof(MYSQL_TIME) };
			case MYSQL_TYPE_TINY_BLOB:
			case MYSQL_TYPE_MEDIUM_BLOB:
			case MYSQL_TYPE_LONG_BLOB:
			case MYSQL_TYPE_BLOB:
			case MYSQL_TYPE_STRING:
			case MYSQL_TYPE_VAR_STRING:
			case MYSQL_TYPE_VARCHAR:
#if LIBMYSQL_VERSION_ID > 50700
			case MYSQL_TYPE_JSON:
#endif //LIBMYSQL_VERSION_ID > 50700
				return type{ new char[field->max_length + 1], field->max_length + 1 };
			case MYSQL_TYPE_DECIMAL:
			case MYSQL_TYPE_NEWDECIMAL:
				return type{ new char[64], 64 };
			case MYSQL_TYPE_BIT:
				return type{ new char[8], 8 };
				// There two are not sent over the wire
			case MYSQL_TYPE_ENUM:
			case MYSQL_TYPE_SET:
			case MYSQL_TYPE_GEOMETRY:
			case MYSQL_TYPE_NEWDATE:
			case MYSQL_TYPE_TIMESTAMP2:
			case MYSQL_TYPE_DATETIME2:
			case MYSQL_TYPE_TIME2:
#if LIBMYSQL_VERSION_ID >= 80019
			case MYSQL_TYPE_TYPED_ARRAY:
#endif //LIBMYSQL_VERSION_ID >= 80019
			default:
				// TODO: Andrey, there can be crashes when we go through this. Please fix.
				throw std::runtime_error("allocate_buffer_for_field: invalid rbind data type");
			}
		}

		template<class Args> inline std::tm mysql_time_to_tm(Args&& args)
		{
			MYSQL_TIME* mysql_time = nullptr;

			if constexpr (std::is_pointer_v<std::remove_cv_t<std::remove_reference_t<Args>>>)
				mysql_time = args;
			else if constexpr (std::is_reference_v<std::remove_cv_t<Args>>)
				mysql_time = &args;
			else
				std::ignore = true;

			std::tm tm = { 0 };
			if (mysql_time)
			{
				tm.tm_sec = mysql_time->second;   // seconds after the minute - [0, 60] including leap second
				tm.tm_min = mysql_time->minute;   // minutes after the hour - [0, 59]
				tm.tm_hour = mysql_time->hour;  // hours since midnight - [0, 23]
				tm.tm_mday = mysql_time->day;  // day of the month - [1, 31]
				tm.tm_mon = mysql_time->month - 1;   // months since January - [0, 11]
				tm.tm_year = mysql_time->year - 1900;  // years since 1900
				tm.tm_wday = 0;  // days since Sunday - [0, 6]
				tm.tm_yday = 0;  // days since January 1 - [0, 365]
				tm.tm_isdst = 0; // daylight savings time flag
			}

			return tm;
		}

		template<typename = void> long               str2l  (const char *str, char **str_end = nullptr, int base = 10) { return strtol  (str, str_end, base); }
		template<typename = void> long long          str2ll (const char *str, char **str_end = nullptr, int base = 10) { return strtoll (str, str_end, base); }
		template<typename = void> unsigned long      str2ul (const char *str, char **str_end = nullptr, int base = 10) { return strtoul (str, str_end, base); }
		template<typename = void> unsigned long long str2ull(const char *str, char **str_end = nullptr, int base = 10) { return strtoull(str, str_end, base); }
		template<typename = void> float              str2f  (const char* str, char** str_end = nullptr)                { return strtof  (str, str_end      ); }
		template<typename = void> double             str2d  (const char* str, char** str_end = nullptr)                { return strtod  (str, str_end      ); }
		template<typename = void> long double        str2ld (const char* str, char** str_end = nullptr)                { return strtold (str, str_end      ); }


		template<class T>
		struct convert;

		template<>
		struct convert<bool>
		{
			template<class ...Args> inline static bool stov(Args&&... args) { return (str2l(std::forward<Args>(args)...) != 0); }
			template<class Args> inline static bool ntov(Args&& args) { return (args != std::decay_t<Args>(0)); }
			template<class Args> inline static bool ttov(Args&& args) { std::tm tm = mysql_time_to_tm(std::forward<Args>(args)); return static_cast<bool>(std::mktime(&tm)); }
		};

		template<>
		struct convert<char>
		{
			template<class ...Args> inline static char stov(Args&&... args) { return static_cast<char>(str2l(std::forward<Args>(args)...)); }
			template<class Args> inline static char ntov(Args&& args) { return static_cast<char>(std::forward<Args>(args)); }
			template<class Args> inline static char ttov(Args&& args) { std::tm tm = mysql_time_to_tm(std::forward<Args>(args)); return static_cast<char>(std::mktime(&tm)); }
		};

		template<>
		struct convert<unsigned char>
		{
			template<class ...Args> inline static unsigned char stov(Args&&... args) { return static_cast<unsigned char>(str2ul(std::forward<Args>(args)...)); }
			template<class Args> inline static unsigned char ntov(Args&& args) { return static_cast<unsigned char>(std::forward<Args>(args)); }
			template<class Args> inline static unsigned char ttov(Args&& args) { std::tm tm = mysql_time_to_tm(std::forward<Args>(args)); return static_cast<unsigned char>(std::mktime(&tm)); }
		};

		template<>
		struct convert<short>
		{
			template<class ...Args> inline static short stov(Args&&... args) { return static_cast<short>(str2l(std::forward<Args>(args)...)); }
			template<class Args> inline static short ntov(Args&& args) { return static_cast<short>(std::forward<Args>(args)); }
			template<class Args> inline static short ttov(Args&& args) { std::tm tm = mysql_time_to_tm(std::forward<Args>(args)); return static_cast<short>(std::mktime(&tm)); }
		};

		template<>
		struct convert<unsigned short>
		{
			template<class ...Args> inline static unsigned short stov(Args&&... args) { return static_cast<unsigned short>(str2ul(std::forward<Args>(args)...)); }
			template<class Args> inline static unsigned short ntov(Args&& args) { return static_cast<unsigned short>(std::forward<Args>(args)); }
			template<class Args> inline static unsigned short ttov(Args&& args) { std::tm tm = mysql_time_to_tm(std::forward<Args>(args)); return static_cast<unsigned short>(std::mktime(&tm)); }
		};

		template<>
		struct convert<int>
		{
			template<class ...Args> inline static int stov(Args&&... args) { return static_cast<int>(str2l(std::forward<Args>(args)...)); }
			template<class Args> inline static int ntov(Args&& args) { return static_cast<int>(std::forward<Args>(args)); }
			template<class Args> inline static int ttov(Args&& args) { std::tm tm = mysql_time_to_tm(std::forward<Args>(args)); return static_cast<int>(std::mktime(&tm)); }
		};

		template<>
		struct convert<unsigned int>
		{
			template<class ...Args> inline static unsigned int stov(Args&&... args) { return static_cast<unsigned int>(str2ul(std::forward<Args>(args)...)); }
			template<class Args> inline static unsigned int ntov(Args&& args) { return static_cast<unsigned int>(std::forward<Args>(args)); }
			template<class Args> inline static unsigned int ttov(Args&& args) { std::tm tm = mysql_time_to_tm(std::forward<Args>(args)); return static_cast<unsigned int>(std::mktime(&tm)); }
		};

		template<>
		struct convert<long>
		{
			template<class ...Args> inline static long stov(Args&&... args) { return str2l(std::forward<Args>(args)...); }
			template<class Args> inline static long ntov(Args&& args) { return static_cast<long>(std::forward<Args>(args)); }
			template<class Args> inline static long ttov(Args&& args) { std::tm tm = mysql_time_to_tm(std::forward<Args>(args)); return static_cast<long>(std::mktime(&tm)); }
		};

		template<>
		struct convert<unsigned long>
		{
			template<class ...Args> inline static unsigned long stov(Args&&... args) { return str2ul(std::forward<Args>(args)...); }
			template<class Args> inline static unsigned long ntov(Args&& args) { return static_cast<unsigned long>(std::forward<Args>(args)); }
			template<class Args> inline static unsigned long ttov(Args&& args) { std::tm tm = mysql_time_to_tm(std::forward<Args>(args)); return static_cast<unsigned long>(std::mktime(&tm)); }
		};

		template<>
		struct convert<long long>
		{
			template<class ...Args> inline static long long stov(Args&&... args) { return str2ll(std::forward<Args>(args)...); }
			template<class Args> inline static long long ntov(Args&& args) { return static_cast<long long>(std::forward<Args>(args)); }
			template<class Args> inline static long long ttov(Args&& args) { std::tm tm = mysql_time_to_tm(std::forward<Args>(args)); return static_cast<long long>(std::mktime(&tm)); }
		};

		template<>
		struct convert<unsigned long long>
		{
			template<class ...Args> inline static unsigned long long stov(Args&&... args) { return str2ull(std::forward<Args>(args)...); }
			template<class Args> inline static unsigned long long ntov(Args&& args) { return static_cast<unsigned long long>(std::forward<Args>(args)); }
			template<class Args> inline static unsigned long long ttov(Args&& args) { std::tm tm = mysql_time_to_tm(std::forward<Args>(args)); return static_cast<unsigned long long>(std::mktime(&tm)); }
		};

		template<>
		struct convert<float>
		{
			template<class ...Args> inline static float stov(Args&&... args) { return str2f(std::forward<Args>(args)...); }
			template<class Args> inline static float ntov(Args&& args) { return static_cast<float>(std::forward<Args>(args)); }
			template<class Args> inline static float ttov(Args&& args) { std::tm tm = mysql_time_to_tm(std::forward<Args>(args)); return static_cast<float>(std::mktime(&tm)); }
		};

		template<>
		struct convert<double>
		{
			template<class ...Args> inline static double stov(Args&&... args) { return str2d(std::forward<Args>(args)...); }
			template<class Args> inline static double ntov(Args&& args) { return static_cast<double>(std::forward<Args>(args)); }
			template<class Args> inline static double ttov(Args&& args) { std::tm tm = mysql_time_to_tm(std::forward<Args>(args)); return static_cast<double>(std::mktime(&tm)); }
		};

		template<>
		struct convert<long double>
		{
			template<class ...Args> inline static long double stov(Args&&... args) { return str2ld(std::forward<Args>(args)...); }
			template<class Args> inline static long double ntov(Args&& args) { return static_cast<long double>(std::forward<Args>(args)); }
			template<class Args> inline static long double ttov(Args&& args) { std::tm tm = mysql_time_to_tm(std::forward<Args>(args)); return static_cast<long double>(std::mktime(&tm)); }
		};

		template<>
		struct convert<std::tm>
		{
			template<class Args> inline static std::tm stov(Args&&) { return std::tm{}; }
			template<class Args> inline static std::tm ntov(Args&&) { return std::tm{}; }
			template<class Args> inline static std::tm ttov(Args&& args) { return mysql_time_to_tm(std::forward<Args>(args)); }
		};

		template<class CharT, class Traits, class Allocator>
		struct convert<std::basic_string<CharT, Traits, Allocator>>
		{
			template<class Args> inline static std::basic_string<CharT, Traits, Allocator> stov(Args&& args, const std::string& format)
			{
				return fmt::format(format, std::forward<Args>(args));
			}
			template<class Args> inline static std::basic_string<CharT, Traits, Allocator> ntov(Args&& args, const std::string& format)
			{
				return fmt::format(format, std::forward<Args>(args));
			}
			template<class Args> inline static std::basic_string<CharT, Traits, Allocator> ttov(Args&& args, const std::string& format)
			{
				if (args == nullptr)
					return std::basic_string<CharT, Traits, Allocator>{};
				return fmt::format(format, mysql_time_to_tm(std::forward<Args>(args)));
			}
		};

		template<class CharT, class Traits>
		struct convert<std::basic_string_view<CharT, Traits>>
		{
			template<class Args> inline static std::basic_string_view<CharT, Traits> stov(Args&& args)
			{
				return std::basic_string_view<CharT, Traits>(std::forward<Args>(args));
			}
			template<class Args> inline static std::basic_string_view<CharT, Traits> ntov(Args&&)
			{
				throw std::runtime_error("not supported");
			}
			template<class Args> inline static std::basic_string_view<CharT, Traits> ttov(Args&&)
			{
				throw std::runtime_error("not supported");
			}
		};

		//template<class CharT, std::size_t N>
		//struct convert<CharT(&)[N]>
		//{
		//	template<class ...Args>
		//	inline static CharT* stov(CharT* str, Args&&... args)
		//	{
		//		auto s = std::basic_string<CharT>(std::forward<Args>(args)...);
		//		std::memcpy((void *)str, (const void *)s.data(), (std::min<std::size_t>)(N, s.size()));
		//		return str;
		//	}
		//	template<class Args>
		//	inline static CharT* ntov(CharT* str, Args&& args)
		//	{
		//		auto s = std::to_string(std::forward<Args>(args));
		//		std::memcpy((void *)str, (const void *)s.data(), (std::min<std::size_t>)(N, s.size()));
		//		return str;
		//	}
		//};

		template<class T>
		struct converter
		{
			template<class ...Args>
			inline static T tov(Args&&... args)
			{
				using tuple_type = std::tuple<std::decay_t<Args>...>;
				using from_type = std::remove_cv_t<std::remove_reference_t<std::tuple_element_t<0, tuple_type>>>;
				if constexpr /**/ (std::is_integral_v<from_type>)
				{
					return convert<T>::ntov(std::forward<Args>(args)...);
				}
				else if constexpr (std::is_floating_point_v<from_type>)
				{
					return convert<T>::ntov(std::forward<Args>(args)...);
				}
				else if constexpr (std::is_same_v<std::remove_pointer_t<from_type>, MYSQL_TIME>)
				{
					return convert<T>::ttov(std::forward<Args>(args)...);
				}
				else if constexpr (std::is_pointer_v<from_type>)
				{
					return convert<T>::stov(std::forward<Args>(args)...);
				}
				else if constexpr (std::is_same_v<std::string, from_type> || std::is_same_v<std::string_view, from_type>)
				{
					return convert<T>::stov(std::forward<Args>(args.data())...);
				}
				else
				{
					std::ignore = true;
				}
				return {};
			}
		};

		template<>
		struct converter<binder*>
		{
			template<class ...Args>
			inline static binder* tov(Args&&...)
			{
				return nullptr;
			}
		};
	}

	template<std::size_t> class fetcher;

	struct binder
	{
		MYSQL_FIELD         * field    = nullptr;
		MYSQL_BIND          * bind     = nullptr;
		unsigned long         length   = 0;
		bool                  is_null  = false;
		bool                  error    = false;
		std::unique_ptr<char> buffer;
		std::size_t           capacity = 0;
	};

	class recordset
	{
		template<std::size_t> friend class fetcher;
	protected:
		struct bind_data
		{
			std::vector<MYSQL_BIND> binds;
			std::vector<binder>     datas;

			bind_data(MYSQL_FIELD * fields, std::size_t size) : binds(size), datas(size)
			{
				for (std::size_t i = 0; i < size; ++i)
				{
					MYSQL_BIND& bind = binds[i];
					binder & data = datas[i];
					MYSQL_FIELD * field = fields ? (&(fields[i])) : nullptr;

					data.bind = &bind;

					if (field)
					{
						auto &&[buf, cap] = detail::allocate_buffer_for_field(field);
						data.field = field;
						data.buffer = std::move(buf);
						data.capacity = std::move(cap);

						bind.buffer_type = field->type;
						bind.is_null = &(data.is_null);
						bind.length = &(data.length);
						bind.error = &(data.error);
						bind.is_unsigned = field->flags & UNSIGNED_FLAG;
						bind.buffer = data.buffer.get();
						bind.buffer_length = static_cast<unsigned long>(data.capacity);
					}
				}
			}
		};
	public:
		template<class... Args>
		recordset(MYSQL* conn, const std::string_view& sql, Args&&... args) : conn_(conn)
		{
			this->stmt_ = mysql_stmt_init(conn_);
			if (!this->stmt_)
				throw std::runtime_error("Out of memory");

			if (mysql_stmt_prepare(this->stmt_, sql.data(), static_cast<unsigned long>(sql.size())))
				throw std::runtime_error(mysql_stmt_error(this->stmt_));

			this->res_ = mysql_stmt_result_metadata(this->stmt_);
			if (!this->res_ && mysql_stmt_errno(this->stmt_))
				throw std::runtime_error(mysql_stmt_error(this->stmt_));

			unsigned long param_count = mysql_stmt_param_count(this->stmt_);
			if (param_count)
			{
				this->ibinder_ = std::make_unique<bind_data>(nullptr, param_count);
				(((*this) << std::forward<Args>(args)), ...);
			}
		}
		~recordset()
		{
			try
			{
				this->execute();
			}
			catch (...)
			{
				std::ignore = true;
			}

			if (this->res_)
			{
				mysql_free_result(this->res_);
				this->res_ = nullptr;
			}

			if (this->stmt_)
			{
				mysql_stmt_close(this->stmt_);
				this->stmt_ = nullptr;
			}
		}

		recordset(const recordset&) = delete;
		recordset(recordset&& other) noexcept
		{
			this->swap(std::move(other));
		}

		recordset& operator=(const recordset&) = delete;
		recordset& operator=(recordset&& other) noexcept
		{
			return this->swap(std::move(other));
		}

		template<typename T> inline recordset& operator << (const T& val)
		{
			if (!this->stmt_ || !this->ibinder_) return (*this);

			if (!(this->iindex_ < this->ibinder_->binds.size())) return (*this);

			this->current_mode_ = 'i';

			using type = std::remove_cv_t<std::remove_reference_t<T>>;

			MYSQL_BIND& bind = this->ibinder_->binds[iindex_];
			binder& data = this->ibinder_->datas[iindex_];
			std::unique_ptr<char>& buffer = data.buffer;

			if constexpr (std::is_integral_v<type>)
			{
				data.capacity = sizeof(type);
				buffer.reset(new char[data.capacity]);
				std::memcpy((void *)buffer.get(), (const void *)&val, sizeof(type));

				if constexpr (std::is_same_v<type, bool>)
					bind.buffer_type = MYSQL_TYPE_BIT;
				else
				{
					if constexpr /**/ (sizeof(type) == sizeof(std::int8_t))
						bind.buffer_type = MYSQL_TYPE_TINY;
					else if constexpr (sizeof(type) == sizeof(std::int16_t))
						bind.buffer_type = MYSQL_TYPE_SHORT;
					else if constexpr (sizeof(type) == sizeof(std::int32_t))
						bind.buffer_type = MYSQL_TYPE_LONG;
					else if constexpr (sizeof(type) == sizeof(std::int64_t))
						bind.buffer_type = MYSQL_TYPE_LONGLONG;
					else
						std::ignore = true;

					if constexpr (std::is_unsigned_v<type>)
						bind.is_unsigned = true;
					else
						std::ignore = true;
				}
			}
			else if constexpr (std::is_floating_point_v<type>)
			{
				data.capacity = sizeof(type);
				buffer.reset(new char[data.capacity]);
				std::memcpy((void *)buffer.get(), (const void *)&val, sizeof(type));

				if constexpr /**/ (sizeof(type) == sizeof(float))
					bind.buffer_type = MYSQL_TYPE_FLOAT;
				else if constexpr (sizeof(type) == sizeof(double))
					bind.buffer_type = MYSQL_TYPE_DOUBLE;
				else
					bind.buffer_type = MYSQL_TYPE_DECIMAL;
			}
			else if constexpr (std::is_pointer_v<type>)
			{
				using CharT = std::remove_cv_t<std::remove_pointer_t<type>>;

				if (std::is_same_v<CharT, char>)
				{
					data.capacity = std::strlen(val);
					buffer.reset(new char[data.capacity]);
					std::memcpy((void *)buffer.get(), (const void *)val, data.capacity);

					bind.buffer_type = MYSQL_TYPE_STRING;
				}
				else
				{
					std::ignore = true;
				}
			}
			else if constexpr (std::is_same_v<type, std::string> || std::is_same_v<type, std::string_view>)
			{
				data.capacity = val.size();
				buffer.reset(new char[data.capacity]);
				std::memcpy((void *)buffer.get(), (const void *)val.data(), val.size());

				bind.buffer_type = MYSQL_TYPE_STRING;
			}
			else if constexpr (std::is_same_v<type, std::nullptr_t>)
			{
				bind.buffer_type = MYSQL_TYPE_NULL;
			}
			else if constexpr (std::is_same_v<type, std::tm>)
			{
				data.capacity = sizeof(MYSQL_TIME);
				buffer.reset(new char[sizeof(MYSQL_TIME)]);
				MYSQL_TIME* mysql_time = reinterpret_cast<MYSQL_TIME*>(buffer.get());

				mysql_time->second = val.tm_sec;   // seconds after the minute - [0, 60] including leap second
				mysql_time->minute = val.tm_min;   // minutes after the hour - [0, 59]
				mysql_time->hour = val.tm_hour;  // hours since midnight - [0, 23]
				mysql_time->day = val.tm_mday;  // day of the month - [1, 31]
				mysql_time->month = val.tm_mon + 1;   // months since January - [0, 11]
				mysql_time->year = val.tm_year + 1900;  // years since 1900
				mysql_time->second_part = 0;
				mysql_time->neg = false;
				mysql_time->time_type = MYSQL_TIMESTAMP_DATETIME;
				mysql_time->time_zone_displacement = 0;

				bind.buffer_type = MYSQL_TYPE_DATETIME;
			}
			else
			{
				(const_cast<T&>(val)).orm(*this);
				return (*this);
			}

			bind.buffer = (void *)buffer.get();
			bind.buffer_length = static_cast<unsigned long>(data.capacity);

			++iindex_;

			return (*this);
		}

		template<std::size_t N> inline recordset& operator <<(const char(&str)[N])
		{
			return ((*this) << (const char *)(str));
		}

		template <typename T, std::size_t I = 0>
		inline typename std::enable_if_t<!detail::is_callable_v<T>, recordset&> operator>>(T& val)
		{
			if (!this->stmt_) return (*this);

			this->current_mode_ = 'o';

			this->execute();

			if constexpr (false
				|| std::is_floating_point_v<T>
				|| std::is_integral_v<T>
				|| std::is_same_v<T, std::string>
				|| std::is_same_v<T, std::u16string>
				|| std::is_same_v<T, std::string_view>
				|| std::is_same_v<T, std::tm>
				|| std::is_same_v<T, binder*>)
			{
				int status = mysql_stmt_fetch(this->stmt_);
				if (status == 1)
					throw std::runtime_error(mysql_stmt_error(this->stmt_));
				else if (status == MYSQL_NO_DATA)
					return (*this);

				val = this->buffer_to_val<T>(I);
			}
			else
			{
				val.orm(*this);
			}

			return (*this);
		}

		template<typename... Args> inline recordset& operator>>(std::tuple<Args...>&& val)
		{
			return ((*this) >> val);
		}

		template<typename... Args> inline recordset& operator>>(std::tuple<Args...>& val)
		{
			if (!this->stmt_) return (*this);

			this->current_mode_ = 'o';

			this->execute();

			int status = mysql_stmt_fetch(this->stmt_);
			if (status == 1)
				throw std::runtime_error(mysql_stmt_error(this->stmt_));
			else if (status == MYSQL_NO_DATA)
				return (*this);

			fetcher<sizeof...(Args)>::fetch(*this, val);

			return (*this);
		}

		template <typename Fn>
		inline typename std::enable_if_t<detail::is_callable_v<Fn>, recordset&> operator>>(Fn&& function)
		{
			return ((*this) >> function);
		}

		template <typename Fn>
		inline typename std::enable_if_t<detail::is_callable_v<Fn>, recordset&> operator>>(Fn& function)
		{
			return ((*this) >> const_cast<const Fn&>(function));
		}

		template <typename Fn>
		inline typename std::enable_if_t<detail::is_callable_v<Fn>, recordset&> operator>>(const Fn& function)
		{
			if (!this->stmt_) return (*this);

			this->current_mode_ = 'o';

			this->execute();

			using fun_traits_type = detail::function_traits<Fn>;
			using T = typename fun_traits_type::template args<0>::type;

			while (true)
			{
				if constexpr (fun_traits_type::argc == 1 && !(false
					|| std::is_floating_point_v<T>
					|| std::is_integral_v<T>
					|| std::is_same_v<T, std::string>
					|| std::is_same_v<T, std::u16string>
					|| std::is_same_v<T, std::string_view>
					|| std::is_same_v<T, std::tm>
					|| std::is_same_v<T, binder*>))
				{
					T val{};
					if (val.orm(*this))
						function(val);
					else
						break;
				}
				else
				{
					int status = mysql_stmt_fetch(this->stmt_);
					if (status == 1)
						throw std::runtime_error(mysql_stmt_error(this->stmt_));
					else if (status == MYSQL_NO_DATA)
						break;

					fetcher<fun_traits_type::argc>::fetch(*this, function);
				}
			}

			return (*this);
		}

		/**
		 * Retrieve one row of records
		 */
		template<typename... Args> inline bool fetch(Args&&... args)
		{
			if (!this->stmt_) return false;

			this->current_mode_ = 'o';

			this->execute();

			using tuple_type = std::tuple<std::decay_t<Args>...>;
			using T = std::remove_cv_t<std::remove_reference_t<std::tuple_element_t<0, tuple_type>>>;

			if constexpr (sizeof...(Args) == 1 && !(false
				|| std::is_floating_point_v<T>
				|| std::is_integral_v<T>
				|| std::is_same_v<T, std::string>
				|| std::is_same_v<T, std::u16string>
				|| std::is_same_v<T, std::string_view>
				|| std::is_same_v<T, std::tm>
				|| std::is_same_v<T, binder*>))
			{
				(args.orm(*this), ...);
				return true;
			}
			else
			{
				int status = mysql_stmt_fetch(this->stmt_);
				if (status == 1)
					throw std::runtime_error(mysql_stmt_error(this->stmt_));
				else if (status == MYSQL_NO_DATA)
					return false;

				fetcher<sizeof...(Args)>::fetch_args(*this, std::make_index_sequence<sizeof...(Args)>{}, std::forward<Args>(args)...);

				return true;
			}
		}

		/**
		 * Retrieve one row of records
		 */
		template<typename... Args> inline bool operator()(Args&&... args)
		{
			if /**/ (this->current_mode_ == 'i')
			{
				(((*this) << std::forward<Args>(args)), ...);
				return true;
			}
			else if (this->current_mode_ == 'o')
			{
				return fetch(std::forward<Args>(args)...);
			}
			else
			{
#if defined(_DEBUG) || defined(DEBUG)
				assert(false);
#endif
			}
			return false;
		}

		/**
		 * execute the prepare statement
		 */
		inline recordset& execute()
		{
			while (!this->executed_ && this->stmt_)
			{
				struct autocommit
				{
					recordset& rs;
					autocommit(recordset& r) : rs(r) { mysql_autocommit(rs.conn_, false); }
					~autocommit() { mysql_autocommit(rs.conn_, true); }
				} autocommiter{ *this };

				if (this->ibinder_)
				{
					if (mysql_stmt_bind_param(this->stmt_, this->ibinder_->binds.data()))
						break;
				}

				if (mysql_stmt_execute(this->stmt_))
				{
					if (!this->res_)
					{
						mysql_rollback(conn_);
					}
					break;
				}
				else
				{
					if (!this->res_)
					{
						mysql_commit(conn_);
					}
				}

				if (this->res_)
				{
					bool attr_max_length = true;
					mysql_stmt_attr_set(this->stmt_, STMT_ATTR_UPDATE_MAX_LENGTH, (const void*)&attr_max_length);

					if (mysql_stmt_store_result(this->stmt_))
						break;

					if (!this->obinder_)
						this->obinder_ = std::make_unique<bind_data>(mysql_fetch_fields(this->res_), mysql_num_fields(this->res_));

					if (mysql_stmt_bind_result(this->stmt_, &(this->obinder_->binds[0])))
						break;
				}

				this->executed_ = true;

				break;
			}

			if (!this->executed_ && this->stmt_) throw std::runtime_error(mysql_stmt_error(this->stmt_));

			return (*this);
		}

		/**
		 * Return the number of rows affected or retrieved.
		 * Usually used for "insert, update, delete, select"
		 */
		std::uint64_t affect_rows()
		{
			return (this->stmt_ ? mysql_stmt_affected_rows(this->stmt_) : 0);
		}

		/**
		 * Return the number of rows in the result set.
		 * Usually used for "select"
		 */
		std::uint64_t result_rows()
		{
			return (this->stmt_ ? mysql_stmt_num_rows(this->stmt_) : 0);
		}

		/**
		 * Sets the output format for the date string, default is "{:%Y-%m-%d}"
		 */
		inline recordset& set_date_format    (std::string s) { this->date_format_     = s; return (*this); }
		/**
		 * Sets the output format for the time string, default is "{:%H:%M:%S}"
		 */
		inline recordset& set_time_format    (std::string s) { this->time_format_     = s; return (*this); }
		/**
		 * Sets the output format for the date and time string, default is "{:%Y-%m-%d %H:%M:%S}"
		 */
		inline recordset& set_datetime_format(std::string s) { this->datetime_format_ = s; return (*this); }
		/**
		 * Sets the output format for the integer string, default is "{}"
		 */
		inline recordset& set_integer_format (std::string s) { this->integer_format_  = s; return (*this); }
		/**
		 * Sets the output format for the float and double string, default is "{}"
		 */
		inline recordset& set_floating_format(std::string s) { this->floating_format_ = s; return (*this); }
		/**
		 * Sets the output format for every column
		 */
		template<typename... Args>
		inline recordset& set_fields_format(Args&&... args)
		{
			((this->fields_format_.emplace_back(std::forward<Args>(args))),...);
			return (*this);
		}

		/**
		 * returns the error code for the most recently invoked prepare statement api function
		 * call mysql_stmt_errno internal
		 */
		unsigned int errid()
		{
			return (this->stmt_ ? mysql_stmt_errno(this->stmt_) : 0);
		}

		/**
		 * returns a null-terminated string containing the error message for the most recently invoked prepare statement api function
		 * call mysql_stmt_error internal
		 */
		const char * error()
		{
			return (this->stmt_ ? mysql_stmt_error(this->stmt_) : "");
		}

		/**
		 * returns a pointer of MYSQL_STMT
		 */
		MYSQL_STMT* native_handle() { return this->stmt_; }

		inline recordset& swap(recordset&& other) noexcept
		{
			std::swap(this->conn_                  ,other.conn_               );
			std::swap(this->stmt_                  ,other.stmt_               );
			std::swap(this->res_                   ,other.res_                );
			std::swap(this->executed_              ,other.executed_           );
			std::swap(this->date_format_           ,other.date_format_        );
			std::swap(this->time_format_           ,other.time_format_        );
			std::swap(this->datetime_format_       ,other.datetime_format_    );
			std::swap(this->integer_format_        ,other.integer_format_     );
			std::swap(this->floating_format_       ,other.floating_format_    );
			std::swap(this->fields_format_	       ,other.fields_format_	  );
			std::swap(this->current_mode_          ,other.current_mode_       );
			std::swap(this->iindex_                ,other.iindex_             );
			std::swap(this->ibinder_			   ,other.ibinder_			  );
			std::swap(this->obinder_			   ,other.obinder_			  );

			return (*this);
		}

	protected:
		template <typename T>
		inline typename std::enable_if_t<false
			|| std::is_floating_point_v<T>
			|| std::is_integral_v<T>
			|| std::is_same_v<T, std::string>
			|| std::is_same_v<T, std::u16string>
			|| std::is_same_v<T, std::string_view>
			|| std::is_same_v<T, std::tm>
			|| std::is_same_v<T, binder*>
			, T> buffer_to_val(std::size_t i)
		{
			if (!this->obinder_) return T{};
			if (this->obinder_->binds.size() <= i) return T{};

			MYSQL_BIND& bind = this->obinder_->binds[i];
			binder & data = this->obinder_->datas[i];
			MYSQL_FIELD * field = data.field;

			if (!field) return T{};

			using type = std::remove_cv_t<std::remove_reference_t<T>>;
			if constexpr (std::is_pointer_v<type> && std::is_same_v<binder, std::remove_pointer_t<type>>)
				return ((T)(&data));
			else
				std::ignore = true;

			switch (field->type)
			{
			case MYSQL_TYPE_YEAR:
			case MYSQL_TYPE_TINY:
			case MYSQL_TYPE_SHORT:
			case MYSQL_TYPE_INT24:
			case MYSQL_TYPE_LONG:
			case MYSQL_TYPE_LONGLONG:
			{
				std::uint64_t ret = 0;
				switch (bind.buffer_length)
				{
				case 1:
					if (bind.is_unsigned)
						ret = !data.is_null ? *reinterpret_cast<std::uint8_t *>(bind.buffer) : 0;
					else
						ret = !data.is_null ? *reinterpret_cast<std::int8_t *>(bind.buffer) : 0;
					break;
				case 2:
					if (bind.is_unsigned)
						ret = !data.is_null ? *reinterpret_cast<std::uint16_t *>(bind.buffer) : 0;
					else
						ret = !data.is_null ? *reinterpret_cast<std::int16_t *>(bind.buffer) : 0;
					break;
				case 4:
					if (bind.is_unsigned)
						ret = !data.is_null ? *reinterpret_cast<std::uint32_t *>(bind.buffer) : 0;
					else
						ret = !data.is_null ? *reinterpret_cast<std::int32_t *>(bind.buffer) : 0;
					break;
				case 8:
					if (bind.is_unsigned)
						ret = !data.is_null ? *reinterpret_cast<std::uint64_t *>(bind.buffer) : 0;
					else
						ret = !data.is_null ? *reinterpret_cast<std::int64_t *>(bind.buffer) : 0;
					break;
				default:
					throw std::runtime_error("MySQL_Prepared_ResultSet::getInt64_intern: invalid type");
				}
				if constexpr (std::is_same_v<std::string, T>)
					return detail::converter<T>::tov(ret, this->fields_format_.size() > i ? this->fields_format_[i] : this->integer_format_);
				else
					return detail::converter<T>::tov(ret);
			}
			case MYSQL_TYPE_FLOAT:
			{
				float ret = !data.is_null ? *reinterpret_cast<float *>(bind.buffer) : 0.0f;
				if constexpr (std::is_same_v<std::string, T>)
					return detail::converter<T>::tov(ret, this->fields_format_.size() > i ? this->fields_format_[i] : this->floating_format_);
				else
					return detail::converter<T>::tov(ret);
			}
			case MYSQL_TYPE_DOUBLE:
			{
				double ret = !data.is_null ? *reinterpret_cast<double *>(bind.buffer) : 0.0;
				if constexpr (std::is_same_v<std::string, T>)
					return detail::converter<T>::tov(ret, this->fields_format_.size() > i ? this->fields_format_[i] : this->floating_format_);
				else
					return detail::converter<T>::tov(ret);
			}
			case MYSQL_TYPE_NULL:
			{
				break;
			}
			case MYSQL_TYPE_TIMESTAMP2:
			case MYSQL_TYPE_DATETIME2:
			case MYSQL_TYPE_NEWDATE:
			case MYSQL_TYPE_TIME2:
			case MYSQL_TYPE_TIMESTAMP:
			case MYSQL_TYPE_DATE:
			case MYSQL_TYPE_TIME:
			case MYSQL_TYPE_DATETIME:
			{
				MYSQL_TIME * ret = data.is_null ? nullptr : static_cast<MYSQL_TIME *>(bind.buffer);
				if constexpr (std::is_same_v<std::string, T>)
				{
					std::string* format = &datetime_format_;
					if /**/ (field->type == MYSQL_TYPE_DATE) format = &date_format_;
					else if (field->type == MYSQL_TYPE_TIME) format = &time_format_;
					else if (field->type == MYSQL_TYPE_NEWDATE) format = &date_format_;
					else if (field->type == MYSQL_TYPE_TIME2) format = &time_format_;
					return detail::converter<T>::tov(ret, this->fields_format_.size() > i ? this->fields_format_[i] : *format);
				}
				else
					return detail::converter<T>::tov(ret);
			}
			case MYSQL_TYPE_BIT:
			{
				std::uint64_t ret = 0;
				/* This length is in bytes, on the contrary to what can be seen in mysql_resultset.cpp where the Meta is used */
				if (!data.is_null)
				{
					switch (data.length)
					{
					case 8:ret = (std::uint64_t)bit_uint8korr(bind.buffer); break;
					case 7:ret = (std::uint64_t)bit_uint7korr(bind.buffer); break;
					case 6:ret = (std::uint64_t)bit_uint6korr(bind.buffer); break;
					case 5:ret = (std::uint64_t)bit_uint5korr(bind.buffer); break;
					case 4:ret = (std::uint64_t)bit_uint4korr(bind.buffer); break;
					case 3:ret = (std::uint64_t)bit_uint3korr(bind.buffer); break;
					case 2:ret = (std::uint64_t)bit_uint2korr(bind.buffer); break;
					case 1:ret = (std::uint64_t)bit_uint1korr(bind.buffer); break;
					}
				}
				if constexpr (std::is_same_v<std::string, T>)
					return detail::converter<T>::tov(ret, this->fields_format_.size() > i ? this->fields_format_[i] : this->integer_format_);
				else
					return detail::converter<T>::tov(ret);
			}
#if LIBMYSQL_VERSION_ID >= 80019
			case MYSQL_TYPE_TYPED_ARRAY:
#endif //LIBMYSQL_VERSION_ID >= 80019
			case MYSQL_TYPE_DECIMAL:
			case MYSQL_TYPE_NEWDECIMAL:
			case MYSQL_TYPE_TINY_BLOB:
			case MYSQL_TYPE_MEDIUM_BLOB:
			case MYSQL_TYPE_LONG_BLOB:
			case MYSQL_TYPE_BLOB:
			case MYSQL_TYPE_ENUM:
			case MYSQL_TYPE_SET:
#if LIBMYSQL_VERSION_ID > 50700
			case MYSQL_TYPE_JSON:
#endif //LIBMYSQL_VERSION_ID > 50700
			case MYSQL_TYPE_VARCHAR:
			case MYSQL_TYPE_VAR_STRING:
			case MYSQL_TYPE_STRING:
			{
				char* ret = reinterpret_cast<char*>(bind.buffer);
				ret[data.length] = '\0';
				if constexpr (std::is_same_v<std::string, T>)
					return detail::converter<T>::tov(ret, this->fields_format_.size() > i ? this->fields_format_[i] : "{}");
				else
					return detail::converter<T>::tov(ret);
			}
			case MYSQL_TYPE_GEOMETRY:
				break;
			}
			return T{};
		}

	protected:
		MYSQL                                            * conn_               = nullptr;
		MYSQL_STMT                                       * stmt_               = nullptr;
		MYSQL_RES                                        * res_                = nullptr;
		bool                                               executed_           = false;
		std::string                                        date_format_        = "{:%Y-%m-%d}";
		std::string                                        time_format_        = "{:%H:%M:%S}";
		std::string                                        datetime_format_    = "{:%Y-%m-%d %H:%M:%S}";
		std::string                                        integer_format_     = "{}";
		std::string                                        floating_format_    = "{}";
		std::vector<std::string>                           fields_format_;
		char                                               current_mode_       = '\0';
		int                                                iindex_             = 0;
		std::unique_ptr<bind_data>                         ibinder_;
		std::unique_ptr<bind_data>                         obinder_;
	};

	template<std::size_t Count>
	class fetcher
	{
		friend class recordset;
	protected:
		template<typename T, typename... Indexs, std::size_t Boundary = Count>
		static inline typename std::enable_if_t<(sizeof...(Indexs) < Boundary), void>
			fetch(recordset& db, T& obj, Indexs&&... indexs)
		{
			fetch<T>(db, obj, std::forward<Indexs>(indexs)..., sizeof...(Indexs));
		}

		template<typename T, typename... Indexs, std::size_t Boundary = Count>
		static inline typename std::enable_if_t<(sizeof...(Indexs) == Boundary), void>
			fetch(recordset& db, T& obj, Indexs&&... indexs)
		{
			call(db, obj, std::make_index_sequence<sizeof...(Indexs)>{});
		}

		template<typename T, std::size_t... I>
		static inline void call(recordset& db, T& obj, const std::index_sequence<I...>&)
		{
			if constexpr (detail::is_callable_v<T>)
			{
				using fun_traits_type = detail::function_traits<T>;

				obj((db.buffer_to_val<typename fun_traits_type::template args<I>::type>(I))...);
			}
			else
			{
				obj = std::forward_as_tuple((db.buffer_to_val<std::remove_cv_t<std::remove_reference_t<
					typename std::tuple_element_t<I, T>>>>(I))...);
			}
		}

		template<typename... Args, std::size_t... I>
		static inline void fetch_args(recordset& db, const std::index_sequence<I...>&, Args&&... args)
		{
			((args = (db.buffer_to_val<std::remove_cv_t<std::remove_reference_t<Args>>>(I))), ...);
		}
	};

	class database
	{
	public:
		database()
		{
			mysql_init(&conn_);
		}
		~database()
		{
			mysql_close(&conn_);
		}

		database& connect(
			const std::string_view& host,
			const std::string_view& user,
			const std::string_view& passwd,
			const std::string_view& db,
			unsigned int port = 3306
		)
		{
			// Call mysql_options() after mysql_init() and before mysql_connect() or mysql_real_connect().
			// See : https://dev.mysql.com/doc/refman/5.7/en/mysql-options.html

			bool reconnect = true;
			mysql_options(&conn_, MYSQL_OPT_RECONNECT, (const void *)&reconnect);

			if (!mysql_real_connect(&conn_, host.data(), user.data(), passwd.data(), db.data(), port, nullptr, 0))
				throw std::runtime_error(mysql_error(&conn_));

			// SET CHARACTER SET Statement
			// charset_name may be quoted or unquoted.
			//mysql_query(&conn_, "SET NAMES GBK");

			return (*this);
		}

		/**
		 * Construct a prepare statement for the sql, but do not execute the prepare statement immediately,
		 * it will be executed automatically when the recordset object is destroyed
		 */
		recordset operator<<(const std::string_view& sql)
		{
			return recordset(&conn_, sql);
		}

		/**
		 * Construct a prepare statement and execute it immediately
		 */
		template<typename... Args>
		recordset execute(const std::string_view& sql, Args&&... args)
		{
			auto rs = recordset(&conn_, sql, std::forward<Args>(args)...);
			rs.execute();
			return rs;
		}

		/**
		 * set the default character set for the current connection, such as "utf8" "gbk" and so on
		 * call mysql_set_character_set internal
		 */
		database& set_charset(const std::string_view& charset)
		{
			// "SET NAMES GBK"
			// SET CHARACTER SET Statement
			// charset_name may be quoted or unquoted.
			//std::string s = fmt::format("SET NAMES {}", charset);
			//if (mysql_query(&conn_, s.data()))
			//	throw std::runtime_error(mysql_error(&conn_));
			mysql_set_character_set(&conn_, charset.data());
			return (*this);
		}

		/**
		 * Checks whether the connection to the server is working
		 * call mysql_ping internal
		 */
		inline bool is_alive()
		{
			return mysql_ping(&conn_) == 0;
		}

		/**
		 * returns the error code for the most recently invoked API function
		 * call mysql_errno internal
		 */
		unsigned int errid()
		{
			return mysql_errno(&conn_);
		}

		/**
		 * returns a null-terminated string containing the error message for the most recently invoked API function
		 * call mysql_error internal
		 */
		const char * error()
		{
			return mysql_error(&conn_);
		}

		/**
		 * returns a refrence of MYSQL
		 */
		MYSQL& native_handle() { return this->conn_; }

	protected:
		MYSQL conn_;
	};
}

#undef FMT_HEADER_ONLY
