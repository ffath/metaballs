#ifndef SCALARFIELD_H
#define SCALARFIELD_H
#include <QDebug>

#include <math.h>
#include <time.h>

#include "inlinemath.h"

using namespace inlinemath;

//#define CLOCK_TIMINGS


template <class D, class T>
class ScalarField
{
public:
    typedef T FloatType;

    ScalarField() :
        intersect_hit_(0),
        intersect_miss_(0),
        intersect_elapsed_(0),
        intersect_elapsed_min_(0),
        intersect_elapsed_max_(0),
        iterations_hit_(0),
        iterations_hit_min_(100),
        iterations_hit_max_(0),
        iterations_miss_(0),
        iterations_miss_min_(100),
        iterations_miss_max_(0) {
    }

    virtual ~ScalarField() {
        if (iterations_hit_) {
            qDebug() << __func__ << "hit" << iterations_hit_min_ << iterations_hit_ / intersect_hit_ << iterations_hit_max_;
        }
        if (iterations_miss_) {
            qDebug() << __func__ << "miss" << iterations_miss_min_ << iterations_miss_ / intersect_miss_ << iterations_miss_max_;
        }
        if (intersect_hit_ || intersect_miss_) {
            qDebug() << __func__ << "elapsed" << intersect_elapsed_min_ << intersect_elapsed_ / (intersect_hit_ + intersect_miss_) << intersect_elapsed_max_ << intersect_elapsed_ / 1000000;
        }
    }

    inline T fieldAt(const Vector3D<T> &pos, Vector3D<T> &gradient) const {
        return static_cast<const D*>(this)->fieldAt(pos, gradient);
    }

    // (p, direction) -> starting point and normalized direction vector for the line to insect with
    // length -> max length to explore starting from p
    // i -> intersection if any
    // g -> gradient at i
    inline bool intersect(const Vector3D<T> &p, const Vector3D<T> &direction, T length, Vector3D<T> &i, Vector3D<T> &g) const {
#ifdef CLOCK_TIMINGS
        struct timespec t0, t;
        clock_gettime(CLOCK_REALTIME, &t0);
#endif
        T walked = 0.0;

        // metrics
        int iterations = 0;

        Vector3D<T> pos = p;
        Vector3D<T> gradient;
        T delta;
        while (std::abs(delta = (isovalue - fieldAt(pos, gradient))) > epsilon && walked < length && iterations < max_iterations) {
            T gradval = std::abs(Vector3D<T>::dotProduct(gradient, direction)); // gradient value projected on direction
            T disp = delta / gradval;
            if (std::abs(disp) > step) { // going too fast ?
                disp = signbit(disp) ? -step : step;
            }
            pos += disp * direction;
            walked += disp;

            iterations++;
        }

        if (walked < length && iterations < max_iterations) {
            i = pos;
            g = gradient;

            intersect_hit_++;
            iterations_hit_ += iterations;

            if (iterations > iterations_hit_max_)
                iterations_hit_max_ = iterations;

            if (iterations < iterations_hit_min_)
                iterations_hit_min_ = iterations;

            return true;
        }
#ifdef CLOCK_TIMINGS
        clock_gettime(CLOCK_REALTIME, &t);

        long long elapsed = (t.tv_sec - t0.tv_sec) * 1000000000 + (t.tv_nsec - t0.tv_nsec);
        intersect_elapsed_ += elapsed;
        if (elapsed > intersect_elapsed_max_)
            intersect_elapsed_max_ = elapsed;
        if (elapsed < intersect_elapsed_min_)
            intersect_elapsed_min_ = elapsed;
#endif
        intersect_miss_++;
        iterations_miss_ += iterations;

        if (iterations > iterations_miss_max_)
            iterations_miss_max_ = iterations;

        if (iterations < iterations_miss_min_)
            iterations_miss_min_ = iterations;

        return false;
    }

private:
    // constants
    static constexpr T isovalue = 1.0;
    static constexpr T epsilon = 0.001;
    static constexpr T step = 0.5;
    static constexpr int max_iterations = 20;

    // metrics
    mutable int intersect_hit_;
    mutable int intersect_miss_;
    mutable long long intersect_elapsed_;
    mutable int intersect_elapsed_min_;
    mutable int intersect_elapsed_max_;
    mutable int iterations_hit_;
    mutable int iterations_hit_min_;
    mutable int iterations_hit_max_;
    mutable int iterations_miss_;
    mutable int iterations_miss_min_;
    mutable int iterations_miss_max_;
};

#endif // SCALARFIELD_H
