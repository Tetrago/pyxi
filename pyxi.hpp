// MIT License
//
// Copyright (c) 2024 James Geiss
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef PYXI_HPP
#define PYXI_HPP

#include <array>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#if __cplusplus >= 201703L
#define PYXI_CXX 17
#elif __cplusplus >= 201402L
#define PYXI_CXX 14
#elif __cplusplus >= 201103L
#define PYXI_CXX 11
#else
#error "Unsupported C++ version (<11)"
#endif

namespace pyxi
{

/////////////////
//// bitsize ////
/////////////////

template <typename T = uint8_t>
struct bitsize : public std::integral_constant<size_t, sizeof(T) * CHAR_BIT>
{};

////////////////
//// void_t ////
////////////////

#if PYXI_CXX >= 17
using std::void_t;
#else
template <typename...>
struct make_void
{
	using type = void;
};

template <typename... Ts>
using void_t = typename make_void<Ts...>::type;
#endif

/////////////////////
//// enable_if_t ////
/////////////////////

#if PYXI_CXX >= 14
using std::enable_if_t;
#else
template <bool B, typename T = void>
using enable_if_t = typename std::enable_if<B, T>::type;
#endif

//////////////////////
//// is_resizable ////
//////////////////////

template <typename, typename = void>
struct is_resizable : public std::false_type
{};

template <typename T>
struct is_resizable<
    T,
    void_t<decltype(std::declval<T&>().resize(
        std::declval<decltype(std::declval<const T&>().size())>()))>>
    : public std::true_type
{};

/////////////////////
//// is_iterable ////
/////////////////////

template <typename, typename = void>
struct is_iterable : public std::false_type
{};

template <typename T>
struct is_iterable<
    T,
    void_t<decltype(std::declval<T&>().begin(), std::declval<T&>().end())>>
    : public std::true_type
{};

////////////////
//// Policy ////
////////////////

enum class Priority
{
	First,
	Primary,
	Secondary,
	Last
};

template <typename T, typename = void, Priority P = Priority::First>
struct Policy
    : public Policy<
          T,
          void,
          static_cast<Priority>(
              static_cast<typename std::underlying_type<Priority>::type>(P) +
              1)>
{};

template <typename T>
struct Policy<T, void, Priority::Last>
{
	static_assert(false, "No policy exists for this type");
};

////////////////////
//// Serializer ////
////////////////////

class Serializer
{
public:
	template <typename T>
	Serializer& operator<<(const T& t);

	template <typename T>
	void put(const T& t);

	template <typename T, typename = enable_if_t<std::is_integral<T>::value>>
	void give(T data, size_t bits = bitsize<T>::value);

protected:
	virtual void impl(size_t data, size_t bits) = 0;
};

template <typename T>
Serializer& Serializer::operator<<(const T& t)
{
	put(t);
	return *this;
}

template <typename T>
void Serializer::put(const T& t)
{
	Policy<T>::serialize(t, *this);
}

template <typename T, typename>
void Serializer::give(const T data, size_t bits)
{
	if (bits > bitsize<T>::value)
	{
		throw std::invalid_argument(
		    "Attempting to invoke serializer with bit size exceeding given "
		    "type");
	}
	if (sizeof(T) > sizeof(size_t))
	{
		throw std::invalid_argument(
		    "Attempting to invoke serializer with bit size exceeding platform "
		    "support");
	}
	else if (bits == 0)
	{
		throw std::invalid_argument(
		    "Attempting to invoke serializer with no bits");
	}
	else
	{
		impl(data, bits);
	}
}

//////////////////////
//// Deserializer ////
//////////////////////

class Deserializer
{
public:
	template <typename T>
	Deserializer& operator>>(T& t);

	template <typename T>
	void get(T& t);

