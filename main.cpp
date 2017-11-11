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

#include <functional>
#include <memory>
#include <list>
#include <vector>
#include <typeinfo>

#include <math.h>
#include <stdlib.h>

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

    Charge(T value) : Charge(Vector3D<T>(), value) {}

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
        Vector3D<T>::operator=(pos);
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


typedef PotentialField<float> Field;

void animate(Field &field) {
    static Vector3D<Field::FloatType> *directions = nullptr;
    int i, size = field.size();

    if (directions == nullptr) {
        directions = new Vector3D<Field::FloatType>[size];
        for (i = 0; i < size; i++) {
            float x = (float) rand() / RAND_MAX;
            float y = (float) rand() / RAND_MAX;
//            float z = (float) rand() / RAND_MAX;
            directions[i] = Vector3D<Field::FloatType>(x / 5.0, y / 5.0, 0.0);
        }
    }

    for (i = 0; i < size; i++) {
        auto pos = field[i].pos();
        pos += directions[i];
        if (std::abs(pos.x) > 5.5) {
            directions[i].x *= -1;
        }
        if (std::abs(pos.y) > 3.5) {
            directions[i].y *= -1;
        }
        field[i].setPos(pos);
    }
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // init charges
    Field field;
//    field << Field::ChargeType(0.0, 3.0, 0.0, 1.0);
//    field << Field::ChargeType(-2.0, 0.0, 0.0, 1.0);
//    field << Field::ChargeType(2.0, 0.0, 0.0, 1.0);
//    field << Field::ChargeType(0.0, -2.0, 0.0, 1.0);
    field << Field::ChargeType(1.5);
    field << Field::ChargeType(1.5);
    field << Field::ChargeType(1.5);
    field << Field::ChargeType(1.5);
    field << Field::ChargeType(1.5);

    // gui
    FieldRenderer<Field, Field::FloatType> renderer(field);
    DrawingArea da(renderer);
    da.resize(640, 480);
    da.show();

    // animation
    QTimer timer;
    timer.setInterval(0);
    timer.setSingleShot(false);
    QObject::connect(&timer, &QTimer::timeout, [&]() {
        animate(field);
        da.update();
    });
    timer.start();

    return a.exec();
}
