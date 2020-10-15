#pragma once
//Author: Ugo Varetto
//
//SeRialiZation Framework (SRZ).
//This code is distributed under the terms of the GNU General Public License
//as published by the Free Software Foundation, either version 3 of the License,
//or (at your option) any later version.
//
//srz is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with srz.  If not, see <http://www.gnu.org/licenses/>.

//! \file Serialize.h
//! \brief Implementation of specializations for serializing any type to
//! a byte array.
//!
//! The \c Serializer instance is selected through
//! \code
//! GetSerializer< T >::Type
//! \endcode
//!
//! Do specialize \c GetSerialize as needed.
//! All serializers expose the inteface:
//! \code
//! static ByteArray Pack(const T &d, ByteArray buf = ByteArray())
//! static ByteIterator Pack(const T &d, ByteIterator i)
//! static ConstByteIterator UnPack(ConstByteIterator i, T& d)
//! \endcode
//!
//! The preferred way of serializing/deserializing data is through the
//! \c Pack, \c UnPack and \c UnPackTuple functions.
//! \sa Pack, UnPack, UnPackTuple

#pragma once
#include <cstring>
#include <vector>
#include <string>
#include <type_traits>
#include <map>

#ifdef ZRF_UNSIGNED_CHAR
using Byte = unsigned char;
#else
using Byte = char;
static_assert(sizeof(char) == 1, "sizeof(char) != 1");
#endif

#ifdef ZRF_int32_size
#include <cstdint>
using Size = int32_t;
#else
using Size = size_t;
#endif

