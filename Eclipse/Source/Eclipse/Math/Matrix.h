#pragma once

#include <Eclipse/Core/Core.h>
#include "Vector.h"

namespace Eclipse
{

namespace Math
{
/*
================================
            Mat2
================================
*/
class Mat2
{
  public:
    Mat2() {}
    Mat2(const Mat2& rhs);
    Mat2(const float* mat);
    Mat2(const Vec2& row0, const Vec2& row1);
    Mat2& operator=(const Mat2& rhs);

    const Mat2& operator*=(const float rhs);
    const Mat2& operator+=(const Mat2& rhs);

    FORCEINLINE float Determinant() const { return rows[0].m_X * rows[1].m_Y - rows[0].m_Y * rows[1].m_X; }

  public:
    Vec2 rows[2];
};

/*
================================
            Mat3
================================
*/
class Mat3
{
  public:
    Mat3() {}
    Mat3(const Mat3& rhs);
    Mat3(const float* mat);
    Mat3(const Vec3& row0, const Vec3& row1, const Vec3& row2);
    Mat3& operator=(const Mat3& rhs);

    void Zero();
    void Identity();

    float Trace() const;
    float Determinant() const;
    Mat3 Transpose() const;
    Mat3 Inverse() const;
    Mat2 Minor(const int i, const int j) const;
    float Cofactor(const int i, const int j) const;

    Vec3 operator*(const Vec3& rhs) const;
    Mat3 operator*(const float rhs) const;
    Mat3 operator*(const Mat3& rhs) const;
    Mat3 operator+(const Mat3& rhs) const;

    const Mat3& operator+=(const float rhs);
    const Mat3& operator+=(const Mat3& rhs);

  public:
    Vec3 rows[3];
};

/*
================================
            Mat4
================================
*/
class Mat4
{
  public:
    Mat4() {}

  public:
    Vec3 rows[4];
};
}  // namespace Math

}  // namespace Eclipse
