#include <QDebug>

#include <math.h>

#include "scalarfield.h"


static const qreal isovalue = 1.0;
static const qreal epsilon = 0.001;
static const qreal step = 0.5;


ScalarField::ScalarField() :
    intersect_hit_(0),
    intersect_miss_(0),
    iterations_hit_(0),
    iterations_hit_min_(100),
    iterations_hit_max_(0),
    iterations_miss_(0),
    iterations_miss_min_(100),
    iterations_miss_max_(0) {
}


ScalarField::~ScalarField() {
    if (iterations_hit_) {
        qDebug() << __func__ << "hit" << iterations_hit_min_ << iterations_hit_ / intersect_hit_ << iterations_hit_max_;
    }
    if (iterations_miss_) {
        qDebug() << __func__ << "miss" << iterations_miss_min_ << iterations_miss_ / intersect_miss_ << iterations_miss_max_;
    }
}


bool ScalarField::intersect(const QVector3D &p1, const QVector3D &p2, QVector3D &i) const {
    QVector3D direction = p2 - p1;
    qreal length = direction.length();
    qreal walked = 0.0;
    direction /= length;    // direction = normalized vector

    // metrics
    int iterations = 0;

    QVector3D pos = p1;
    qreal delta;// = isovalue - valueAt(pos);
    while (abs(delta = (isovalue - valueAt(pos))) > epsilon && walked < length) {
        QVector3D gradient(gradientAt(pos));
        qreal gradval = abs(QVector3D::dotProduct(gradient, direction)); // gradient value projected on direction
        qreal disp = delta / gradval;
        if (abs(disp) > step) { // going too fast ?
            disp = signbit(disp) ? -step : step;
        }
        pos += disp * direction;
        walked += disp;

        iterations++;
    }

    if (walked < length) {
        i = pos;

        intersect_hit_++;
        iterations_hit_ += iterations;

        if (iterations > iterations_hit_max_)
            iterations_hit_max_ = iterations;

        if (iterations < iterations_hit_min_)
            iterations_hit_min_ = iterations;

        return true;
    }

    intersect_miss_++;
    iterations_miss_ += iterations;

    if (iterations > iterations_miss_max_)
        iterations_miss_max_ = iterations;

    if (iterations < iterations_miss_min_)
        iterations_miss_min_ = iterations;

    return false;
}
