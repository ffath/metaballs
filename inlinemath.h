#ifndef INLINEMATH_H
#define INLINEMATH_H

namespace inlinemath {

class Vector3D {
public:
    float x;
    float y;
    float z;

    Vector3D();
    Vector3D(float xx, float yy, float zz);
    Vector3D(const Vector3D &o);

    Vector3D &operator=(const Vector3D &o);

    Vector3D &operator+=(const Vector3D &o);
    Vector3D &operator-=(const Vector3D &o);
    Vector3D &operator*=(float t);
    Vector3D &operator/=(float t);

    float lengthSquared();
    float length();
    void normalize();

    static float dotProduct(const Vector3D &v1, const Vector3D &v2);
};

inline Vector3D::Vector3D() : x(0), y(0), z(0) {
}

inline Vector3D::Vector3D(float xx, float yy, float zz) : x(xx), y(yy), z(zz) {
}

inline Vector3D::Vector3D(const Vector3D &o) : x(o.x), y(o.y), z(o.z) {
}

inline Vector3D &Vector3D::operator=(const Vector3D &o) {
    x = o.x;
    y = o.y;
    z = o.z;
    return *this;
}

inline Vector3D &Vector3D::operator+=(const Vector3D &o) {
    x += o.x;
    y += o.y;
    z += o.z;
    return *this;
}

inline Vector3D &Vector3D::operator-=(const Vector3D &o) {
    x -= o.x;
    y -= o.y;
    z -= o.z;
    return *this;
}

inline Vector3D &Vector3D::operator*=(float t) {
    x *= t;
    y *= t;
    z *= t;
    return *this;
}

inline Vector3D &Vector3D::operator/=(float t) {
    x /= t;
    y /= t;
    z /= t;
    return *this;
}

inline const Vector3D operator+(const Vector3D &v1, const Vector3D &v2) {
    return Vector3D(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
}

inline const Vector3D operator-(const Vector3D &v1, const Vector3D &v2) {
    return Vector3D(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z);
}

inline const Vector3D operator*(const Vector3D &v, float t) {
    return Vector3D(v.x * t, v.y * t, v.z * t);
}

inline const Vector3D operator*(float t, const Vector3D &v) {
    return Vector3D(v.x * t, v.y * t, v.z * t);
}

inline const Vector3D operator/(const Vector3D &v, float t) {
    return Vector3D(v.x / t, v.y / t, v.z / t);
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
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

} // inlinemath
#endif // INLINEMATH_H
