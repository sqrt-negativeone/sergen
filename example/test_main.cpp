
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define MemoryCopy(d,s,z)   memcpy(d,s,z)
#define Assert(c) if (!(c)) { *(volatile int *)0 = 0; }
#include <stdint.h>
typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float    f32;
typedef double   f64;

struct MyBuffer
{
    void *data;
    u64 pos;
    u64 max_size;
};

MyBuffer
MakeBuffer(void *data, u64 max_size)
{
    MyBuffer result;
    result.data = data;
    result.pos = 0;
    result.max_size = max_size;
    return result;
}

void
ResetBufferPos(MyBuffer *buffer)
{
    buffer->pos = 0;
}

void
WriteToBuffer(MyBuffer *buffer, void *src, u64 len)
{
    Assert(buffer->pos + len < buffer->max_size);
    char *dst = (char *)buffer->data + buffer->pos;
    MemoryCopy(dst,src,len);
    buffer->pos += len;
}

void
ReadFromBuffer(MyBuffer *buffer, void *dst, u64 len)
{
    char *src = (char *)buffer->data + buffer->pos;
    MemoryCopy(dst,src,len);
    buffer->pos += len;
}

u64
BufferFreeSize(MyBuffer *buffer)
{
    u64 result = buffer->max_size - buffer->pos;
    return result;
}

u64
BufferSize(MyBuffer *buffer)
{
    u64 result = buffer->max_size - buffer->pos;
    return result;
}

#define SG_BUFFER_TYPE struct MyBuffer
#define SG_READ_FROM_BUFFER(buffer, dst, nbytes) ReadFromBuffer(buffer, dst, nbytes)
#define SG_WRITE_TO_BUFFER(buffer, src, nbytes)  WriteToBuffer(buffer, src, nbytes)
#define SG_BUFFER_FREE_SIZE(buffer)              BufferFreeSize(buffer)
#define SG_BUFFER_SIZE(buffer)                   BufferSize(buffer)

// Base Types
#define SG_u8_TYPE u8
#define SG_u16_TYPE u16
#define SG_u32_TYPE u32
#define SG_u64_TYPE u64
#define SG_i8_TYPE i8
#define SG_i16_TYPE i16
#define SG_i32_TYPE i32
#define SG_i64_TYPE i64
#define SG_f32_TYPE f32
#define SG_f64_TYPE f64

#include "generated/test.meta.h"
#include "generated/test.meta.c"

int main()
{
    char buf[512];
    MyBuffer buffer = MakeBuffer(buf, sizeof(buf));
    MyUnionType my_union = {};
    my_union.kind = MyUnionTypeKind_Option1;
    my_union.option1.x = 1;
    my_union.option1.y = 2;
    my_union.option1.z = 3;
    printf("my_union\n\tkind: %d\n\tx: %f\n\ty: %f\n\tz: %f\n", 
           my_union.kind,
           my_union.option1.x,
           my_union.option1.y,
           my_union.option1.z);
    
    SG_Serialize_MyUnionType(&buffer, &my_union);
    
    ResetBufferPos(&buffer);
    MyUnionType my_other_union = {};
    SG_Deserialize_MyUnionType(&buffer, &my_other_union);
    Assert(my_other_union.kind == MyUnionTypeKind_Option1);
    Assert(my_other_union.option1.x == 1);
    Assert(my_other_union.option1.y == 2);
    Assert(my_other_union.option1.z == 3);
    printf("my_other_union\n\tkind: %d\n\tx: %f\n\ty: %f\n\tz: %f\n", 
           my_other_union.kind,
           my_other_union.option1.x,
           my_other_union.option1.y,
           my_other_union.option1.z);
    
    printf("test passed");
}