	template <typename T, typename = enable_if_t<std::is_integral<T>::value>>
	T take(size_t bits = bitsize<T>::value);

protected:
	virtual void impl(size_t& data, size_t bits, bool signExtend) = 0;
};

template <typename T>
Deserializer& Deserializer::operator>>(T& t)
{
	get(t);
	return *this;
}

template <typename T>
void Deserializer::get(T& t)
{
	Policy<T>::deserialize(t, *this);
}

template <typename T, typename>
inline T Deserializer::take(size_t bits)
{
	if (bits > bitsize<T>::value)
	{
		throw std::invalid_argument(
		    "Attempting to invoke deserializer with bit size exceeding given "
		    "type");
	}
	if (sizeof(T) > sizeof(size_t))
	{
		throw std::invalid_argument(
		    "Attempting to invoke deserializer with bit size exceeding "
		    "platform "
		    "support");
	}
	else if (bits == 0)
	{
		throw std::invalid_argument(
		    "Attempting to invoke deserializer with no bits");
	}
	else
	{
		size_t v{};
		impl(v, bits, std::is_signed<T>::value);
		return static_cast<T>(v);
	}
}

////////////////////////
//// has_serializer ////
////////////////////////

template <typename, typename = void>
struct has_serializer : public std::false_type
{};

template <typename T>
struct has_serializer<T,
                      void_t<decltype(std::declval<const T&>().serialize(
                          std::declval<Serializer&>()))>>
    : public std::true_type
{};

//////////////////////////
//// has_deserializer ////
//////////////////////////

template <typename, typename = void>
struct has_deserializer : public std::false_type
{};

template <typename T>
struct has_deserializer<T,
                        void_t<decltype(std::declval<T&>().deserialize(
                            std::declval<Deserializer&>()))>>
    : public std::true_type
{};

////////////////////////
//// Default Policy ////
////////////////////////

template <typename T>
struct Policy<
    T,
    enable_if_t<has_serializer<T>::value && has_deserializer<T>::value>,
    Priority::Primary>
{
	static void serialize(const T& t, Serializer& ser) { t.serialize(ser); }

	static void deserialize(T& t, Deserializer& des) { t.deserialize(des); }
};

template <typename T>
struct Policy<
    T,
    enable_if_t<has_serializer<T>::value && !has_deserializer<T>::value>,
    Priority::Primary>
{
	static void serialize(const T& t, Serializer& ser) { t.serialize(ser); }

	static void deserialize(T& t, Deserializer& des)
	{
		static_assert(false, "Type has not implemented a deserializer");
	}
};

template <typename T>
struct Policy<
    T,
    enable_if_t<!has_serializer<T>::value && has_deserializer<T>::value>,
    Priority::Primary>
{
	static void serialize(const T& t, Serializer& ser)
	{
		static_assert(false, "Type has not implemented a serializer");
	}

	static void deserialize(T& t, Deserializer& des) { t.deserialize(des); }
};

/////////////////////////
//// Integral Policy ////
/////////////////////////

template <typename T>
struct Policy<T, enable_if_t<std::is_integral<T>::value>, Priority::Primary>
{
	static void serialize(T t, Serializer& ser) { ser.give(t); }

	static void deserialize(T& t, Deserializer& des) { t = des.take<T>(); }
};

/////////////////////
//// Enum Policy ////
/////////////////////

template <typename T>
struct Policy<T, enable_if_t<std::is_enum<T>::value>, Priority::Primary>
{
	static void serialize(T t, Serializer& ser)
	{
		auto v = static_cast<typename std::underlying_type<T>::type>(t);
		ser.give(v);
	}

	static void deserialize(T& t, Deserializer& des)
	{
		auto v = des.take<typename std::underlying_type<T>::type>();
		t      = static_cast<T>(v);
	}
};

//////////////////////
//// Float Policy ////
//////////////////////

template <typename T, typename = void>
struct floating_width_equivalent
{};

template <typename T>
struct floating_width_equivalent<
    T,
    enable_if_t<std::is_floating_point<T>::value && bitsize<T>::value == 16>>
{
	using type = uint16_t;
};

template <typename T>
struct floating_width_equivalent<
    T,
    enable_if_t<std::is_floating_point<T>::value && bitsize<T>::value == 32>>
{
	using type = uint32_t;
};

template <typename T>
struct floating_width_equivalent<
    T,
    enable_if_t<std::is_floating_point<T>::value && bitsize<T>::value == 64>>
{
	using type = uint64_t;
};

template <typename T>
struct Policy<T,
              void_t<typename floating_width_equivalent<T>::type>,
              Priority::Primary>
{
	static void serialize(T t, Serializer& ser)
	{
		typename floating_width_equivalent<T>::type v;
		std::memcpy(&v, &t, sizeof(T));

		ser.give(v);
	}

	static void deserialize(T& t, Deserializer& des)
	{
		typename floating_width_equivalent<T>::type v;
		v = des.take<decltype(v)>();

		std::memcpy(&t, &v, sizeof(T));
	}
};

///////////////////////////
//// Collection Policy ////
///////////////////////////

template <typename T>
struct Policy<T,
              enable_if_t<is_resizable<T>::value && is_iterable<T>::value>,
              Priority::Primary>
{
	static void serialize(const T& t, Serializer& ser)
	{
		ser.put(t.size());

		for (auto it = t.begin(); it != t.end(); ++it)
		{
			ser.put(*it);
		}
	}

	static void deserialize(T& t, Deserializer& des)
	{
		decltype(t.size()) size;
		des.get(size);
		t.resize(size);

		for (auto it = t.begin(); it != t.end(); ++it)
		{
			des.get(*it);
		}
	}
};

template <typename T>
struct Policy<T,
              enable_if_t<!is_resizable<T>::value && is_iterable<T>::value>,
              Priority::Primary>
{
	static void serialize(const T& t, Serializer& ser)
	{
		for (auto it = t.begin(); it != t.end(); ++it)
		{
			ser.put(*it);
		}
	}

	static void deserialize(T& t, Deserializer& des)
	{
		for (auto it = t.begin(); it != t.end(); ++it)
		{
			des.get(*it);
		}
	}
};

//////////////////
//// sequence ////
//////////////////

template <size_t... Is>
struct sequence

{
	static constexpr size_t size() noexcept { return sizeof...(Is); }
};

namespace detail
{
	template <size_t C, size_t... Is>
	struct make_sequence_impl : public make_sequence_impl<C - 1, C - 1, Is...>
	{};

