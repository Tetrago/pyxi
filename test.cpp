#include <gtest/gtest.h>

#include <array>
#include <pyxi.hpp>
#include <string>
#include <vector>

#define CAT2(x, y) x##y
#define CAT(x, y)  CAT2(x, y)

#define EXPECT_TRUE_V(name, ...)                                            \
	{                                                                       \
		bool CAT(v, __LINE__) = name<__VA_ARGS__>::value;                   \
		EXPECT_TRUE(CAT(v, __LINE__)) << #name "<" #__VA_ARGS__ ">::value"; \
	}

#define EXPECT_FALSE_V(name, ...)                                            \
	{                                                                        \
		bool CAT(v, __LINE__) = name<__VA_ARGS__>::value;                    \
		EXPECT_FALSE(CAT(v, __LINE__)) << #name "<" #__VA_ARGS__ ">::value"; \
	}

using namespace pyxi;

TEST(trait, is_resizable)
{
	EXPECT_FALSE_V(is_resizable, int);
	EXPECT_FALSE_V(is_resizable, std::array<int, 5>);
	EXPECT_TRUE_V(is_resizable, std::string);
	EXPECT_TRUE_V(is_resizable, std::vector<int>);
}

TEST(trait, is_iterable)
{
	EXPECT_FALSE_V(is_iterable, int);
	EXPECT_TRUE_V(is_iterable, std::array<int, 5>);
	EXPECT_TRUE_V(is_iterable, std::string);
	EXPECT_TRUE_V(is_iterable, std::vector<int>);
}

struct HasSerializer
{
	void serialize(Serializer&) const;
};

struct HasDeserializer
{
	void deserialize(Deserializer&);
};

TEST(trait, has_serializer)
{
	EXPECT_TRUE_V(has_serializer, HasSerializer);
	EXPECT_FALSE_V(has_serializer, HasDeserializer);
}

TEST(trait, has_deserializer)
{
	EXPECT_FALSE_V(has_deserializer, HasSerializer);
	EXPECT_TRUE_V(has_deserializer, HasDeserializer);
}

struct Trio
{
	uint32_t a;
	bool b;
	char c;
};

TEST(sequence, size)
{
	using type0 = make_sequence<0>;
	EXPECT_EQ(type0::size(), 0);

	using type10 = make_sequence<10>;
	EXPECT_EQ(type10::size(), 10);
}

TEST(trait, is_initializable)
{
	EXPECT_TRUE_V(is_initializable, Trio, uint32_t);
	EXPECT_FALSE_V(is_initializable, Trio, uint32_t, char);
	EXPECT_TRUE_V(is_initializable, Trio, uint32_t, bool);
	EXPECT_TRUE_V(is_initializable, Trio, uint32_t, bool, char);
	EXPECT_FALSE_V(is_initializable, Trio, uint32_t, bool, char, int);
}

struct Empty
{};

struct Large
{
	int x00;
	int x01;
	int x02;
	int x03;
	int x04;
	int x05;
	int x06;
	int x07;
	int x08;
	int x09;
	int x10;
	int x11;
	int x12;
	int x13;
	int x14;
	int x15;
	int x16;
	int x17;
	int x18;
	int x19;
	int x20;
	int x21;
	int x22;
	int x23;
	int x24;
	int x25;
	int x26;
	int x27;
	int x28;
	int x29;
};

struct Flags
{
	Spare<4> spare;
	Bits<uint8_t, 2> a;
	Bits<uint8_t, 2> b;
};

TEST(trait, member_count)
{
	size_t count;

	count = member_count<Empty>::value;
	EXPECT_EQ(count, 0);

	count = member_count<Trio>::value;
	EXPECT_EQ(count, 3);

	count = member_count<Flags>::value;
	EXPECT_EQ(count, 3);

	count = member_count<Large>::value;
	EXPECT_EQ(count, 30);
}

TEST(trait, member_offsets)
{
	auto offsets = member_offsets<Trio>::value();
	ASSERT_EQ(offsets.size(), 3);

	EXPECT_EQ(offsets[0], 0);
	EXPECT_EQ(offsets[1], 4);
	EXPECT_EQ(offsets[2], 5);
}

TEST(serialize, trio_lsb)
{
	Trio trio{0x12345678, true, 0};

	auto bytes = serialize(trio, ByteOrder::LsbFirst);
	ASSERT_EQ(bytes.size(), 6);

	// 0xf0
	EXPECT_EQ(bytes[0], 0x78);
	EXPECT_EQ(bytes[1], 0x56);
	EXPECT_EQ(bytes[2], 0x34);
	EXPECT_EQ(bytes[3], 0x12);

	// true
	EXPECT_EQ(bytes[4], 0x01);

	// false
	EXPECT_EQ(bytes[5], 0x00);
}

TEST(serialize, trio_msb)
{
	Trio trio{0x12345678, true, 0};

	auto bytes = serialize(trio, ByteOrder::MsbFirst);
	ASSERT_EQ(bytes.size(), 6);

	// 0xf0
	EXPECT_EQ(bytes[0], 0x12);
	EXPECT_EQ(bytes[1], 0x34);
	EXPECT_EQ(bytes[2], 0x56);
	EXPECT_EQ(bytes[3], 0x78);

	// true
	EXPECT_EQ(bytes[4], 0x01);

	// false
	EXPECT_EQ(bytes[5], 0x00);
}

TEST(serialize, flags)
{
	Flags flags{{}, 1, 2};

	auto bytes = serialize(flags, ByteOrder::LsbFirst);
	ASSERT_EQ(bytes.size(), 1);

	EXPECT_EQ(bytes[0], 0b10010000);
}

TEST(deserialize, uint32_msb)
{
	std::vector<uint8_t> buffer = {0x12, 0x34, 0x56, 0x78};
	uint32_t v = deserialize<uint32_t>(buffer, ByteOrder::MsbFirst);

	EXPECT_EQ(v, 0x12345678);
}

TEST(deserialize, uint32_lsb)
{
	std::vector<uint8_t> buffer = {0x12, 0x34, 0x56, 0x78};
	uint32_t v = deserialize<uint32_t>(buffer, ByteOrder::LsbFirst);

	EXPECT_EQ(v, 0x78563412);
}

TEST(deserialize, int8)
{
	std::vector<uint8_t> buffer = {static_cast<uint8_t>(-5)};

	int8_t v = deserialize<int8_t>(buffer);

	EXPECT_EQ(v, -5);
}

TEST(roundtrip, flags)
{
	Flags flags{{}, 1, 2};

	auto bytes = serialize(flags);
	ASSERT_EQ(bytes.size(), 1);

	flags = {};
	deserialize(flags, bytes);

	EXPECT_EQ(*flags.a, 1);
	EXPECT_EQ(*flags.b, 2);
}

TEST(roundtrip, float)
{
	float v = -1.25f;

	auto bytes = serialize(v);
	ASSERT_EQ(bytes.size(), sizeof(float));

	v = {};
	deserialize(v, bytes);

	EXPECT_EQ(v, -1.25f);
}
