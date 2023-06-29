#include <EclipsePCH.h>
#include "Matrix.h"

namespace Eclipse
{

namespace Math
{

/*
================================
            Mat2
================================
*/

Mat2::Mat2(const Mat2& rhs)
{
    rows[0] = rhs.rows[0];
    rows[1] = rhs.rows[1];
}

Mat2::Mat2(const float* mat)
{
    rows[0] = mat + 0;
    rows[1] = mat + 2;
}

Mat2::Mat2(const Vec2& row0, const Vec2& row1)
{
    rows[0] = row0;
    rows[1] = row1;
}

Mat2& Mat2::operator=(const Mat2& rhs)
{
    if (this == &rhs) return *this;

    rows[0] = rhs.rows[0];
    rows[1] = rhs.rows[1];

    return *this;
}

const Mat2& Mat2::operator*=(const float rhs)
{
    rows[0] *= rhs;
    rows[1] *= rhs;

    return *this;
}

const Mat2& Mat2::operator+=(const Mat2& rhs)
{
    rows[0] += rhs.rows[0];
    rows[1] += rhs.rows[1];

    return *this;
}

/*
================================
            Mat3
================================
*/

Mat3::Mat3(const Mat3& rhs)
{
    rows[0] = rhs.rows[0];
    rows[1] = rhs.rows[1];
    rows[2] = rhs.rows[2];
}

Mat3::Mat3(const float* mat)
{
    rows[0] = mat + 0;
    rows[1] = mat + 3;
    rows[2] = mat + 6;
}

Mat3::Mat3(const Vec3& row0, const Vec3& row1, const Vec3& row2)
{
    rows[0] = row0;
    rows[1] = row1;
    rows[2] = row2;
}

Mat3& Mat3::operator=(const Mat3& rhs)
{
    if (this == &rhs) return *this;

    rows[0] = rhs.rows[0];
    rows[1] = rhs.rows[1];
    rows[2] = rhs.rows[2];

    return *this;
}

void Mat3::Zero()
{
    rows[0].Zero();
    rows[1].Zero();
    rows[2].Zero();
}

void Mat3::Identity()
{
    rows[0] = Vec3(1, 0, 0);
    rows[1] = Vec3(0, 1, 0);
    rows[2] = Vec3(0, 0, 1);
}

float Mat3::Trace() const
{
    const float xx = rows[0][0] * rows[0][0];
    const float yy = rows[1][1] * rows[1][1];
    const float zz = rows[2][2] * rows[2][2];
    return xx + yy + zz;
}

float Mat3::Determinant() const
{
    const float i = rows[0][0] * (rows[1][1] * rows[2][2] - rows[1][2] * rows[2][1]);
    const float j = rows[0][1] * (rows[1][0] * rows[2][2] - rows[1][2] * rows[2][0]);
    const float k = rows[0][2] * (rows[1][0] * rows[2][1] - rows[1][1] * rows[2][0]);
    return i - j + k;
}

Mat3 Mat3::Transpose() const
{
    Mat3 Transposed;
    for (uint32_t i = 0; i < 3; ++i)
    {
        for (uint32_t j = 0; j < 3; ++j)
        {
            Transposed.rows[i][j] = rows[j][i];
        }
    }

    return Transposed;
}

Mat3 Mat3::Inverse() const
{
    Mat3 Inversed;
    for (uint32_t i = 0; i < 3; ++i)
    {
        for (uint32_t j = 0; j < 3; ++j)
        {
            Inversed.rows[j][i] = Cofactor(i, j);
        }
    }

    const float InvDet = 1.0f / Determinant();

    return Inversed * InvDet;
}

Mat2 Mat3::Minor(const int i, const int j) const
{
    Mat2 minor;
    int yy = 0;
    for (int y = 0; y < 3; ++y)
    {
        if (y == j)
        {
            continue;
        }
        int xx = 0;
        for (int x = 0; x < 3; ++x)
        {
            if (x == i)
            {
                continue;
            }
            minor.rows[xx][yy] = rows[x][y];
            xx++;
        }
        yy++;
    }

    return minor;
}

float Mat3::Cofactor(const int i, const int j) const
{
    const Mat2 minor = Minor(i, j);
    const float C = float(pow(-1, i + 1 + j + 1)) * minor.Determinant();
    return C;
}

Vec3 Mat3::operator*(const Vec3& rhs) const
{
    Vec3 tmp;
    tmp.m_X = Vec3::DotProduct(rows[0], rhs);
    tmp.m_Y = Vec3::DotProduct(rows[1], rhs);
    tmp.m_Z = Vec3::DotProduct(rows[2], rhs);
    return tmp;
}

Mat3 Mat3::operator*(const float rhs) const
{
    Mat3 tmp;
    tmp.rows[0] = rows[0] * rhs;
    tmp.rows[1] = rows[1] * rhs;
    tmp.rows[2] = rows[2] * rhs;

    return tmp;
}

Mat3 Mat3::operator*(const Mat3& rhs) const
{
    Mat3 tmp;
    for (int i = 0; i < 3; ++i)
    {
        tmp.rows[i].m_X = rows[i].m_X * rhs.rows[0].m_X + rows[i].m_Y * rhs.rows[1].m_X + rows[i].m_Z * rhs.rows[2].m_X;
        tmp.rows[i].m_Y = rows[i].m_X * rhs.rows[0].m_Y + rows[i].m_Y * rhs.rows[1].m_Y + rows[i].m_Z * rhs.rows[2].m_Y;
        tmp.rows[i].m_Z = rows[i].m_X * rhs.rows[0].m_Z + rows[i].m_Y * rhs.rows[1].m_Z + rows[i].m_Z * rhs.rows[2].m_Z;
    }

    return tmp;
}

Mat3 Mat3::operator+(const Mat3& rhs) const
{
    Mat3 tmp;
    tmp.rows[0] = rows[0] + rhs.rows[0];
    tmp.rows[1] = rows[1] + rhs.rows[1];
    tmp.rows[2] = rows[2] + rhs.rows[2];

    return tmp;
}

const Mat3& Mat3::operator+=(const float rhs)
{
    rows[0] += rhs;
    rows[1] += rhs;
    rows[2] += rhs;

    return *this;
}

const Mat3& Mat3::operator+=(const Mat3& rhs)
{
    rows[0] += rhs.rows[0];
    rows[1] += rhs.rows[1];
    rows[2] += rhs.rows[2];
    return *this;
}
}  // namespace Math

}  // namespace Eclipse