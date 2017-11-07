#ifndef SCALARFIELD_H
#define SCALARFIELD_H
#include <QVector3D>


class ScalarField
{
public:
    ScalarField();
    virtual ~ScalarField();

    virtual qreal valueAt(const QVector3D &pos) const = 0;
    virtual QVector3D gradientAt(const QVector3D &pos) const = 0;

    bool intersect(const QVector3D &p1, const QVector3D &p2, QVector3D &i) const;

private:
    // metrics
    mutable int intersect_hit_;
    mutable int intersect_miss_;
    mutable int iterations_hit_;
    mutable int iterations_hit_min_;
    mutable int iterations_hit_max_;
    mutable int iterations_miss_;
    mutable int iterations_miss_min_;
    mutable int iterations_miss_max_;
};

#endif // SCALARFIELD_H
