#ifndef RENDERER_H
#define RENDERER_H
#include <QImage>

#include <memory>

#include "scalarfield.h"
#include "inlinemath.h"

using namespace inlinemath;

class Renderer {
public:
    virtual void render(QImage *image) = 0;
};

template <class F, class T>
class FieldRenderer : public Renderer {
public:
    FieldRenderer(const F &field) : field_(field) {
        setFrustum(2.0, 50.0, -2.0, 37.5);
    }

    void setFrustum(T front, T frontZoom, T back, T backZoom) {
        front_ = front;
        frontZoom_ = frontZoom;
        back_ = back;
        backZoom_ = backZoom;

        updateTransforms();
    }

    void render(QImage *image) override {
        QSize size = image->size();
        if (size_ != size) {
            size_ = size;
            updateTransforms();
            updateRays();
        }

        // temp: set light sources
        Vector3D<T> lightSource(0.0, 0.0, 50.0);

        QTime time;
        time.start();

        uchar *line = image->bits();
        int bytesPerLine = image->bytesPerLine();
        Ray *rays = rays_.get();

        Vector3D<T> i;
        Vector3D<T> normal;
        Vector3D<T> lightVec;

        int w = size_.width();
        int h = size_.height();

        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                Ray &r = rays[y * w + x];
                uint c = 0;

                if (field_.intersect(r.p, r.direction, r.length, i, normal)) {
                    normal.normalize();
                    lightVec = i - lightSource;
                    lightVec.normalize();
                    T light = Vector3D<T>::dotProduct(normal, lightVec);
                    c = 0xff & (uint) (light * 255);
                }
                ((uint *) line)[x] = qRgb(c, c, c);
            }
            line += bytesPerLine;
        }

        qDebug() << __func__ << time.elapsed() << "ms";
    }

private:
    const F &field_;

    T front_;
    T frontZoom_;
    T back_;
    T backZoom_;

    QTransform frontTransform_;
    QTransform frontTransformInverted_;
    QTransform backTransform_;
    QTransform backTransformInverted_;

    struct Ray {
        Vector3D<T> p;
        Vector3D<T> direction;
        T length;
    };

    // precalculated rays
    std::unique_ptr<Ray> rays_;

    // last image size
    QSize size_;

    // update transformation matrices
    void updateTransforms() {
        int w = size_.width();
        int h = size_.height();
        frontTransform_ = QTransform::fromTranslate(w / 2, h / 2).scale(frontZoom_, -frontZoom_);
        frontTransformInverted_ = frontTransform_.inverted();
        backTransform_ = QTransform::fromTranslate(w / 2, h / 2).scale(backZoom_, -backZoom_);
        backTransformInverted_ = backTransform_.inverted();
    }

    // update precomputed rays
    void updateRays() {
        QTime time;
        time.start();

        int w = size_.width();
        int h = size_.height();

        Ray *rays = new Ray[w * h];
        int index = 0;
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                Ray &r = rays[index++];

                QPointF pos(x, y);
                QPointF f = frontTransformInverted_.map(pos);
                QPointF b = backTransformInverted_.map(pos);
                Vector3D<T> front((T) f.x(), (T) f.y(), front_);
                Vector3D<T> back((T) b.x(), (T) b.y(), back_);

                Vector3D<T> direction = back - front;
                T length = direction.length();
                direction /= length;

                r.p = front;
                r.direction = direction;
                r.length = length;
            }
        }
        rays_.reset(rays);

        qDebug() << __func__ << time.elapsed() << "ms";
    }
};
#endif // RENDERER_H