//! Serialization framework
namespace srz {
using ByteArray = std::vector< Byte >;
using ByteIterator = ByteArray::iterator;
using ConstByteIterator = ByteArray::const_iterator;

template< typename T >
struct GetSerializer;

//Serializer definitions

//! \defgroup Serializers
//! @{
//! Serialize POD data/
//! Use \c memmove to copy data into buffer.
template< typename T >
struct SerializePOD {
    static ByteArray Pack(const T& d, ByteArray buf = ByteArray()) {
        const Size sz = Size(buf.size());
        buf.resize(buf.size() + sizeof(d));
        memmove(buf.data() + sz, &d, sizeof(d));
        return buf;
    }
    static ByteIterator Pack(const T& d, ByteIterator i) {
        memmove(&*i, &d, sizeof(d));
        return i + sizeof(d);
    }
    static ConstByteIterator UnPack(ConstByteIterator i, T& d) {
        memmove(&d, &*i, sizeof(T));
        return i + sizeof(T);
    }
    //size of serialized data
    static size_t Sizeof(const T& d) { return sizeof(d); }
};

//! Fallback for standard copy constructible objects.
//! @warning verify that indeed works with your custom type
#ifndef SRZ_DISABLE_DEFAULT
//! Serialize copy constructible objects.
//! Placement \c new and copy constructors are used to copy data into buffer
template< typename T >
struct Serialize {
    static ByteArray Pack(const T& d, ByteArray buf = ByteArray()) {
        const Size sz = Size(buf.size());
        buf.resize(buf.size() + sizeof(d));
        new(buf.data() + sz) T(d); //copy constructor
        return buf;
    }
    static ByteIterator Pack(const T& d, ByteIterator i) {
        new(&*i) T(d); //copy constructor
        return i + sizeof(d);
    }
    static ConstByteIterator UnPack(ConstByteIterator i, T& d) {
        d = *reinterpret_cast< const T* >(&*i); //assignment operator
        return i + sizeof(T);
    }
    //size of serialized data
    static size_t Sizeof(const T& d) { return sizeof(d); }
};
#endif

//! Specialization for pair
template< typename F, typename S >
struct SerializePair {
    using P = std::pair< F, S >;
    using FS = typename GetSerializer< F >::Type;
    using SS = typename GetSerializer< S >::Type;
    static ByteArray Pack(const P& p, ByteArray buf = ByteArray()) {
        buf = FS::Pack(p.first, buf);
        return SS::Pack(p.second, buf);
    }
    static ByteIterator Pack(const P& p, ByteIterator i) {
        i = FS::Pack(p.first, i);
        return SS::Pack(p.second, i);
    }
    static ConstByteIterator UnPack(ConstByteIterator i, P& d) {
        F f;
        S s;
        i = FS::UnPack(i, f);
        i = SS::UnPack(i, s);
        d = std::make_pair(f, s);
        return i;
    }
    //size of serialized data
    static size_t Sizeof(const P& p) {
        return FS::Sizeof(p.first) + SS::Sizeof(p.second);
    }
};

//! Specialization for \c vector of POD types.
template< typename T >
struct SerializeVectorPOD {
    using ST = Size;//typename std::vector< T >::size_type;
    static ByteArray Pack(const std::vector< T >& d,
                          ByteArray buf = ByteArray()) {
        const Size sz = Size(buf.size());
        const ST s = ST(d.size());
        buf.resize(buf.size() + sizeof(T) * d.size() + sizeof(ST));
        memmove(buf.data() + sz, &s, sizeof(s));
        memmove(buf.data() + sz + sizeof(s), d.data(), sizeof(T) * d.size());
        return buf;
    }
    static ByteIterator Pack(const std::vector< T >& d, ByteIterator i) {
        const ST s = d.size();
        memmove(&*i, &s, sizeof(s));
        memmove(&*i + sizeof(s), d.data(), d.size() * sizeof(T));
        return i + sizeof(T) * d.size() + sizeof(s);
    }
    static ConstByteIterator UnPack(ConstByteIterator i, std::vector< T >& d) {
        ST s = 0;
        memmove(&s, &*i, sizeof(s));
        d.resize(s);
        memmove(d.data(), &*(i + sizeof(s)), s * sizeof(T));
        return i + sizeof(s) + sizeof(T) * s;
    }
    //size of serialized data
    static size_t Sizeof(const std::vector< T >& v) {
        const size_t sz = sizeof(Size);
        const size_t bs = sizeof(T) * v.size();
        return  sz + bs;
    }
};

//! Specialization for \c vector of non-POD types.
template< typename T >
struct SerializeVector {
    using ST = Size;/*typename std::vector<
        typename std::remove_cv< T >::type >::size_type;*/
    using TS = typename GetSerializer<
        typename std::remove_cv< T >::type >::Type;
    static ByteArray Pack(const std::vector< T >& d,
                          ByteArray buf = ByteArray()) {
        const Size sz = Size(buf.size());
        const Size s = Size(d.size());
        buf.resize(buf.size() + sizeof(s));
        memmove(buf.data() + sz, &s, sizeof(s));
        for(decltype(d.cbegin()) i = d.cbegin(); i != d.cend(); ++i) {
            buf = TS::Pack(*i, buf);
        }
        return buf;
    }
    static ByteIterator Pack(const std::vector< T >& d, ByteIterator bi) {
        const ST s = ST(d.size());
        memmove(&*bi, &s, sizeof(s));
        bi += sizeof(s);
#if __cplusplus == 201402L
        for (decltype(cbegin(d)) i = cbegin(d); i != cend(d); ++i) {
            bi = TS::Pack(*i, bi);
        }
#else
        for (decltype(begin(d)) i = begin(d); i != end(d); ++i) {
            bi = TS::Pack(*i, bi);
        }
#endif
        return bi;
    }
    static ConstByteIterator UnPack(ConstByteIterator bi, std::vector< T >& d) {
        ST s = 0;
        memmove(&s, &*bi, sizeof(s));
        bi += sizeof(s);
        d.reserve(s);
        for (ST i = 0; i != s; ++i) {
            T data;
            bi = TS::UnPack(bi, data);
            d.push_back(std::move(data));
        }
        return bi;
    }
    //!size of serialized data
    //! @warning O(n) operation
    static size_t Sizeof(const std::vector< T >& v) {
        size_t size = sizeof(Size);
        for(auto& e: v) size += TS::Sizeof(e);
        return size;
    }
};



//! \c std::string serialization.
struct SerializeString {
    using T = std::string::value_type;
    using V = std::vector< T >;
    static ByteArray Pack(const std::string& d,
                          ByteArray buf = ByteArray()) {
        return SerializeVectorPOD< T >::Pack(V(d.begin(), d.end()), buf);
    }
    static ByteIterator Pack(const std::string& d, ByteIterator bi) {
        return SerializeVectorPOD< T >::Pack(V(d.begin(), d.end()), bi);
    }
    static ConstByteIterator UnPack(ConstByteIterator bi, std::string& d) {
        V v;
        bi = SerializeVectorPOD< T >::UnPack(bi, v);
        d = std::string(v.begin(), v.end());
        return bi;
    }
    //size of serialized data
    static size_t Sizeof(const std::string& v) {
        return sizeof(size_t) + v.size() * sizeof(T);
    }
};

//! Specialization for \c std::map
template< typename K, typename T >
struct SerializeMap {
    using KS = typename GetSerializer< K >::Type;
    using VS = typename GetSerializer< T >::Type;
    using SS = SerializePOD< Size >;
    static ByteArray Pack(const std::map< K, T >& m,
                          ByteArray buf = ByteArray()) {
        buf = SS::Pack(m.size(), buf);
        for(auto& mi: m) {
            buf = KS::Pack(mi.first, buf);
            buf = VS::Pack(mi.second, buf);
        }
        return buf;
    }
    static ByteIterator Pack(const std::map< K, T >& m, ByteIterator bi) {
        bi = SS::Pack(m.size(), bi);
        for(auto& mi: m) {
            bi = KS::Pack(mi.first, bi);
            bi = VS::Pack(mi.second, bi);
        }
        return bi;
    }
    static ConstByteIterator UnPack(ConstByteIterator bi,
                                    std::map< K, T >& d) {

        Size size = 0;
        bi = SS::UnPack(bi, size);
        for(Size i = 0; i != size; ++i) {
            K key;
            T value;
            bi = KS::UnPack(bi, key);
            bi = VS::UnPack(bi, value);
            d.insert(std::make_pair(key, value));
        }
        return bi;
    }
    //! Compute size of serialized data
    //! @warning O(n) operation
    static size_t Sizeof(const std::map< K, T >& m) {
        size_t size = sizeof(size_t);
        for(auto& mi: m) {
            size += KS::Sizeof(mi.first);
            size += VS::Sizeof(mi.second);
        }
        return size;
    }
};



//! @}

//! \defgroup \code [Serializer selection]
//! Return proper specialization from type.
//! @{
//! Select serializer for \code [std::vector< T >] type, also removing
//! \c cv qualifiers.
template< typename T >
struct GetSerializer< std::vector< T > > {
    using NCV = typename std::remove_cv< T >::type;
    using Type = typename std::conditional< std::is_pod< NCV >::value,
                                            SerializeVectorPOD< NCV >,
                                            SerializeVector< NCV > >::type;
};

//! Select serializer for \c [const std::vector].
template< typename T >
struct GetSerializer< const std::vector< T > > {
    using Type = typename GetSerializer< std::vector< T > >::Type;
};

//! Select serializer for \c [volatile std::vector].
template< typename T >
struct GetSerializer< volatile std::vector< T > > {
    using Type = typename GetSerializer< std::vector< T > >::Type;
};

//! Select serializer for scalar type; also removing \cv qualifiers.
template< typename T >
struct GetSerializer {
    using NCV = typename std::remove_cv< T >::type;
    using Type = typename std::conditional< std::is_pod< NCV >::value,
                                            SerializePOD< NCV >,
                                            Serialize< NCV > >::type;
};

//! Select serializer for \c std::string.
template<>
struct GetSerializer< std::string > {
    using Type = SerializeString;
};

//! Select serializer for \c [const std::string].
template<>
struct GetSerializer< const std::string > {
    using Type = SerializeString;
};

//! Select serializer for \c [volatile std::string].
template<>
struct GetSerializer< volatile std::string > {
    using Type = SerializeString;
};


template <typename F, typename S>
struct GetSerializer< std::pair< F, S > > {
    using Type = SerializePair< F, S >;
};

template <typename F, typename S>
struct GetSerializer< const std::pair< F, S > > {
    using Type = SerializePair< F, S >;
};
template <typename F, typename S>
struct GetSerializer< volatile std::pair< F, S > > {
    using Type = SerializePair< F, S >;
};

namespace detail {
//! Logical compile-time AND condition.
template< typename H, typename...T >
struct And {
    static const bool Value = H::value && And< T... >::Value;
};

//! Logical compile-time AND condition - end of iteration case.
template< typename T >
struct And< T > {
    static const bool Value = T::value;
};
}

//! Selection of \c tuple serializer: if \c tuple types are all POD then
//! slect a POD serializer otherwise a generic serializer for copy
//! constructible objects.
template< typename...ArgsT >
struct GetSerializer< std::tuple< ArgsT... > > {
    using Type =
    typename std::conditional<
        detail::And< std::is_pod< ArgsT >... >::Value,
        SerializePOD< std::tuple< ArgsT... > >,
        Serialize< std::tuple< ArgsT... > > >::type;
};

//! \code [const tuple] specialization.
template< typename...ArgsT >
struct GetSerializer< const std::tuple< ArgsT... > > {
    using Type =
    typename std::conditional< detail::And< std::is_pod< ArgsT >... >::Value,
                               SerializePOD< std::tuple< ArgsT... > >,
                               Serialize< std::tuple< ArgsT... > > >::type;
};

//! \code [\volatile tuple] specialization.
template< typename...ArgsT >
struct GetSerializer< volatile std::tuple< ArgsT... > > {
    using Type =
    typename std::conditional< detail::And< std::is_pod< ArgsT >... >::Value,
                               SerializePOD< std::tuple< ArgsT... > >,
                               Serialize< std::tuple< ArgsT... > > >::type;
};


template < typename K, typename T >
struct GetSerializer< std::map< K, T > > {
    using Type = SerializeMap< K, T >;
};

//! \defgroup raw pointer serialization
//! Prevent from automatically serializing raw pointers.
//! @{
template< typename T >
struct GetSerializer< T* >;
template< typename T >
struct GetSerializer< const T* >;
template< typename T >
struct GetSerializer< volatile T* >;
//! @}
//! @}


//! \defgroup Packing/Unpacking
//! Serialize data to byte array: void specialization.
inline ByteArray Pack(ByteArray&& ba) {
    return ba;
}

//! Serialize data to byte array.
template< typename T, typename... ArgsT >
ByteArray Pack(ByteArray&& ba, const T& h, const ArgsT&... t) {
    return Pack(GetSerializer< T >::Type::Pack(h, std::move(ba)), t...);
};

//! Serialize data into newly created byte array.
template< typename... ArgsT >
ByteArray Pack(const ArgsT&... t) {
    return Pack(ByteArray(), t...);
};

//! Serialize data to byte array at position pointed by iterator,
//! void specialization
inline ByteIterator Pack(ByteIterator bi) {
    return bi;
}

//! Serialize data to byte array at position pointed by iterator
template< typename T, typename... ArgsT >
ByteIterator Pack(ByteIterator&& bi, const T& h, const ArgsT&... t) {
    return Pack(GetSerializer< T >::Type::Pack(h, std::move(bi)), t...);
};


//! Serialize data to byte array in place, termination condition
inline size_t Pack(ByteArray&) {
    return 0;
}

//! Serialize data to byte array in place
template< typename T, typename... ArgsT >
size_t Pack(ByteArray& ba, const T& h, const ArgsT&... t) {
    const size_t sz = ba.size();
    const size_t size = GetSerializer< T >::Type::Sizeof(h);
    ba.resize(sz + size);
    GetSerializer< T >::Type::Pack(h, ba.begin() + sz);
    return size + Pack(ba, t...);
};

template< typename... ArgsT >
ByteArray PackArgs(ArgsT...args) {
    return Pack(ByteArray(), args...);
};


//! Return de-serialized data from byte array iterator
//! (e.g. \code [const char*]).
template< typename T >
typename std::remove_reference<
    typename std::remove_cv< T >::type >::type
UnPack(ConstByteIterator bi) {
    using U = typename std::remove_reference<
        typename std::remove_cv< T >::type >::type;
    U d;
    GetSerializer< U >::Type::UnPack(bi, d);
    return d;
}

//! De-serialize data from byte array iterator (e.g. \code [const char*])
//! into reference.
template< typename T >
ConstByteIterator UnPack(ConstByteIterator bi, T& d) {
    return GetSerializer< T >::Type::UnPack(bi, d);
}

//! De-serialize data from byte array.
template< typename T >
T UnPack(const ByteArray& ba) {
    T d;
    UnPack(ba.begin(), d);
    return d;
}

//! Upack and convert data from byte array.
template< typename T >
T To(const ByteArray& ba) {
    return UnPack< T >(begin(ba));
}

//! Private namespace for
namespace detail {

//! Index sequence; use it to access elements at compile time
//! e.g. \code [foo(std::get< Is >(mytuple)...);]
template< size_t... > struct Seq {};

//! Index sequence generator; termination condition
template< size_t M, size_t... Ints >
struct GenSeq : GenSeq< M - 1, M - 1, Ints... > {};
//! Index sequence generator.
template< size_t... Ints >
struct GenSeq< 0, Ints... > {
    using Type = Seq< Ints... >;
};

//! Construct new tuple instance from new head element and other tuple, use
//! compile time index sequence to expand parameter pack into \code[make_tuple]
//! invocation.
template< typename H, typename... T, size_t... Is >
std::tuple< H, T... > TupleConsHelper(const H& h,
                                      const std::tuple< T... >& t,
                                      const Seq< Is... >&) {
    return std::make_tuple(h, std::get< Is >(t)...);
}

//! Construct new tuple by addind head element to other tuple.
template< typename H, typename...T >
std::tuple< H, T... > TupleCons(const H& h, const std::tuple< T... >& t) {
    return TupleConsHelper(h, t, typename GenSeq< sizeof...(T) >::Type());
};

//! Helper for unpacking element into tuple.
template< typename T, typename...ArgsT >
struct UnpackTHelper {
    static std::tuple< T, ArgsT... > Unpack(ConstByteIterator& bi) {
        T d = T();
        bi = UnPack(bi, d);
        std::tuple< ArgsT... > t =
            UnpackTHelper< ArgsT... >::Unpack(bi);
        return TupleCons(d, t);
    }
};

//! Helper for unpacking element into tuple: termination specialization.
template< typename T >
struct UnpackTHelper< T > {
    static std::tuple< T > Unpack(ConstByteIterator& bi) {
        T d = T();
        bi = UnPack(bi, d);
        return std::make_tuple(d);
    }
};
}

//! Unpack individual values into tuple: updated iterator
template< typename...ArgsT >
std::tuple< ArgsT... > UnPackTuple(ConstByteIterator& bi) {
    return detail::UnpackTHelper< ArgsT... >::Unpack(bi);
}

//! Unpack individual values into tuple: from temporary iterator (e.g. .begin())
template< typename...ArgsT >
std::tuple< ArgsT... > UnPackTuple(const ConstByteIterator& bi) {
    return detail::UnpackTHelper< ArgsT... >::Unpack(bi);
}


//! Unpack individual values into tuple: from temporary iterator (e.g. .begin())
template< typename...ArgsT >
std::tuple< ArgsT... > UnPackTuple(const ByteArray& ba) {
    ConstByteIterator bi = ba.begin();
    return detail::UnpackTHelper< ArgsT... >::Unpack(bi);
}

}