	template <size_t... Is>
	struct make_sequence_impl<0, Is...>
	{
		using type = sequence<Is...>;
	};
} // namespace detail

template <size_t C>
using make_sequence = typename detail::make_sequence_impl<C>::type;

//////////////
//// pack ////
//////////////

template <typename... Ts>
struct pack
{
	static constexpr size_t size() noexcept { return sizeof...(Ts); }
};

//////////////////////////
//// is_initializable ////
//////////////////////////

namespace detail
{
	template <typename, typename, typename...>
	struct is_initializable_impl : public std::false_type
	{};

	template <typename T, typename... Ts>
	struct is_initializable_impl<T,
	                             void_t<decltype(T{std::declval<Ts>()...})>,
	                             Ts...> : public std::true_type
	{};
} // namespace detail

template <typename T, typename... Ts>
using is_initializable = detail::is_initializable_impl<T, void, Ts...>;

/////////////////////
//// member_pack ////
/////////////////////

namespace detail
{
	template <typename, typename, typename = void, typename...>
	struct member_pack_impl
	{
		using type = pack<>;
	};

	template <typename T, typename Atom>
	struct member_pack_impl<T,
	                        Atom,
	                        enable_if_t<!std::is_empty<T>::value &&
	                                    std::is_standard_layout<T>::value &&
	                                    std::is_class<T>::value>>
	    : public member_pack_impl<T, Atom, void, Atom>
	{};

	template <typename T, typename Atom, typename T0, typename... Ts>
	struct member_pack_impl<
	    T,
	    Atom,
	    enable_if_t<is_initializable<T, T0, Ts..., T0, Ts...>::value>,
	    T0,
	    Ts...> : public member_pack_impl<T, Atom, void, T0, Ts..., T0, Ts...>
	{};

	template <typename T, typename Atom, typename T0, typename... Ts>
	struct member_pack_impl<
	    T,
	    Atom,
	    enable_if_t<is_initializable<T, T0, Ts...>::value &&
	                is_initializable<T, Atom, T0, Ts...>::value &&
	                !is_initializable<T, T0, Ts..., T0, Ts...>::value>,
	    T0,
	    Ts...> : public member_pack_impl<T, Atom, void, Atom, T0, Ts...>
	{};

	template <typename T, typename Atom, typename T0, typename... Ts>
	struct member_pack_impl<
	    T,
	    Atom,
	    enable_if_t<is_initializable<T, T0, Ts...>::value &&
	                !is_initializable<T, Atom, T0, Ts...>::value>,
	    T0,
	    Ts...>
	{
		using type = pack<T0, Ts...>;
	};
} // namespace detail

template <typename T, typename Atom>
using member_pack = typename detail::member_pack_impl<T, Atom>::type;

//////////////////////
//// member_count ////
//////////////////////

namespace detail
{
	struct atom
	{
		template <typename T>
		constexpr operator T();
	};

