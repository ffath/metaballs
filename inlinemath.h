#ifndef INLINEMATH_H
#define INLINEMATH_H
#include <QVector3D>

namespace inlinemath {

static inline float dotProduct(const QVector3D &v1, const QVector3D &v2) {
    return v1.x() * v2.x() + v1.y() * v2.y() + v1.z() * v2.z();
}

static inline float lengthSquared(const QVector3D &v) {
    return dotProduct(v, v);
}

static inline float length(const QVector3D &v) {
    return std::sqrt(lengthSquared(v));
}

static inline void normalize(QVector3D &v) {
    v /= length(v);
}

} // inlinemath
#endif // INLINEMATH_H
