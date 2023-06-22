#include <EclipsePCH.h>
#include "Vector.h"

#include <assert.h>
#include <math.h>

namespace Eclipse
{
namespace Math
{

/*-------------------------------------------
                Vec2
--		-----------------------------------------*/

Vec2::Vec2() : m_X(0), m_Y(0) {}

Vec2::Vec2(const float value) : m_X(value), m_Y(value) {}

Vec2::Vec2(const Vec2& rhs) : m_X(rhs.m_X), m_Y(rhs.m_Y) {}

Vec2::Vec2(float x, float y) : m_X(x), m_Y(y) {}

Vec2::Vec2(const float* xy) : m_X(xy[0]), m_Y(xy[1]) {}

Vec2& Vec2::operator=(const Vec2& rhs)
{
    if (this == &rhs) return *this;

    m_X = rhs.m_X;
    m_Y = rhs.m_Y;
    return *this;
}

bool Vec2::operator==(const Vec2& rhs) const
{
    return m_X == rhs.m_X && m_Y == rhs.m_Y;
}

bool Vec2::operator!=(const Vec2& rhs) const
{
    return (*this == rhs) ? false : true;
}

const Vec2& Vec2::operator+=(const Vec2& rhs)
{
    m_X += rhs.m_X;
    m_Y += rhs.m_Y;

    return *this;
}

const Vec2& Vec2::operator-=(const Vec2& rhs)
{
    m_X -= rhs.m_X;
    m_Y -= rhs.m_Y;

    return *this;
}

const Vec2& Vec2::operator*=(const float rhs)
{
    m_X *= rhs;
    m_Y *= rhs;

    return *this;
}

const Vec2& Vec2::operator/=(const float rhs)
{
    assert(rhs != 0 && "Zero division!");
    m_X /= rhs;
    m_Y /= rhs;

    return *this;
}

Vec2 Vec2::operator+(const Vec2& rhs) const
{
    Vec2 temp;
    temp.m_X = m_X + rhs.m_Y;
    temp.m_Y = m_Y + rhs.m_Y;

    return temp;
}

Vec2 Vec2::operator-(const Vec2& rhs) const
{
    Vec2 temp;
    temp.m_X = m_X - rhs.m_Y;
    temp.m_Y = m_Y - rhs.m_Y;

    return temp;
}

Vec2 Vec2::operator*(const float rhs) const
{
    Vec2 temp;
    temp.m_X = m_X * rhs;
    temp.m_Y = m_Y * rhs;

    return temp;
}

Vec2 Vec2::operator/(const float rhs) const
{
    assert(rhs != 0 && "Zero division!");

    Vec2 temp;
    temp.m_X = m_X / rhs;
    temp.m_Y = m_Y / rhs;

    return temp;
}

float Vec2::operator[](const int index) const
{
    assert(index >= 0 && index < 2 && "Index should be in range: [0,1]");
    return (&m_X)[index];
}

float& Vec2::operator[](const int index)
{
    assert(index >= 0 && index < 2 && "Index should be in range: [0,1]");
    return (&m_X)[index];
}

const Vec2& Vec2::Normalize()
{
    const float magnitude = GetMagnitude();

    assert(magnitude != 0 && "Zero divison!");
    const float invertMagnitude = 1.0f / magnitude;

    // Check NaN
    if (0.0f * invertMagnitude == 0.0f * invertMagnitude)
    {
        m_X *= invertMagnitude;
        m_Y *= invertMagnitude;
    }

    return *this;
}

float Vec2::GetMagnitude() const
{
    float magnitude = DotProduct(*this, *this);
    magnitude = sqrtf(magnitude);

    return magnitude;
}

bool Vec2::IsValid() const
{

    if (m_X * 0.0f != m_X * 0.0f || m_Y * 0.0f != m_Y * 0.0f) return false;

    return true;
}

/*-------------------------------------------
                Vec3
-------------------------------------------*/

Vec3::Vec3() : m_X(0), m_Y(0), m_Z(0) {}

Vec3::Vec3(const float value) : m_X(value), m_Y(value), m_Z(value) {}

Vec3::Vec3(const Vec3& rhs) : m_X(rhs.m_X), m_Y(rhs.m_Y), m_Z(rhs.m_Z) {}

Vec3::Vec3(float x, float y, float z) : m_X(x), m_Y(y), m_Z(z) {}

Vec3::Vec3(const float* xyz) : m_X(xyz[0]), m_Y(xyz[1]), m_Z(xyz[2]) {}

Vec3& Vec3::operator=(const Vec3& rhs)
{
    m_X = rhs.m_X;
    m_Y = rhs.m_Y;
    m_Z = rhs.m_Z;

    return *this;
}

Vec3& Vec3::operator=(const float* rhs)
{
    m_X = rhs[0];
    m_Y = rhs[1];
    m_Z = rhs[2];

    return *this;
}

bool Vec3::operator==(const Vec3& rhs) const
{
    return m_X == rhs.m_X && m_Y == rhs.m_Y && m_Z == rhs.m_Z;
}

bool Vec3::operator!=(const Vec3& rhs) const
{
    return (*this == rhs) ? false : true;
}

const Vec3& Vec3::operator+=(const Vec3& rhs)
{
    m_X += rhs.m_X;
    m_Y += rhs.m_Y;
    m_Z += rhs.m_Z;

    return *this;
}

const Vec3& Vec3::operator-=(const Vec3& rhs)
{
    m_X -= rhs.m_X;
    m_Y -= rhs.m_Y;
    m_Z -= rhs.m_Z;

    return *this;
}

const Vec3& Vec3::operator*=(const float rhs)
{
    m_X *= rhs;
    m_Y *= rhs;
    m_Z *= rhs;

    return *this;
}

const Vec3& Vec3::operator/=(const float rhs)
{
    assert(rhs != 0 && "Zero division!");

    m_X /= rhs;
    m_Y /= rhs;
    m_Z /= rhs;

    return *this;
}

Vec3 Vec3::operator+(const Vec3& rhs) const
{
    Vec3 temp;
    temp.m_X = m_X + rhs.m_X;
    temp.m_Y = m_Y + rhs.m_Y;
    temp.m_Z = m_Z + rhs.m_Z;

    return temp;
}

Vec3 Vec3::operator-(const Vec3& rhs) const
{
    Vec3 temp;
    temp.m_X = m_X + rhs.m_X;
    temp.m_Y = m_Y + rhs.m_Y;
    temp.m_Z = m_Z + rhs.m_Z;

    return temp;
}

Vec3 Vec3::operator*(const float rhs) const
{
    Vec3 temp;
    temp.m_X = m_X * rhs;
    temp.m_Y = m_Y * rhs;
    temp.m_Z = m_Z * rhs;

    return temp;
}

Vec3 Vec3::operator/(const float rhs) const
{
    assert(rhs != 0 && "Zero division!");

    Vec3 temp;
    temp.m_X = m_X / rhs;
    temp.m_Y = m_Y / rhs;
    temp.m_Z = m_Z / rhs;

    return temp;
}

float Vec3::operator[](const int index) const
{
    assert(index >= 0 && index <= 2 && "Index should be in range: [0,2]");
    return (&m_X)[index];
    return 0.0f;
}

float& Vec3::operator[](const int index)
{
    assert(index >= 0 && index <= 2 && "Index should be in range: [0,2]");
    return (&m_X)[index];
}

const Vec3& Vec3::Normalize()
{
    const float magnitude = GetMagnitude();

    assert(magnitude != 0 && "Zero divison!");
    const float invertMagnitude = 1.0f / magnitude;

    if (0.0f * invertMagnitude == 0.0f * invertMagnitude)
    {
        m_X *= invertMagnitude;
        m_Y *= invertMagnitude;
        m_Z *= invertMagnitude;
    }

    return *this;
}

float Vec3::GetMagnitude() const
{
    float magnitude = DotProduct(*this, *this);
    magnitude = sqrtf(magnitude);

    return magnitude;
}

bool Vec3::IsValid() const
{
    if (m_X * 0.0f != m_X * 0.0f || m_Y * 0.0f != m_Y * 0.0f || m_Z * 0.0f != m_Z * 0.0f) return false;

    return true;
}

// TODO: Figure out how it works.
void Vec3::GetOrtho(Vec3& u, Vec3& v) const
{
    Vec3 n = *this;
    n.Normalize();

    const Vec3 w = (n.m_Z * n.m_Z > 0.9f * 0.9f) ? Vec3(1, 0, 0) : Vec3(0, 0, 1);
    u = Cross(w, n).Normalize();

    v = Cross(n, u).Normalize();
    u = Cross(v, n).Normalize();
}

/*-------------------------------------------
                Vec4
-------------------------------------------*/

Vec4::Vec4() : m_X(0), m_Y(0), m_Z(0), m_W(0) {}

Vec4::Vec4(const float value) : m_X(value), m_Y(value), m_Z(value), m_W(value) {}

Vec4::Vec4(const Vec4& rhs) : m_X(rhs.m_X), m_Y(rhs.m_Y), m_Z(rhs.m_Z), m_W(rhs.m_W) {}

Vec4::Vec4(float x, float y, float z, float w) : m_X(x), m_Y(y), m_Z(z), m_W(w) {}

Vec4::Vec4(const float* xyzw) : m_X(xyzw[0]), m_Y(xyzw[1]), m_Z(xyzw[2]), m_W(xyzw[3]) {}

Vec4& Vec4::operator=(const Vec4& rhs)
{
    m_X = rhs.m_X;
    m_Y = rhs.m_Y;
    m_Z = rhs.m_Z;
    m_W = rhs.m_W;

    return *this;
}

Vec4& Vec4::operator=(const float* rhs)
{
    m_X = rhs[0];
    m_Y = rhs[1];
    m_Z = rhs[2];
    m_W = rhs[3];

    return *this;
}

bool Vec4::operator==(const Vec4& rhs) const
{
    return (m_X == rhs.m_X) && (m_Y == rhs.m_Y) && (m_Z == rhs.m_Z) && (m_W == rhs.m_W);
}

bool Vec4::operator!=(const Vec4& rhs) const
{
    return (*this == rhs) ? false : true;
}

const Vec4& Vec4::operator+=(const Vec4& rhs)
{
    m_X += rhs.m_X;
    m_Y += rhs.m_Y;
    m_Z += rhs.m_Z;
    m_W += rhs.m_W;

    return *this;
}

const Vec4& Vec4::operator-=(const Vec4& rhs)
{
    m_X -= rhs.m_X;
    m_Y -= rhs.m_Y;
    m_Z -= rhs.m_Z;
    m_W -= rhs.m_W;

    return *this;
}

const Vec4& Vec4::operator*=(const float rhs)
{
    m_X *= rhs;
    m_Y *= rhs;
    m_Z *= rhs;
    m_W *= rhs;

    return *this;
}

const Vec4& Vec4::operator/=(const float rhs)
{
    assert(rhs != 0 && "Zero division!");

    m_X /= rhs;
    m_Y /= rhs;
    m_Z /= rhs;
    m_W /= rhs;

    return *this;
}

Vec4 Vec4::operator+(const Vec4& rhs) const
{
    Vec4 temp;
    temp.m_X = m_X + rhs.m_X;
    temp.m_Y = m_Y + rhs.m_Y;
    temp.m_Z = m_Z + rhs.m_Z;
    temp.m_W = m_W + rhs.m_W;

    return temp;
}

Vec4 Vec4::operator-(const Vec4& rhs) const
{
    Vec4 temp;
    temp.m_X = m_X - rhs.m_X;
    temp.m_Y = m_Y - rhs.m_Y;
    temp.m_Z = m_Z - rhs.m_Z;
    temp.m_W = m_W - rhs.m_W;

    return temp;
}

Vec4 Vec4::operator*(const float rhs) const
{
    Vec4 temp;
    temp.m_X = m_X * rhs;
    temp.m_Y = m_Y * rhs;
    temp.m_Z = m_Z * rhs;
    temp.m_W = m_W * rhs;

    return temp;
}

Vec4 Vec4::operator/(const float rhs) const
{
    assert(rhs != 0 && "Zero divison!");

    Vec4 temp;
    temp.m_X = m_X / rhs;
    temp.m_Y = m_Y / rhs;
    temp.m_Z = m_Z / rhs;
    temp.m_W = m_W / rhs;

    return temp;
}

float Vec4::operator[](const int index) const
{
    assert(index >= 0 && index <= 3 && "Index should be in range: [0,3]");
    return (&m_X)[index];
}

float& Vec4::operator[](const int index)
{
    assert(index >= 0 && index <= 3 && "Index should be in range: [0,3]");
    return (&m_X)[index];
}

const Vec4& Vec4::Normalize()
{
    float magnitude = GetMagnitude();
    magnitude = sqrtf(magnitude);

    assert(false);
    return *this; /*magnitude*/
}

float Vec4::GetMagnitude() const
{
    return DotProduct(*this, *this);
}

bool Vec4::IsValid() const
{
    if (m_X * 0.0f != m_X * 0.0f || m_Y * 0.0f != m_Y * 0.0f || m_Z * 0.0f != m_Z * 0.0f || m_W * 0.0f != m_W * 0.0f) return false;

    return true;
}

}  // namespace Math
}  // namespace Eclipse