	template <typename... Ts>
	constexpr size_t member_count_impl(pack<Ts...>) noexcept
	{
		return sizeof...(Ts);
	}
} // namespace detail

template <typename T>
struct member_count
    : public std::integral_constant<size_t,
                                    detail::member_count_impl(
                                        member_pack<T, detail::atom>{})>
{};

////////////////////////
//// member_offsets ////
////////////////////////

namespace detail
{
	struct atom_offset_binder
	{
		template <typename T>
		operator T() const noexcept
		{
			base    = (base + alignof(T) - 1) / alignof(T) * alignof(T);
			offset  = base;
			base   += sizeof(T);

			return {};
		}

		size_t& base;
		size_t& offset;
	};

	template <typename T, size_t... Is>
	std::array<size_t, sizeof...(Is)> member_offsets_impl(
	    sequence<Is...>) noexcept
	{
		size_t base = 0;
		std::array<size_t, sizeof...(Is)> offsets;
		T{atom_offset_binder{base, offsets[Is]}...};
		return offsets;
	}
} // namespace detail

template <typename T>
struct member_offsets
{
	static auto value() noexcept -> decltype(detail::member_offsets_impl<T>(
	    make_sequence<member_count<T>::value>{}))
	{
		return detail::member_offsets_impl<T>(
		    make_sequence<member_count<T>::value>{});
	}
};

////////////////////////////
//// member_serializers ////
////////////////////////////

namespace detail
{
	using atom_serializer = void (*)(const void*, size_t, Serializer&);

	struct atom_serializer_binder
	{
		template <typename T>
		operator T() const noexcept
		{
			*serializer =
			    [](const void* pData, size_t offset, Serializer& ser) {
				    ser.put(*reinterpret_cast<const T*>(
				        static_cast<const uint8_t*>(pData) + offset));
			    };

			return {};
		}

		atom_serializer* serializer;
	};

	template <typename T, size_t... Is>
	std::array<atom_serializer, sizeof...(Is)> member_serializers_impl(
	    sequence<Is...> seq) noexcept
	{
		std::array<atom_serializer, sizeof...(Is)> serializers;
		T{atom_serializer_binder{&serializers[Is]}...};
		return serializers;
	}
} // namespace detail

template <typename T>
struct member_serializers
{
	static auto value() noexcept -> decltype(detail::member_serializers_impl<T>(
	    make_sequence<member_count<T>::value>{}))
	{
		return detail::member_serializers_impl<T>(
		    make_sequence<member_count<T>::value>{});
	}
};

//////////////////////////////
//// member_deserializers ////
//////////////////////////////

namespace detail
{
	using atom_deserializer = void (*)(void*, size_t, Deserializer&);

	struct atom_deserializer_binder
	{
		template <typename T>
		operator T() const noexcept
		{
			*deserializer = [](void* pData, size_t offset, Deserializer& des) {
				des.get(*reinterpret_cast<T*>(static_cast<uint8_t*>(pData) +
				                              offset));
			};

			return {};
		}

		atom_deserializer* deserializer;
	};

	template <typename T, size_t... Is>
	std::array<atom_deserializer, sizeof...(Is)> member_deserializers_impl(
	    sequence<Is...> seq) noexcept
	{
		std::array<atom_deserializer, sizeof...(Is)> deserializers;
		T{atom_deserializer_binder{&deserializers[Is]}...};
		return deserializers;
	}
} // namespace detail

template <typename T>
struct member_deserializers
{
	static auto value() noexcept
	    -> decltype(detail::member_deserializers_impl<T>(
	        make_sequence<member_count<T>::value>{}))
	{
		return detail::member_deserializers_impl<T>(
		    make_sequence<member_count<T>::value>{});
	}
};

///////////////////////
//// Member Policy ////
///////////////////////

template <typename T>
struct Policy<
    T,
    enable_if_t<std::is_class<T>::value && std::is_standard_layout<T>::value &&
                member_count<T>::value != 0>,
    Priority::Secondary>
{
	static void serialize(const T& t, Serializer& ser)
	{
		auto offsets     = member_offsets<T>::value();
		auto serializers = member_serializers<T>::value();

		for (size_t i = 0; i < offsets.size(); ++i)
		{
			serializers[i](&t, offsets[i], ser);
		}
	}

	static void deserialize(T& t, Deserializer& des)
	{
		auto offsets       = member_offsets<T>::value();
		auto deserializers = member_deserializers<T>::value();

		for (size_t i = 0; i < offsets.size(); ++i)
		{
			deserializers[i](&t, offsets[i], des);
		}
	}
};

//////////////
//// Bits ////
//////////////

template <typename T, size_t Width, typename = void>
class Bits
{
public:
	enum
	{
		bitwidth = Width
	};

	Bits() noexcept = default;
	Bits(T value) noexcept;

