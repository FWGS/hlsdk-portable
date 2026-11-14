/*
minmax.h - min, max and clamp for the original CZDS code
The original was built with MSVC, where min and max accept mixed argument
types, e.g. min( float, int ). std::min doesn't, so C++ gets templates that
return the common type of both arguments.
*/
#pragma once
#if !defined( MINMAX_H )
#define MINMAX_H

#if defined( __cplusplus )
#undef min
#undef max

// no decltype: the old compilers of the Steam Runtime build as C++98
template <class T> struct MinMaxRank { enum { value = -1 }; }; // enums and the rest: converted by the other argument
template <> struct MinMaxRank<char> { enum { value = 1 }; };
template <> struct MinMaxRank<signed char> { enum { value = 1 }; };
template <> struct MinMaxRank<unsigned char> { enum { value = 1 }; };
template <> struct MinMaxRank<short> { enum { value = 2 }; };
template <> struct MinMaxRank<unsigned short> { enum { value = 2 }; };
template <> struct MinMaxRank<int> { enum { value = 3 }; };
template <> struct MinMaxRank<unsigned int> { enum { value = 4 }; };
template <> struct MinMaxRank<long> { enum { value = 5 }; };
template <> struct MinMaxRank<unsigned long> { enum { value = 6 }; };
template <> struct MinMaxRank<long long> { enum { value = 7 }; };
template <> struct MinMaxRank<unsigned long long> { enum { value = 8 }; };
template <> struct MinMaxRank<float> { enum { value = 9 }; };
template <> struct MinMaxRank<double> { enum { value = 10 }; };
template <> struct MinMaxRank<long double> { enum { value = 11 }; };

template <bool Cond, class A, class B> struct MinMaxSelect { typedef A type; };
template <class A, class B> struct MinMaxSelect<false, A, B> { typedef B type; };

template <class A, class B> struct MinMaxType
{
	typedef typename MinMaxSelect<( (int)MinMaxRank<A>::value >= (int)MinMaxRank<B>::value ), A, B>::type type;
};
template <class T> struct MinMaxType<T, T> { typedef T type; };

template <class A, class B>
inline typename MinMaxType<A, B>::type min( const A &a, const B &b )
{
	return a < b ? a : b;
}

template <class A, class B>
inline typename MinMaxType<A, B>::type max( const A &a, const B &b )
{
	return a > b ? a : b;
}
#else // !__cplusplus
#if !defined( min )
#define min( a, b )	((( a ) < ( b )) ? ( a ) : ( b ))
#endif

#if !defined( max )
#define max( a, b )	((( a ) > ( b )) ? ( a ) : ( b ))
#endif
#endif // !__cplusplus

#if !defined( clamp )
#define clamp( val, lo, hi )	((( val ) > ( hi )) ? ( hi ) : ((( val ) < ( lo )) ? ( lo ) : ( val )))
#endif

#endif // MINMAX_H
