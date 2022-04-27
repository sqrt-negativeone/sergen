
//
// Serialization Functions Definition
//

static void
SG_Serialize_u8(SG_Buffer *buffer, SG_u8 *s)
{
	SG_ASSERT(SG_BUFFER_FREE_SIZE(buffer) >= sizeof(SG_u8));
	SG_WRITE_TO_BUFFER(buffer, s, sizeof(SG_u8));
}

static void
SG_Serialize_u16(SG_Buffer *buffer, SG_u16 *s)
{
	SG_ASSERT(SG_BUFFER_FREE_SIZE(buffer) >= sizeof(SG_u16));
	SG_WRITE_TO_BUFFER(buffer, s, sizeof(SG_u16));
}

static void
SG_Serialize_u32(SG_Buffer *buffer, SG_u32 *s)
{
	SG_ASSERT(SG_BUFFER_FREE_SIZE(buffer) >= sizeof(SG_u32));
	SG_WRITE_TO_BUFFER(buffer, s, sizeof(SG_u32));
}

static void
SG_Serialize_u64(SG_Buffer *buffer, SG_u64 *s)
{
	SG_ASSERT(SG_BUFFER_FREE_SIZE(buffer) >= sizeof(SG_u64));
	SG_WRITE_TO_BUFFER(buffer, s, sizeof(SG_u64));
}

static void
SG_Serialize_i8(SG_Buffer *buffer, SG_i8 *s)
{
	SG_ASSERT(SG_BUFFER_FREE_SIZE(buffer) >= sizeof(SG_i8));
	SG_WRITE_TO_BUFFER(buffer, s, sizeof(SG_i8));
}

static void
SG_Serialize_i16(SG_Buffer *buffer, SG_i16 *s)
{
	SG_ASSERT(SG_BUFFER_FREE_SIZE(buffer) >= sizeof(SG_i16));
	SG_WRITE_TO_BUFFER(buffer, s, sizeof(SG_i16));
}

static void
SG_Serialize_i32(SG_Buffer *buffer, SG_i32 *s)
{
	SG_ASSERT(SG_BUFFER_FREE_SIZE(buffer) >= sizeof(SG_i32));
	SG_WRITE_TO_BUFFER(buffer, s, sizeof(SG_i32));
}

static void
SG_Serialize_i64(SG_Buffer *buffer, SG_i64 *s)
{
	SG_ASSERT(SG_BUFFER_FREE_SIZE(buffer) >= sizeof(SG_i64));
	SG_WRITE_TO_BUFFER(buffer, s, sizeof(SG_i64));
}

static void
SG_Serialize_f32(SG_Buffer *buffer, SG_f32 *s)
{
	SG_ASSERT(SG_BUFFER_FREE_SIZE(buffer) >= sizeof(SG_f32));
	SG_WRITE_TO_BUFFER(buffer, s, sizeof(SG_f32));
}

static void
SG_Serialize_f64(SG_Buffer *buffer, SG_f64 *s)
{
	SG_ASSERT(SG_BUFFER_FREE_SIZE(buffer) >= sizeof(SG_f64));
	SG_WRITE_TO_BUFFER(buffer, s, sizeof(SG_f64));
}

static void
SG_Serialize_MyEnumType(SG_Buffer *buffer, MyEnumType *s)
{
	SG_Serialize_u8(buffer, s);
}

static void
SG_Serialize_Vec3(SG_Buffer *buffer, Vec3 *s)
{
	SG_Serialize_f32(buffer, &s->x);
	SG_Serialize_f32(buffer, &s->y);
	SG_Serialize_f32(buffer, &s->z);
}

static void
SG_Serialize_MyUnionType(SG_Buffer *buffer, MyUnionType *s)
{
	SG_Serialize_MyUnionTypeKind(buffer, &s->kind);
	switch(s->kind)
	{
		case MyUnionTypeKind_Option1: SG_Serialize_MyUnionType_Option1(buffer, &s->my_union_type_option1); break;
		case MyUnionTypeKind_Option2: SG_Serialize_MyUnionType_Option2(buffer, &s->my_union_type_option2); break;
	}
}

static void
SG_Serialize_MyUnionTypeKind(SG_Buffer *buffer, MyUnionTypeKind *s)
{
	SG_Serialize_u8(buffer, s);
}

static void
SG_Serialize_MyUnionType_Option1(SG_Buffer *buffer, MyUnionType_Option1 *s)
{
	SG_Serialize_f32(buffer, &s->x);
	SG_Serialize_f32(buffer, &s->y);
	SG_Serialize_f32(buffer, &s->z);
}

static void
SG_Serialize_MyUnionType_Option2(SG_Buffer *buffer, MyUnionType_Option2 *s)
{
	SG_Serialize_Vec3(buffer, &s->v);
}


