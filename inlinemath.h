#ifndef INLINEMATH_H
#define INLINEMATH_H

namespace inlinemath {

template <class T>
class Vector3D {
public:
    typedef T Type;

    T x;
    T y;
    T z;

    Vector3D();
    Vector3D(T xx, T yy, T zz);
    Vector3D(const Vector3D &o);

    Vector3D &operator=(const Vector3D &o);

    Vector3D &operator+=(const Vector3D &o);
    Vector3D &operator-=(const Vector3D &o);
    Vector3D &operator*=(T t);
    Vector3D &operator/=(T t);

    T lengthSquared();
    T length();
    void normalize();

    static T dotProduct(const Vector3D &v1, const Vector3D &v2);
};

template <class T>
inline Vector3D<T>::Vector3D() : x(0), y(0), z(0) {
}

template <class T>
inline Vector3D<T>::Vector3D(T xx, T yy, T zz) : x(xx), y(yy), z(zz) {
}

template <class T>
inline Vector3D<T>::Vector3D(const Vector3D &o) : x(o.x), y(o.y), z(o.z) {
}

template <class T>
inline Vector3D<T> &Vector3D<T>::operator=(const Vector3D<T> &o) {
    x = o.x;
    y = o.y;
    z = o.z;
    return *this;
}

template <class T>
inline Vector3D<T> &Vector3D<T>::operator+=(const Vector3D<T> &o) {
    x += o.x;
    y += o.y;
    z += o.z;
    return *this;
}

template <class T>
inline Vector3D<T> &Vector3D<T>::operator-=(const Vector3D<T> &o) {
    x -= o.x;
    y -= o.y;
    z -= o.z;
    return *this;
}

template <class T>
inline Vector3D<T> &Vector3D<T>::operator*=(T t) {
    x *= t;
    y *= t;
    z *= t;
    return *this;
}

template <class T>
inline Vector3D<T> &Vector3D<T>::operator/=(T t) {
    x /= t;
    y /= t;
    z /= t;
    return *this;
}

template <class T>
inline const Vector3D<T> operator+(const Vector3D<T> &v1, const Vector3D<T> &v2) {
    return Vector3D<T>(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
}

template <class T>
inline const Vector3D<T> operator-(const Vector3D<T> &v1, const Vector3D<T> &v2) {
    return Vector3D<T>(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z);
}

template <class T>
inline const Vector3D<T> operator*(const Vector3D<T> &v, T t) {
    return Vector3D<T>(v.x * t, v.y * t, v.z * t);
}

template <class T>
inline const Vector3D<T> operator*(T t, const Vector3D<T> &v) {
    return Vector3D<T>(v.x * t, v.y * t, v.z * t);
}

template <class T>
inline const Vector3D<T> operator/(const Vector3D<T> &v, T t) {
    return Vector3D<T>(v.x / t, v.y / t, v.z / t);
}

template <class T>
inline T Vector3D<T>::lengthSquared() {
    return dotProduct(*this, *this);
}

template <class T>
inline T Vector3D<T>::length() {
    return std::sqrt(lengthSquared());
}

template <class T>
inline void Vector3D<T>::normalize() {
    *this /= length();
}

template <class T>
inline T Vector3D<T>::dotProduct(const Vector3D &v1, const Vector3D &v2) {
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}


typedef Vector3D<float> Vector3DF;

} // inlinemath
#endif // INLINEMATH_H
