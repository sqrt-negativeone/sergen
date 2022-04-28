#ifndef TEST_META_H
#define TEST_META_H

#ifndef SG_ArrayCount
# define SG_ArrayCount(a) (sizeof(a) / sizeof((a)[0]))
#endif

#define SG_MAX(a, b) (((a)>(b))?(a):(b))
#define SG_MIN(a, b) (((a)<(b))?(a):(b))

#ifndef SG_ASSERT
#define SG_ASSERT(c) if (!(c)) { *(volatile int *)0 = 0; }
#endif

#ifndef SG_BUFFER_TYPE
# error Buffer Type Was not Provided
#endif

typedef SG_BUFFER_TYPE SG_Buffer;

#ifndef SG_WRITE_TO_BUFFER
# error Definition For SG_WRITE_TO_BUFFER(buffer, src, nbytes) was not provided
#endif

#ifndef SG_READ_FROM_BUFFER
# error Definition For SG_READ_FROM_BUFFER(buffer, dst, nbytes) was not provided
#endif

#ifndef SG_BUFFER_FREE_SIZE
# error Definition for SG_BUFFER_FREE_SIZE(buffer) was not provided
#endif

#ifndef SG_BUFFER_SIZE
# error Definition For SG_BUFFER_SIZE(buffer) was not provided
#endif

// Base Types
#ifndef SG_u8_TYPE
# error Definition for SG_u8_TYPE was not provided
#endif
typedef SG_u8_TYPE SG_u8;
#ifndef SG_u16_TYPE
# error Definition for SG_u16_TYPE was not provided
#endif
typedef SG_u16_TYPE SG_u16;
#ifndef SG_u32_TYPE
# error Definition for SG_u32_TYPE was not provided
#endif
typedef SG_u32_TYPE SG_u32;
#ifndef SG_u64_TYPE
# error Definition for SG_u64_TYPE was not provided
#endif
typedef SG_u64_TYPE SG_u64;
#ifndef SG_i8_TYPE
# error Definition for SG_i8_TYPE was not provided
#endif
typedef SG_i8_TYPE SG_i8;
#ifndef SG_i16_TYPE
# error Definition for SG_i16_TYPE was not provided
#endif
typedef SG_i16_TYPE SG_i16;
#ifndef SG_i32_TYPE
# error Definition for SG_i32_TYPE was not provided
#endif
typedef SG_i32_TYPE SG_i32;
#ifndef SG_i64_TYPE
# error Definition for SG_i64_TYPE was not provided
#endif
typedef SG_i64_TYPE SG_i64;
#ifndef SG_f32_TYPE
# error Definition for SG_f32_TYPE was not provided
#endif
typedef SG_f32_TYPE SG_f32;
#ifndef SG_f64_TYPE
# error Definition for SG_f64_TYPE was not provided
#endif
typedef SG_f64_TYPE SG_f64;

// Generated Types
typedef SG_u8 MyEnumType;
typedef struct Vec3 Vec3;
typedef struct MyUnionType MyUnionType;
typedef SG_u8 MyUnionTypeKind;
typedef struct MyUnionType_Option1 MyUnionType_Option1;
typedef struct MyUnionType_Option2 MyUnionType_Option2;

enum
{
	MyEnumType_a,
	MyEnumType_b,
};

struct Vec3
{
	SG_f32 x;
	SG_f32 y;
	SG_f32 z;
};

enum
{
	MyUnionTypeKind_Option1,
	MyUnionTypeKind_Option2,
};

struct MyUnionType_Option1
{
	SG_f32 x;
	SG_f32 y;
	SG_f32 z;
};

struct MyUnionType_Option2
{
	Vec3 v;
};

struct MyUnionType
{
	MyUnionTypeKind kind;
	union
	{
		MyUnionType_Option1 option1;
		MyUnionType_Option2 option2;
	};
};

// Serialization Functions Declarations
static void SG_Serialize_u8(SG_Buffer *buffer, SG_u8 *s);
static void SG_Serialize_u16(SG_Buffer *buffer, SG_u16 *s);
static void SG_Serialize_u32(SG_Buffer *buffer, SG_u32 *s);
static void SG_Serialize_u64(SG_Buffer *buffer, SG_u64 *s);
static void SG_Serialize_i8(SG_Buffer *buffer, SG_i8 *s);
static void SG_Serialize_i16(SG_Buffer *buffer, SG_i16 *s);
static void SG_Serialize_i32(SG_Buffer *buffer, SG_i32 *s);
static void SG_Serialize_i64(SG_Buffer *buffer, SG_i64 *s);
static void SG_Serialize_f32(SG_Buffer *buffer, SG_f32 *s);
static void SG_Serialize_f64(SG_Buffer *buffer, SG_f64 *s);
static void SG_Serialize_MyEnumType(SG_Buffer *buffer, MyEnumType *s);
static void SG_Serialize_Vec3(SG_Buffer *buffer, Vec3 *s);
static void SG_Serialize_MyUnionTypeKind(SG_Buffer *buffer, MyUnionTypeKind *s);
static void SG_Serialize_MyUnionType_Option1(SG_Buffer *buffer, MyUnionType_Option1 *s);
static void SG_Serialize_MyUnionType_Option2(SG_Buffer *buffer, MyUnionType_Option2 *s);
static void SG_Serialize_MyUnionType(SG_Buffer *buffer, MyUnionType *s);

// Deserialization Functions Declarations
static void SG_Deserialize_u8(SG_Buffer *buffer, SG_u8 *s);
static void SG_Deserialize_u16(SG_Buffer *buffer, SG_u16 *s);
static void SG_Deserialize_u32(SG_Buffer *buffer, SG_u32 *s);
static void SG_Deserialize_u64(SG_Buffer *buffer, SG_u64 *s);
static void SG_Deserialize_i8(SG_Buffer *buffer, SG_i8 *s);
static void SG_Deserialize_i16(SG_Buffer *buffer, SG_i16 *s);
static void SG_Deserialize_i32(SG_Buffer *buffer, SG_i32 *s);
static void SG_Deserialize_i64(SG_Buffer *buffer, SG_i64 *s);
static void SG_Deserialize_f32(SG_Buffer *buffer, SG_f32 *s);
static void SG_Deserialize_f64(SG_Buffer *buffer, SG_f64 *s);
static void SG_Deserialize_MyEnumType(SG_Buffer *buffer, MyEnumType *s);
static void SG_Deserialize_Vec3(SG_Buffer *buffer, Vec3 *s);
static void SG_Deserialize_MyUnionTypeKind(SG_Buffer *buffer, MyUnionTypeKind *s);
static void SG_Deserialize_MyUnionType_Option1(SG_Buffer *buffer, MyUnionType_Option1 *s);
static void SG_Deserialize_MyUnionType_Option2(SG_Buffer *buffer, MyUnionType_Option2 *s);
static void SG_Deserialize_MyUnionType(SG_Buffer *buffer, MyUnionType *s);

#endif //TEST_META_H