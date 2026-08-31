#pragma once

#ifdef _MSC_VER
#define m128_f32(v)  ((v).m128_f32)
#define m128i_i32(v) ((v).m128i_i32)
#else
#define m128_f32(v)  ((float*)&(v))
#define m128i_i32(v) ((int*)&(v))
#endif