	T& operator*() noexcept { return value; }

	T operator*() const noexcept { return value; }

	const T* operator&() const noexcept { return &value; }

	T* operator&() noexcept { return &value; }

	Bits& operator=(T value) noexcept
	{
		this->value = value;
		return *this;
	}

private:
	T value;
};

template <typename T, size_t Width, typename U>
Bits<T, Width, U>::Bits(T value) noexcept
    : value(value)
{}

template <typename T, size_t Width>
class Bits<T, Width, enable_if_t<(Width > bitsize<T>::value)>>
{
	static_assert(Width <= bitsize<T>::value,
	              "Instantiation of Bits is too large");
};

template <typename T, size_t Width>
struct Policy<Bits<T, Width>, void, Priority::Primary>
{
	static void serialize(const Bits<T, Width>& t, Serializer& ser)
	{
		ser.give(*t, Width);
	}

	static void deserialize(Bits<T, Width>& t, Deserializer& des)
	{
		*t = des.take<T>(Width);
	}
};

///////////////
//// Spare ////
///////////////

template <size_t Width, typename = void>
struct Spare
{
	enum
	{
		bitwidth = Width
	};
};

template <size_t Width>
struct Spare<Width, enable_if_t<(Width > bitsize<size_t>::value)>>
{
	static_assert(Width <= bitsize<size_t>::value,
	              "Instantiation of Spare is too large");
};

template <size_t Width>
struct Policy<Spare<Width>, void, Priority::Primary>
{
	static void serialize(const Spare<Width>& t, Serializer& ser)
	{
		ser.give<size_t>(0, Width);
	}

	static void deserialize(const Spare<Width>& t, Deserializer& des)
	{
		des.take<size_t>(Width);
	}
};

////////////////////
//// Byte Order ////
////////////////////

enum class ByteOrder
{
	MsbFirst,
	LsbFirst
};

/////////////////////////////
//// Bytewise Serializer ////
/////////////////////////////

class BytewiseSerializer : public Serializer
{
public:
	BytewiseSerializer(ByteOrder byteOrder) noexcept;

	void flush();

protected:
	virtual void putByte(uint8_t byte) = 0;

	void impl(size_t data, size_t bits) override;

private:
	ByteOrder byteOrder_;
	uint8_t byte_    = 0;
	uint8_t bitsSet_ = 0;
};

inline BytewiseSerializer::BytewiseSerializer(ByteOrder byteOrder) noexcept
    : byteOrder_(byteOrder)
{}

inline void BytewiseSerializer::flush()
{
	if (bitsSet_ != 0)
	{
		putByte(byte_);
		byte_    = 0;
		bitsSet_ = 0;
	}
}

inline void BytewiseSerializer::impl(size_t data, size_t bits)
{
	if (byteOrder_ == ByteOrder::MsbFirst)
	{
		data <<= bitsize<size_t>::value - bits;
	}

	const uint8_t byteMask = byteOrder_ == ByteOrder::MsbFirst ? 0x01 : 0x80;
	const size_t dataMask  = byteOrder_ == ByteOrder::MsbFirst
	                             ? size_t{1} << (bitsize<size_t>::value - 1)
	                             : 1;

	while (bits--)
	{
		if (data & dataMask)
		{
			byte_ |= byteMask;
		}

		if (++bitsSet_ == bitsize<>::value)
		{
			putByte(byte_);
			byte_    = 0;
			bitsSet_ = 0;
		}

		if (byteOrder_ == ByteOrder::MsbFirst)
		{
			byte_ <<= 1;
			data  <<= 1;
		}
		else
		{
			byte_ >>= 1;
			data  >>= 1;
		}
	}
}

///////////////////////////////
//// Bytewise Deserializer ////
///////////////////////////////

class BytewiseDeserializer : public Deserializer
{
public:
	BytewiseDeserializer(ByteOrder byteOrder) noexcept;

protected:
	virtual uint8_t getByte() = 0;

