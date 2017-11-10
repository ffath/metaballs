#include <QApplication>
#include <QImage>
#include <QLinkedList>
#include <QList>
#include <QMouseEvent>
#include <QPainter>
#include <QPoint>
#include <QPointF>
#include <QRectF>
#include <QSizeF>
#include <QThread>
#include <QTime>
#include <QTimer>
#include <QWidget>

#include <memory>
#include <list>
#include <vector>
#include <typeinfo>

#include <math.h>

#include <QDebug>

#include "inlinemath.h"
#include "scalarfield.h"
#include "renderer.h"

using namespace inlinemath;

template <class T>
class Charge : public Vector3D<T>, public ScalarField<Charge<T>, T> {
public:
    Charge(T x, T y, T z, T value) : Vector3D<T>(x, y, z), value_(value) {}

    Charge(const Vector3D<T> &pos, T value) : Vector3D<T>(pos), value_(value) {}

    Charge() : Charge(Vector3D<T>(), 1.0) {}

    Charge(const Charge &other) : Vector3D<T>(other), value_(other.value_) {}

    Charge &operator=(const Charge &other) {
        Vector3D<T>::operator=(other);
        value_ = other.value_;
        return *this;
    }

    inline T value() const {
        return value_;
    }

    inline void setValue(const T &value) {
        value_ = value;
    }

    inline T fieldAt(const Vector3D<T> &pos, Vector3D<T> &gradient) const {
        Vector3D<T> disp = pos - *(static_cast<const Vector3D<T> *>(this));
        T radius2 = disp.lengthSquared();
        T value = value_ / radius2;
        gradient = (T) -2.0 * value * (disp / radius2);
        return value;
    }

    inline Vector3D<T> pos() const {
        return *this;
    }

    inline void setPos(const Vector3D<T> &pos) {
        *this = Charge(pos, value_);
    }

private:
    T value_;
};


template <class T>
class PotentialField : public QVector<Charge<T>>, public ScalarField<PotentialField<T>, T> {
public:
    typedef Charge<T> ChargeType;

    PotentialField() {}
    virtual ~PotentialField() {}

    inline T fieldAt(const Vector3D<T> &pos, Vector3D<T> &gradient) const {
        T value = 0.0;
        Vector3D<T> g, lg;
        const Charge<T> *charges = this->constData();
        int length = QVector<Charge<T>>::size();
        for (int i = 0; i < length; i++) {
            value += charges[i].fieldAt(pos, lg);
            g += lg;
        }
        gradient = g;
        return value;
    }
};

#if 0
template <class T>
class DrawingArea : public QWidget {
public:
    typedef T FloatType;
    typedef PotentialField<T> FieldType;

    DrawingArea(const FieldType &field, QWidget *parent = nullptr) : QWidget(parent),
        field_(field),
        image_(new QImage(size(), QImage::Format_ARGB32)) {
        setFrustum(2.0, 50.0, -2.0, 37.5);
    }

    void setFrustum(T front, T frontZoom, T back, T backZoom) {
        front_ = front;
        frontZoom_ = frontZoom;
        back_ = back;
        backZoom_ = backZoom;

        updateTransforms();
        update();
    }

protected:
    void paintEvent(QPaintEvent *pe) override {
        (void) pe;

        Vector3D<T> lightSource(0.0, 0.0, 50.0);

        QTime time;
        time.start();

        uchar *line = image_->bits();
        int bytesPerLine = image_->bytesPerLine();
        Ray *rays = rays_.get();

        Vector3D<T> i;
        Vector3D<T> normal;
        Vector3D<T> lightVec;

        int w = size().width();
        int h = size().height();

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

        QPainter p(this);
        p.drawImage(0, 0, *image_.get());

        qDebug() << __func__ << time.elapsed() << "ms";
    }

    void resizeEvent(QResizeEvent *event) {
        (void) event;
        updateTransforms();
        image_.reset(new QImage(size(), QImage::Format_ARGB32));
    }

private:
    const FieldType &field_;

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
        float length;
    };
    std::unique_ptr<Ray> rays_;

    std::unique_ptr<QImage> image_;

    void updateTransforms() {
        QTime time;
        time.start();

        QSize s(size());
        int w = s.width();
        int h = s.height();
        frontTransform_ = QTransform::fromTranslate(w / 2, h / 2).scale(frontZoom_, -frontZoom_);
        frontTransformInverted_ = frontTransform_.inverted();
        backTransform_ = QTransform::fromTranslate(w / 2, h / 2).scale(backZoom_, -backZoom_);
        backTransformInverted_ = backTransform_.inverted();

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
#endif

class DrawingArea : public QWidget {
public:

    DrawingArea(Renderer &renderer, QWidget *parent = nullptr) : QWidget(parent),
        renderer_(renderer),
        image_(new QImage(size(), QImage::Format_ARGB32)) {
    }

protected:
    void paintEvent(QPaintEvent *pe) override {
        (void) pe;

        qDebug() << __func__ << size();

        renderer_.render(image_.get());

        QPainter p(this);
        p.drawImage(0, 0, *image_.get());
    }

    void resizeEvent(QResizeEvent *event) {
        (void) event;

        qDebug() << __func__ << size();

        image_.reset(new QImage(size(), QImage::Format_ARGB32));
    }

private:
    Renderer &renderer_;
    std::unique_ptr<QImage> image_;
};


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // init charges
    typedef PotentialField<float> Field;
    Field field;
    field << Field::ChargeType(0.0, 2.0, 0.0, 1.0);
    field << Field::ChargeType(-2.0, 0.0, 0.0, 1.0);
    field << Field::ChargeType(2.0, 0.0, 0.0, 1.0);
    field << Field::ChargeType(0.0, -2.0, 0.0, 1.0);
//    field << Field::ChargeType();
//    field << Field::ChargeType();
//    field << Field::ChargeType();
//    field << Field::ChargeType();

    // gui
    FieldRenderer<Field, Field::FloatType> renderer(field);
    DrawingArea da(renderer);
    da.resize(640, 360);
    da.show();

    return a.exec();
}
