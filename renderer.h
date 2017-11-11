#ifndef RENDERER_H
#define RENDERER_H
#include <QImage>
#include <QSemaphore>
#include <QThread>
#include <QVector>

#include <functional>
#include <memory>

#include "scalarfield.h"
#include "inlinemath.h"

using namespace inlinemath;

class Renderer {
public:
    virtual ~Renderer() {
    }

    virtual void render(QImage *image) = 0;
};


class WorkerThread : public QThread {
public:
    WorkerThread(int threadNumber, const std::function<void(int)> &func) :
        threadNumber_(threadNumber),
        func_(func) {
    }

protected:
    void run() {
        func_(threadNumber_);
    }
private:
    int threadNumber_;
    const std::function<void(int)> func_;
};

template <class F, class T>
class FieldRenderer : public Renderer {
public:
    FieldRenderer(const F &field) : field_(field), image_(nullptr) {
        setFrustum(2.0, 50.0, -2.0, 37.5);

        threadCount_ = QThread::idealThreadCount();
        if (threadCount_ == -1) {
            threadCount_ = 1;
        }

        for (int i = 0; i < threadCount_; i++) {
            QThread *worker = new WorkerThread(i, std::bind(&FieldRenderer::process, this, std::placeholders::_1));
            worker->start();
        }
    }

    ~FieldRenderer() override {
        int threadCount = threadCount_;
        threadCount_ = 0;
        semaphoreBegin_.release(threadCount);
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

        QTime time;
        time.start();

        image_ = image;
        semaphoreBegin_.release(threadCount_);
        semaphoreEnd_.acquire(threadCount_);
        image_ = nullptr;

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

    // temporary pointer to image used during rendering
    QImage *image_;

    // threading
    int threadCount_;
    QSemaphore semaphoreBegin_;
    QSemaphore semaphoreEnd_;

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

    void process(int threadNumber) {
        qDebug() << "thread" << threadNumber << "in";

        // temp: set light sources elsewhere
        Vector3D<T> lightSource(0.0, 0.0, 50.0);

        while (true) {
            semaphoreBegin_.acquire();

            if (threadCount_ == 0)
                break;

            uchar *line = image_->bits();
            int bytesPerLine = image_->bytesPerLine();
            Ray *rays = rays_.get();

            Vector3D<T> i;
            Vector3D<T> normal;
            Vector3D<T> lightVec;

            int w = size_.width();
            int h = size_.height();

            // my range
            int y1 = (h /threadCount_) * threadNumber;
            int y2 = (h / threadCount_) * (threadNumber + 1);
            line += y1 * bytesPerLine;

            for (int y = y1; y < y2; y++) {
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

            semaphoreEnd_.release();
        }

        qDebug() << "thread" << threadNumber << "out";
    }
};
#endif // RENDERER_H