	void impl(size_t& data, size_t bits, bool signExtend) override;

private:
	ByteOrder byteOrder_;
	uint8_t byte_;
	uint8_t bitsLeft_ = 0;
};

inline BytewiseDeserializer::BytewiseDeserializer(ByteOrder byteOrder) noexcept
    : byteOrder_(byteOrder)
{}

inline void BytewiseDeserializer::impl(size_t& data,
                                       size_t bits,
                                       bool signExtend)
{
	size_t signMask = -1;

	const uint8_t byteMask = byteOrder_ == ByteOrder::LsbFirst ? 0x01 : 0x80;
	const size_t dataMask  = byteOrder_ == ByteOrder::LsbFirst
	                             ? size_t{1} << (bitsize<size_t>::value - 1)
	                             : 1;

	for (size_t i = bits; i > 0; --i)
	{
		if (byteOrder_ == ByteOrder::LsbFirst)
		{
			byte_    >>= 1;
			data     >>= 1;
			signMask >>= 1;
		}
		else
		{
			byte_    <<= 1;
			data     <<= 1;
			signMask <<= 1;
		}

		if (bitsLeft_-- == 0)
		{
			byte_     = getByte();
			bitsLeft_ = bitsize<>::value - 1;
		}

		if (byte_ & byteMask)
		{
			data |= dataMask;
		}
	}

	if (byteOrder_ == ByteOrder::LsbFirst)
	{
		data     >>= bitsize<size_t>::value - bits;
		signMask   = ~signMask;
	}

	if (signExtend)
	{
		data |= signMask;
	}
}

////////////////////////////
//// Dynamic Serializer ////
////////////////////////////

class DynamicSerializer : public BytewiseSerializer
{
public:
	DynamicSerializer(ByteOrder byteOrder) noexcept;

	const std::vector<uint8_t>& data() const noexcept { return bytes; }

protected:
	void putByte(uint8_t byte) override;

private:
	std::vector<uint8_t> bytes;
};

inline DynamicSerializer::DynamicSerializer(ByteOrder byteOrder) noexcept
    : BytewiseSerializer(byteOrder)
{}

inline void DynamicSerializer::putByte(uint8_t byte)
{
	bytes.push_back(byte);
}

/////////////////////////////
//// Buffer Deserializer ////
/////////////////////////////

class BufferDeserializer : public BytewiseDeserializer
{
public:
	BufferDeserializer(const void* pData,
	                   size_t size,
	                   ByteOrder byteOrder) noexcept;

protected:
	uint8_t getByte() override;

private:
	const uint8_t* byte_;
	size_t size_;
};

inline BufferDeserializer::BufferDeserializer(const void* pData,
                                              size_t size,
                                              ByteOrder byteOrder) noexcept
    : BytewiseDeserializer(byteOrder),
      byte_(static_cast<const uint8_t*>(pData)),
      size_(size)
{}

inline uint8_t BufferDeserializer::getByte()
{
	if (size_ == 0)
	{
		throw std::out_of_range("Buffer deserializer out of range");
	}
	else
	{
		--size_;
		return *(byte_++);
	}
}

///////////////////
//// serialize ////
///////////////////

template <typename T>
std::vector<uint8_t> serialize(const T& t,
                               ByteOrder byteOrder = ByteOrder::MsbFirst)
{
	DynamicSerializer ser(byteOrder);
	ser << t;
	ser.flush();
	return ser.data();
}

/////////////////////
//// deserialize ////
/////////////////////

template <typename T>
void deserialize(T& t,
                 const std::vector<uint8_t>& data,
                 ByteOrder byteOrder = ByteOrder::MsbFirst)
{
	BufferDeserializer des(data.data(), data.size(), byteOrder);
	des >> t;
}

template <typename T>
void deserialize(T& t,
                 const void* pData,
                 size_t size,
                 ByteOrder byteOrder = ByteOrder::MsbFirst)
{
	BufferDeserializer des(pData, size, byteOrder);
	des >> t;
}

template <typename T>
T deserialize(const std::vector<uint8_t>& data,
              ByteOrder byteOrder = ByteOrder::MsbFirst)
{
	T t{};
	deserialize(t, data.data(), data.size(), byteOrder);
	return t;
}

template <typename T>
T deserialize(const void* pData,
              size_t size,
              ByteOrder byteOrder = ByteOrder::MsbFirst)
{
	T t{};
	deserialize(t, pData, size, byteOrder);
	return t;
}

///////////////
//// align ////
///////////////

inline void align(std::vector<uint8_t>& data, size_t alignment)
{
	if (data.size() % alignment != 0)
	{
		for (size_t i = 0; i < alignment - (data.size() % alignment); ++i)
		{
			data.push_back(0);
		}
	}
}

inline std::vector<uint8_t> align(std::vector<uint8_t>&& data, size_t alignment)
{
	if (data.size() % alignment != 0)
	{
		for (size_t i = 0; i < alignment - (data.size() % alignment); ++i)
		{
			data.push_back(0);
		}
	}

	return data;
}

} // namespace pyxi

#endif