//
// Deserialization Functions Definition
//

static void
SG_Deserialize_u8(SG_Buffer *buffer, SG_u8 *s)
{
	SG_ASSERT(SG_BUFFER_SIZE(buffer) >= sizeof(SG_u8));
	SG_READ_FROM_BUFFER(buffer, s, sizeof(SG_u8));
}

static void
SG_Deserialize_u16(SG_Buffer *buffer, SG_u16 *s)
{
	SG_ASSERT(SG_BUFFER_SIZE(buffer) >= sizeof(SG_u16));
	SG_READ_FROM_BUFFER(buffer, s, sizeof(SG_u16));
}

static void
SG_Deserialize_u32(SG_Buffer *buffer, SG_u32 *s)
{
	SG_ASSERT(SG_BUFFER_SIZE(buffer) >= sizeof(SG_u32));
	SG_READ_FROM_BUFFER(buffer, s, sizeof(SG_u32));
}

static void
SG_Deserialize_u64(SG_Buffer *buffer, SG_u64 *s)
{
	SG_ASSERT(SG_BUFFER_SIZE(buffer) >= sizeof(SG_u64));
	SG_READ_FROM_BUFFER(buffer, s, sizeof(SG_u64));
}

static void
SG_Deserialize_i8(SG_Buffer *buffer, SG_i8 *s)
{
	SG_ASSERT(SG_BUFFER_SIZE(buffer) >= sizeof(SG_i8));
	SG_READ_FROM_BUFFER(buffer, s, sizeof(SG_i8));
}

static void
SG_Deserialize_i16(SG_Buffer *buffer, SG_i16 *s)
{
	SG_ASSERT(SG_BUFFER_SIZE(buffer) >= sizeof(SG_i16));
	SG_READ_FROM_BUFFER(buffer, s, sizeof(SG_i16));
}

static void
SG_Deserialize_i32(SG_Buffer *buffer, SG_i32 *s)
{
	SG_ASSERT(SG_BUFFER_SIZE(buffer) >= sizeof(SG_i32));
	SG_READ_FROM_BUFFER(buffer, s, sizeof(SG_i32));
}

static void
SG_Deserialize_i64(SG_Buffer *buffer, SG_i64 *s)
{
	SG_ASSERT(SG_BUFFER_SIZE(buffer) >= sizeof(SG_i64));
	SG_READ_FROM_BUFFER(buffer, s, sizeof(SG_i64));
}

static void
SG_Deserialize_f32(SG_Buffer *buffer, SG_f32 *s)
{
	SG_ASSERT(SG_BUFFER_SIZE(buffer) >= sizeof(SG_f32));
	SG_READ_FROM_BUFFER(buffer, s, sizeof(SG_f32));
}

static void
SG_Deserialize_f64(SG_Buffer *buffer, SG_f64 *s)
{
	SG_ASSERT(SG_BUFFER_SIZE(buffer) >= sizeof(SG_f64));
	SG_READ_FROM_BUFFER(buffer, s, sizeof(SG_f64));
}

static void
SG_Deserialize_MyEnumType(SG_Buffer *buffer, MyEnumType *s)
{
	SG_Deserialize_u8(buffer, s);
}

static void
SG_Deserialize_Vec3(SG_Buffer *buffer, Vec3 *s)
{
	SG_Deserialize_f32(buffer, &s->x);
	SG_Deserialize_f32(buffer, &s->y);
	SG_Deserialize_f32(buffer, &s->z);
}

static void
SG_Deserialize_MyUnionType(SG_Buffer *buffer, MyUnionType *s)
{
	SG_Deserialize_MyUnionTypeKind(buffer, &s->kind);
	switch(s->kind)
	{
		case MyUnionTypeKind_Option1: SG_Deserialize_MyUnionType_Option1(buffer, &s->my_union_type_option1); break;
		case MyUnionTypeKind_Option2: SG_Deserialize_MyUnionType_Option2(buffer, &s->my_union_type_option2); break;
	}
}

static void
SG_Deserialize_MyUnionTypeKind(SG_Buffer *buffer, MyUnionTypeKind *s)
{
	SG_Deserialize_u8(buffer, s);
}

static void
SG_Deserialize_MyUnionType_Option1(SG_Buffer *buffer, MyUnionType_Option1 *s)
{
	SG_Deserialize_f32(buffer, &s->x);
	SG_Deserialize_f32(buffer, &s->y);
	SG_Deserialize_f32(buffer, &s->z);
}

static void
SG_Deserialize_MyUnionType_Option2(SG_Buffer *buffer, MyUnionType_Option2 *s)
{
	SG_Deserialize_Vec3(buffer, &s->v);
}

