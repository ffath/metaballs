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

template <class F>
class FieldRenderer : public Renderer {
public:
    FieldRenderer(const F &field) : field_(field), image_(nullptr) {
        qInfo() << __func__ << "in";

        setFrustum(2.0, 50.0, -2.0, 37.5);

        threadCount_ = QThread::idealThreadCount() * 2;
        if (threadCount_ == -1) {
            threadCount_ = 1;
        }

        workers_ = new QThread *[threadCount_];
        for (int i = 0; i < threadCount_; i++) {
            workers_[i] = new WorkerThread(i, std::bind(&FieldRenderer::process, this, std::placeholders::_1));
            workers_[i]->start();
        }
        semaphoreStartWaiting_.acquire(threadCount_);
        qInfo() << __func__ << "out";
    }

    ~FieldRenderer() override {
        qInfo() << __func__ << "in";

        int threadCount = threadCount_;
        threadCount_ = 0;
        semaphoreBeginWorking_.release(threadCount);
        for (int i = 0; i < threadCount; i++) {
            workers_[i]->wait(ULONG_MAX);
            delete workers_[i];
            workers_[i] = nullptr;
        }
        delete workers_;

        qInfo() << __func__ << "out";
    }

    void setFrustum(float front, float frontZoom, float back, float backZoom) {
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
        semaphoreBeginWorking_.release(threadCount_);
        semaphoreWorkDone_.acquire(threadCount_);
        semaphoreStartWaiting_.release(threadCount_);
        image_ = nullptr;

        qInfo() << __func__ << time.elapsed() << "ms";
    }

private:
    const F &field_;

    float front_;
    float frontZoom_;
    float back_;
    float backZoom_;

    QTransform frontTransform_;
    QTransform frontTransformInverted_;
    QTransform backTransform_;
    QTransform backTransformInverted_;

    struct Ray {
        Vector3D p;
        Vector3D direction;
        float length;
    };

    // precalculated rays
    std::unique_ptr<Ray> rays_;

    // last image size
    QSize size_;

    // temporary pointer to image used during rendering
     QImage *image_;

    // threading
    volatile int threadCount_;
    QThread **workers_;
    QSemaphore semaphoreBeginWorking_;
    QSemaphore semaphoreWorkDone_;
    QSemaphore semaphoreStartWaiting_;

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
                Vector3D front(f.x(), f.y(), front_);
                Vector3D back(b.x(), b.y(), back_);

                Vector3D direction = back - front;
                float length = direction.length();
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
        qInfo() << "thread" << threadNumber << "in";

        semaphoreStartWaiting_.release();

        // temp: set light sources elsewhere
        Vector3D lightSource(0.0, 0.0, 50.0);

        while (true) {
            semaphoreBeginWorking_.acquire();

            if (threadCount_ == 0)
                break;

            uchar *line = image_->bits();
            int bytesPerLine = image_->bytesPerLine();
            Ray *rays = rays_.get();

            Vector3D i;
            Vector3D normal;
            Vector3D lightVec;

            int w = size_.width();
            int h = size_.height();

            // my range
            int y1 = (h / threadCount_) * threadNumber;
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
                        float light = Vector3D::dotProduct(normal, lightVec);
                        if (light < 0.0) light = 0.0;
                        if (light > 1.0) light = 1.0;
                        c = light * 255;
                    }
                    ((uint *) line)[x] = qRgb(c, c, c);
                }
                line += bytesPerLine;
            }

            semaphoreWorkDone_.release();
            semaphoreStartWaiting_.acquire();
        }

        qInfo() << "thread" << threadNumber << "out";
    }
};
#endif // RENDERER_H
