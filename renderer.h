#ifndef RENDERER_H
#define RENDERER_H
#include <QAtomicInt>
#include <QImage>
#include <QSemaphore>
#include <QThread>
#include <QVector>

#include <functional>
#include <memory>

#include <emmintrin.h>

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
    FieldRenderer(const F &field) : field_(field), image_(nullptr), lineNumber_(0) {
        qDebug() << __func__ << "in";

        setFrustum(2.0, 50.0, -2.0, 37.5);

        threadCount_ = QThread::idealThreadCount();
        if (threadCount_ == -1) {
            threadCount_ = 1;
        }

        workers_ = new QThread *[threadCount_];
        for (int i = 0; i < threadCount_; i++) {
            workers_[i] = new WorkerThread(i, std::bind(&FieldRenderer::process, this, std::placeholders::_1));
            workers_[i]->start();
        }
        semaphoreStartWaiting_.acquire(threadCount_);
        qDebug() << __func__ << "out";
    }

    ~FieldRenderer() override {
        qDebug() << __func__ << "in";

        int threadCount = threadCount_;
        threadCount_ = 0;
        semaphoreBeginWorking_.release(threadCount);
        for (int i = 0; i < threadCount; i++) {
            workers_[i]->wait(ULONG_MAX);
            delete workers_[i];
            workers_[i] = nullptr;
        }
        delete workers_;

        qDebug() << __func__ << "out";
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
        lineNumber_ = 0;
        semaphoreStartWaiting_.release(threadCount_);
        image_ = nullptr;

        qDebug() << __func__ << time.elapsed() << "ms";
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
    int stride_;

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
    volatile int lineNumber_;

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

        int w = ((size_.width() + 3) / 4) * 4; // multiple of 4;
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
        stride_ = w;

        qDebug() << __func__ << time.elapsed() << "ms";
    }

    void process(int threadNumber) {
        float dotProduct[4];
        float lengthSquaredNorm[4];
        float lengthSquaredLight[4];
        uchar pixel[4];

        Vector3D i;
        Vector3D normal;
        Vector3D lightVec;

        __m128 dotp, a, b, div;
        __m128 zero = _mm_set1_ps(0.0f);
        __m128 one = _mm_set1_ps(1.0f);
        __m128 f255 = _mm_set1_ps(255.0f);
        __m128i light;

        qDebug() << "thread" << threadNumber << "in";

        semaphoreStartWaiting_.release();

        // temp: set light sources elsewhere
        Vector3D lightSource(0.0, 0.0, 50.0);

        while (true) {
            semaphoreBeginWorking_.acquire();

            if (threadCount_ == 0)
                break;

            uchar *bits = image_->bits(), *line = nullptr;
            int bytesPerLine = image_->bytesPerLine();
            Ray *rays = rays_.get();

            int w = size_.width();
            int h = size_.height();

            // each thread is going to find itself a line to draw until there's none left
            int y;
            while ((y = __sync_fetch_and_add(&lineNumber_, 1)) < h) {
                line = bits + y * bytesPerLine;

                // lines commented out in the margin are the former non SSE2 code
                // that does not seem to be actually slower (why i kept it, just in case)
                for (int x = 0; x < w; x += 4) {
                    for (int n = 0; n < 4; n++) {
                        Ray &r = rays[y * stride_ + x + n];
//                        uint c = 0;

                        if (field_.intersect(r.p, r.direction, r.length, i, normal)) {
//                            normal.normalize();
                            lightVec = i - lightSource;
//                            lightVec.normalize();
//                            float light = Vector3D::dotProduct(normal, lightVec);
//                            if (light < 0.0) light = 0.0;
//                            if (light > 1.0) light = 1.0;
//                            c = light * 255;

                            dotProduct[n] = Vector3D::dotProduct(normal, lightVec);
                            lengthSquaredNorm[n] = normal.lengthSquared();
                            lengthSquaredLight[n] = lightVec.lengthSquared();
                        }
                        else {
                            dotProduct[n] = 0.0f;
                            lengthSquaredNorm[n] = 1.0f;
                            lengthSquaredLight[n] = 1.0f;
                        }
//                        ((uint *) line)[x + n] = qRgb(c, c, c);
                    }

                    // a little sse magic
                    dotp = _mm_load_ps(dotProduct);
                    a = _mm_load_ps(lengthSquaredLight);
                    b = _mm_load_ps(lengthSquaredNorm);

                    div = _mm_mul_ps(a, b);         // div = light^2 * norm^2
                    div = _mm_sqrt_ps(div);         // div = sqrt(light^2 * norm^2)
                    dotp = _mm_div_ps(dotp, div);   // dotp = dotProduct(norm/|norm|, light/|light|)
                    dotp = _mm_max_ps(dotp, zero);  // nothing under 0
                    dotp = _mm_min_ps(dotp, one);   // nothing above 1
                    dotp = _mm_mul_ps(dotp, f255);  // expand to 0..255

                    light = _mm_cvtps_epi32(dotp);  // convert to 4 * int32
                    light = _mm_packs_epi32(light, light);  // convert to 2 * 4 * int16 (sort of)
                    light = _mm_packus_epi16(light, light); // same to 2 * 2 * 4 * unit8
                    *((int *) &pixel[0]) = _mm_cvtsi128_si32(light);    // write 32 LSB to pixels

                    // write the pixels, unroll loop
                    ((uint *) line)[x + 1] = qRgb(pixel[0], pixel[0], pixel[0]);
                    ((uint *) line)[x + 2] = qRgb(pixel[1], pixel[1], pixel[1]);
                    ((uint *) line)[x + 3] = qRgb(pixel[2], pixel[2], pixel[2]);
                    ((uint *) line)[x + 4] = qRgb(pixel[3], pixel[3], pixel[3]);
                }
            }

            semaphoreWorkDone_.release();
            semaphoreStartWaiting_.acquire();
        }

        qDebug() << "thread" << threadNumber << "out";
    }
};
#endif // RENDERER_H
