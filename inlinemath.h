#ifndef INLINEMATH_H
#define INLINEMATH_H


#include <emmintrin.h>

namespace inlinemath {

class Vector3D {
public:
    Vector3D();
    Vector3D(float xx, float yy, float zz);
    Vector3D(const Vector3D &o);

    float x();
    float y();
    float z();

    Vector3D &operator=(const Vector3D &o);

    Vector3D &operator+=(const Vector3D &o);
    Vector3D &operator-=(const Vector3D &o);
    Vector3D &operator*=(float t);
    Vector3D &operator*=(const Vector3D &o); // coord to coord multiply, not dot nor cross
    Vector3D &operator/=(float t);

    float lengthSquared();
    float length();
    void normalize();

    static float dotProduct(const Vector3D &v1, const Vector3D &v2);

private:
    __m128 m;

    Vector3D(const __m128 &m);

    friend inline const Vector3D operator+(const Vector3D &v1, const Vector3D &v2);
    friend inline const Vector3D operator-(const Vector3D &v1, const Vector3D &v2);
    friend inline const Vector3D operator*(const Vector3D &v, float t);
    friend inline const Vector3D operator*(float t, const Vector3D &v);
    friend inline const Vector3D operator*(const Vector3D &v1, const Vector3D &v2); // same as operator*(const Vector3D&)
    friend inline const Vector3D operator/(const Vector3D &v, float t);
};

inline Vector3D::Vector3D() : Vector3D(0.0f, 0.0f, 0.0f) {
}

inline Vector3D::Vector3D(const __m128 &mm) : m(mm) {}

inline Vector3D::Vector3D(float xx, float yy, float zz) : m(_mm_set_ps(xx, yy, zz, 0.0f)) {}

inline Vector3D::Vector3D(const Vector3D &o) : m(o.m) {
}

inline float Vector3D::x() {
    float result[4];
    _mm_store_ps(result, m);
    return result[3];
}

inline float Vector3D::y() {
    float result[4];
    _mm_store_ps(result, m);
    return result[2];
}

inline float Vector3D::z() {
    float result[4];
    _mm_store_ps(result, m);
    return result[1];
}

inline Vector3D &Vector3D::operator=(const Vector3D &o) {
    m = o.m;
    return *this;
}

inline Vector3D &Vector3D::operator+=(const Vector3D &o) {
    m = _mm_add_ps(m, o.m);
    return *this;
}

inline Vector3D &Vector3D::operator-=(const Vector3D &o) {
    m = _mm_sub_ps(m, o.m);
    return *this;
}

inline Vector3D &Vector3D::operator*=(float t) {
    __m128 a = _mm_set1_ps(t);
    m = _mm_mul_ps(m, a);
    return *this;
}

inline Vector3D &Vector3D::operator*=(const Vector3D &o) {
    m = _mm_mul_ps(m, o.m);
    return *this;
}

inline Vector3D &Vector3D::operator/=(float t) {
    __m128 a = _mm_set1_ps(t);
    m = _mm_div_ps(m, a);
    return *this;
}

inline const Vector3D operator+(const Vector3D &v1, const Vector3D &v2) {
    return Vector3D(_mm_add_ps(v1.m, v2.m));
}

inline const Vector3D operator-(const Vector3D &v1, const Vector3D &v2) {
    return Vector3D(_mm_sub_ps(v1.m, v2.m));
}

inline const Vector3D operator*(const Vector3D &v, float t) {
    __m128 a = _mm_set1_ps(t);
    return Vector3D(_mm_mul_ps(v.m, a));
}

inline const Vector3D operator*(float t, const Vector3D &v) {
    __m128 a = _mm_set1_ps(t);
    return Vector3D(_mm_mul_ps(v.m, a));
}

inline const Vector3D operator*(const Vector3D &v1, const Vector3D &v2) {
    return Vector3D(_mm_mul_ps(v1.m, v2.m));
}

inline const Vector3D operator/(const Vector3D &v, float t) {
    __m128 a = _mm_set1_ps(t);
    return Vector3D(_mm_div_ps(v.m, a));
}

inline float Vector3D::lengthSquared() {
    return dotProduct(*this, *this);
}

inline float Vector3D::length() {
    return std::sqrt(lengthSquared());
}

inline void Vector3D::normalize() {
    *this /= length();
}

inline float Vector3D::dotProduct(const Vector3D &v1, const Vector3D &v2) {
    float result;
    __m128 mul = _mm_mul_ps(v1.m, v2.m);
    __m128 shu = _mm_shuffle_ps(mul, mul, _MM_SHUFFLE(2, 3, 0, 1));
    __m128 sum = _mm_add_ps(mul, shu);
    shu = _mm_movehl_ps(shu, sum);
    sum = _mm_add_ss(sum, shu);
    _mm_store_ss(&result, sum);
    return result;
}

} // inlinemath
#endif // INLINEMATH_H
