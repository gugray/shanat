#ifndef GEO_H
#define GEO_H

struct Vec3
{
    float vals[3] = {0};
    void set(float x, float y, float z);
    float length() const;
};

struct Mat4
{
    float vals[16] = {0};
};

Vec3 normalize(const Vec3 &v);
Vec3 subtract(const Vec3 &a, const Vec3 &b);
Vec3 cross(const Vec3 &a, const Vec3 &b);
float dot(const Vec3 &a, const Vec3 &b);

void lookAt(const Vec3 &eye, const Vec3 &target, const Vec3 &up, Mat4 &mat);
void perspective(float fov, float aspect, float near, float far, Mat4 mat);

#endif
