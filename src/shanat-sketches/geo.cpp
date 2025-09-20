#include "geo.h"

#include <math.h>

float Vec3::length() const
{
    return sqrt(vals[0] * vals[0] + vals[1] * vals[1] + vals[2] * vals[2]);
}

void Vec3::set(float x, float y, float z)
{
    vals[0] = x;
    vals[1] = y;
    vals[2] = z;
}

Vec3 normalize(const Vec3 &v)
{
    float len = v.length();
    Vec3 res;
    if (len == 0) return res;
    res.vals[0] = v.vals[0] / len;
    res.vals[1] = v.vals[1] / len;
    res.vals[2] = v.vals[2] / len;
    return res;
}

Vec3 subtract(const Vec3 &a, const Vec3 &b)
{
    Vec3 res;
    res.vals[0] = a.vals[0] - b.vals[0];
    res.vals[1] = a.vals[1] - b.vals[1];
    res.vals[2] = a.vals[2] - b.vals[2];
    return res;
}

Vec3 cross(const Vec3 &a, const Vec3 &b)
{
    Vec3 res;
    res.vals[0] = a.vals[1] * b.vals[2] - a.vals[2] * b.vals[1];
    res.vals[1] = a.vals[2] * b.vals[0] - a.vals[0] * b.vals[2];
    res.vals[2] = a.vals[0] * b.vals[1] - a.vals[1] * b.vals[0];
    return res;
}

float dot(const Vec3 &a, const Vec3 &b)
{
    return a.vals[0] * b.vals[0] + a.vals[1] * b.vals[1] + a.vals[2] * b.vals[2];
}

void lookAt(const Vec3 &eye, const Vec3 &target, const Vec3 &up, Mat4 &mat)
{
    const Vec3 zAxis = normalize(subtract(eye, target));
    const Vec3 xAxis = normalize(cross(up, zAxis));
    const Vec3 yAxis = cross(zAxis, xAxis);

    mat.vals[0] = xAxis.vals[0];
    mat.vals[1] = yAxis.vals[0];
    mat.vals[2] = zAxis.vals[0];
    mat.vals[3] = 0;

    mat.vals[4] = xAxis.vals[1];
    mat.vals[5] = yAxis.vals[1];
    mat.vals[6] = zAxis.vals[1];
    mat.vals[7] = 0;

    mat.vals[8] = xAxis.vals[2];
    mat.vals[9] = yAxis.vals[2];
    mat.vals[10] = zAxis.vals[2];
    mat.vals[11] = 0;

    mat.vals[12] = -dot(xAxis, eye);
    mat.vals[13] = -dot(yAxis, eye);
    mat.vals[14] = -dot(zAxis, eye);
    mat.vals[15] = 1;
}

void perspective(float fov, float aspect, float near, float far, Mat4 &mat)
{
    // const float f = tan(M_PI * 0.5 - 0.5 * fov);
    const float fovRadians = fov * M_PI / 180.0f;
    const float f = 1.0f / tan(fovRadians * 0.5f);
    const float rangeInv = 1.0 / (near - far);

    mat.vals[0] = f / aspect;
    mat.vals[1] = 0;
    mat.vals[2] = 0;
    mat.vals[3] = 0;

    mat.vals[4] = 0;
    mat.vals[5] = f;
    mat.vals[6] = 0;
    mat.vals[7] = 0;

    mat.vals[8] = 0;
    mat.vals[9] = 0;
    mat.vals[10] = (near + far) * rangeInv;
    mat.vals[11] = -1;

    mat.vals[12] = 0;
    mat.vals[13] = 0;
    mat.vals[14] = near * far * rangeInv * 2;
    mat.vals[15] = 0;
}
