// Hash without Sine
// MIT License...
/* Copyright (c)2014 David Hoskins.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.*/

#ifndef HASH
#define HASH

//----------------------------------------------------------------------------------------
//  1 out, 1 in...
float Hash11(float p)
{
    p = frac(p * 0.1031f);
    p *= p + 33.33f;
    p *= p + p;
    return frac(p);
}

//----------------------------------------------------------------------------------------
//  1 out, 2 in...
float Hash12(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * 0.1031f);
    p3 += dot(p3, p3.yzx + 33.33f);
    return frac((p3.x + p3.y) * p3.z);
}

//----------------------------------------------------------------------------------------
//  1 out, 3 in...
float Hash13(float3 p3)
{
    p3 = frac(p3 * 0.1031f);
    p3 += dot(p3, p3.zyx + 31.32f);
    return frac((p3.x + p3.y) * p3.z);
}
//----------------------------------------------------------------------------------------
// 1 out 4 in...
float Hash14(float4 p4)
{
    p4 = frac(p4 * float4(0.1031f, 0.1030f, 0.0973f, 0.1099f));
    p4 += dot(p4, p4.wzxy + 33.33f);
    return frac((p4.x + p4.y) * (p4.z + p4.w));
}

//----------------------------------------------------------------------------------------
//  2 out, 1 in...
float2 Hash21(float p)
{
    float3 p3 = frac(float3(p, p, p) * float3(0.1031f, 0.1030f, 0.0973f));
    p3 += dot(p3, p3.yzx + 33.33f);
    return frac((p3.xx + p3.yz) * p3.zy);

}

//----------------------------------------------------------------------------------------
///  2 out, 2 in...
float2 Hash22(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * float3(0.1031f, 0.1030f, 0.0973f));
    p3 += dot(p3, p3.yzx + 33.33f);
    return frac((p3.xx + p3.yz) * p3.zy);

}

//----------------------------------------------------------------------------------------
///  2 out, 3 in...
float2 Hash23(float3 p3)
{
    p3 = frac(p3 * float3(0.1031f, 0.1030f, 0.0973f));
    p3 += dot(p3, p3.yzx + 33.33f);
    return frac((p3.xx + p3.yz) * p3.zy);
}

//----------------------------------------------------------------------------------------
//  3 out, 1 in...
float3 Hash31(float p)
{
    float3 p3 = frac(float3(p, p, p) * float3(0.1031f, 0.1030f, 0.0973f));
    p3 += dot(p3, p3.yzx + 33.33f);
    return frac((p3.xxy + p3.yzz) * p3.zyx);
}


//----------------------------------------------------------------------------------------
///  3 out, 2 in...
float3 Hash32(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * float3(0.1031f, 0.1030f, 0.0973f));
    p3 += dot(p3, p3.yxz + 33.33f);
    return frac((p3.xxy + p3.yzz) * p3.zyx);
}

//----------------------------------------------------------------------------------------
///  3 out, 3 in...
float3 Hash33(float3 p3)
{
    p3 = frac(p3 * float3(0.1031f, 0.1030f, 0.0973f));
    p3 += dot(p3, p3.yxz + 33.33f);
    return frac((p3.xxy + p3.yxx) * p3.zyx);
}

//----------------------------------------------------------------------------------------
// 4 out, 1 in...
float4 Hash41(float p)
{
    float4 p4 = frac(float4(p, p, p, p) * float4(0.1031f, 0.1030f, 0.0973f, 0.1099f));
    p4 += dot(p4, p4.wzxy + 33.33f);
    return frac((p4.xxyz + p4.yzzw) * p4.zywx);
    
}

//----------------------------------------------------------------------------------------
// 4 out, 2 in...
float4 Hash42(float2 p)
{
    float4 p4 = frac(float4(p.xyxy) * float4(0.1031f, 0.1030f, 0.0973f, 0.1099f));
    p4 += dot(p4, p4.wzxy + 33.33f);
    return frac((p4.xxyz + p4.yzzw) * p4.zywx);

}

//----------------------------------------------------------------------------------------
// 4 out, 3 in...
float4 Hash43(float3 p)
{
    float4 p4 = frac(float4(p.xyzx) * float4(0.1031f, 0.1030f, 0.0973f, 0.1099f));
    p4 += dot(p4, p4.wzxy + 33.33f);
    return frac((p4.xxyz + p4.yzzw) * p4.zywx);
}

//----------------------------------------------------------------------------------------
// 4 out, 4 in...
float4 Hash44(float4 p4)
{
    p4 = frac(p4 * float4(0.1031f, 0.1030f, 0.0973f, 0.1099f));
    p4 += dot(p4, p4.wzxy + 33.33f);
    return frac((p4.xxyz + p4.yzzw) * p4.zywx);
}

#endif
