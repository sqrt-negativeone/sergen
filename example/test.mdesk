
@base u8
@base u16
@base u32
@base u64

@base i8
@base i16
@base i32
@base i64

@base f32
@base f64


@enum(base_type: u8) MyEnumType :
{
    MyEnumType_a,
    MyEnumType_b,
};

@struct Vec3 :
{
    x : f32;
    y : f32;
    z : f32;
};

@union(base_type: u8) MyUnionType :
{
    Option1: (x : f32, y : f32, z : f32),
    Option2: (v : Vec3),
};
