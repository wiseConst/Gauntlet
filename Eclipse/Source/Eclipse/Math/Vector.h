#pragma once

#include <Eclipse/Core/Core.h>

namespace Eclipse
{

	namespace Math
	{
		/*-------------------------------------------
			Vec2
		-------------------------------------------*/
		class Vec2
		{
		public:
			Vec2();
			Vec2(const float value);
			Vec2(const Vec2& rhs);
			Vec2(float x, float y);
			Vec2(const float* xy);
			Vec2& operator=(const Vec2& rhs);

			bool operator==(const Vec2& rhs) const;
			bool operator!=(const Vec2& rhs) const;

			const Vec2& operator+=(const Vec2& rhs);
			const Vec2& operator-=(const Vec2& rhs);
			const Vec2& operator*=(const float rhs);
			const Vec2& operator/=(const float rhs);

			Vec2 operator+(const Vec2& rhs) const;
			Vec2 operator-(const Vec2& rhs) const;
			Vec2 operator*(const float rhs) const;
			Vec2 operator/(const float rhs) const;

			float operator[](const int index) const;
			float& operator[](const int index);

			const Vec2& Normalize();
			float GetMagnitude() const;
			bool IsValid() const;

			const float* ToPtr() const { return (&m_X); }

			FORCEINLINE void Zero() { *this = ZeroVector; }

			static const Vec2 ZeroVector;

			static FORCEINLINE float DotProduct(const Vec2& lhs, const Vec2& rhs)
			{
				return lhs.m_X * rhs.m_X + lhs.m_Y * rhs.m_Y;
			}

			static FORCEINLINE float CrossProduct(const Vec2& lhs, const Vec2& rhs)
			{
				return lhs.m_X * rhs.m_Y - lhs.m_Y * rhs.m_X;
			}

			static FORCEINLINE Vec2 Cross(const Vec2& lhs, const Vec2& rhs)
			{
				Vec2 temp;
				temp.m_X = (lhs.m_X * rhs.m_Y);
				temp.m_Y = (lhs.m_Y * rhs.m_X);

				return temp;
			}

			float m_X{ 0 };
			float m_Y{ 0 };
		};

		/*-------------------------------------------
			Vec3
		-------------------------------------------*/
		class Vec3
		{
		public:
			Vec3();
			Vec3(const float value);
			Vec3(const Vec3& rhs);
			Vec3(float x, float y, float z);
			Vec3(const float* xyz);
			Vec3& operator=(const Vec3& rhs);
			Vec3& operator=(const float* rhs);

			bool operator==(const Vec3& rhs) const;
			bool operator!=(const Vec3& rhs) const;

			const Vec3& operator+=(const Vec3& rhs);
			const Vec3& operator-=(const Vec3& rhs);
			const Vec3& operator*=(const float rhs);
			const Vec3& operator/=(const float rhs);

			Vec3 operator+(const Vec3& rhs) const;
			Vec3 operator-(const Vec3& rhs) const;
			Vec3 operator*(const float rhs) const;
			Vec3 operator/(const float rhs) const;

			float operator[](const int index) const;
			float& operator[](const int index);

			const Vec3& Normalize();
			float GetMagnitude() const;
			FORCEINLINE float GetLengthSquared() const { return DotProduct(*this, *this); }
			bool IsValid() const;
			void GetOrtho(Vec3& u, Vec3& v) const;

			FORCEINLINE void Zero() { *this = ZeroVector; }
			const float* ToPtr() const { return (&m_X); }

			static const Vec3 ZeroVector;

			static FORCEINLINE float DotProduct(const Vec3& lhs, const Vec3& rhs)
			{
				return lhs.m_X * rhs.m_X + lhs.m_Y * rhs.m_Y + lhs.m_Z * rhs.m_Z;
			}

			static FORCEINLINE float CrossProduct(const Vec3& lhs, const Vec3& rhs)
			{
				return (lhs.m_Y * rhs.m_Z - lhs.m_Z * rhs.m_Y) - (lhs.m_X * rhs.m_Z - lhs.m_Z * rhs.m_X) + (lhs.m_X * rhs.m_Y - lhs.m_Y * rhs.m_X);
			}

			static FORCEINLINE Vec3 Cross(const Vec3& lhs, const Vec3& rhs)
			{
				Vec3 temp;
				temp.m_X = (lhs.m_Y * rhs.m_Z) - (rhs.m_Y * lhs.m_Z);
				temp.m_Y = (lhs.m_X * rhs.m_Z) - (rhs.m_X * lhs.m_Z);
				temp.m_Z = (lhs.m_X * rhs.m_Y) - (rhs.m_X * lhs.m_Y);

				return temp;
			}

			float m_X{ 0 };
			float m_Y{ 0 };
			float m_Z{ 0 };
		};

		/*-------------------------------------------
			Vec4
		-------------------------------------------*/

		class Vec4
		{
		public:
			Vec4();
			Vec4(const float value);
			Vec4(const Vec4& rhs);
			Vec4(float x, float y, float z, float w);
			Vec4(const float* xyzw);
			Vec4& operator=(const Vec4& rhs);
			Vec4& operator=(const float* rhs);

			bool operator==(const Vec4& rhs) const;
			bool operator!=(const Vec4& rhs) const;

			const Vec4& operator+=(const Vec4& rhs);
			const Vec4& operator-=(const Vec4& rhs);
			const Vec4& operator*=(const float rhs);
			const Vec4& operator/=(const float rhs);

			Vec4 operator+(const Vec4& rhs) const;
			Vec4 operator-(const Vec4& rhs) const;
			Vec4 operator*(const float rhs) const;
			Vec4 operator/(const float rhs) const;

			float operator[](const int index) const;
			float& operator[](const int index);

			const Vec4& Normalize();
			float GetMagnitude() const;
			FORCEINLINE float GetLengthSquared() const { return DotProduct(*this, *this); }
			bool IsValid() const;

			FORCEINLINE void Zero() { *this = ZeroVector; }

			const float* ToPtr() const { return (&m_X); }
			float* ToPtr() { return (&m_X); }

			static const Vec4 ZeroVector;

			static FORCEINLINE float DotProduct(const Vec4& lhs, const Vec4& rhs)
			{
				return lhs.m_X * rhs.m_X + lhs.m_Y * rhs.m_Y + lhs.m_Z * rhs.m_Z + lhs.m_W * rhs.m_W;
			}

			// TODO: Cross && CrossProduct && Ortho
			float m_X{ 0 };
			float m_Y{ 0 };
			float m_Z{ 0 };
			float m_W{ 0 };
		};
	}

}