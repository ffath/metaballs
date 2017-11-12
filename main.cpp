#include <QApplication>
#include <QDebug>
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

// SSE2
#include <emmintrin.h>

#include "inlinemath.h"
#include "scalarfield.h"
#include "renderer.h"

using namespace inlinemath;

class Charge : public Vector3D, public ScalarField<Charge> {
public:
    Charge(float x, float y, float z, float value) : Vector3D(x, y, z), value_(value) {}

    Charge(const Vector3D &pos, float value) : Vector3D(pos), value_(value) {}

    Charge(float value) : Charge(Vector3D(), value) {}

    Charge() : Charge(Vector3D(), 1.0f) {}

    Charge(const Charge &other) : Vector3D(other), value_(other.value_) {}

    Charge &operator=(const Charge &other) {
        Vector3D::operator=(other);
        value_ = other.value_;
        return *this;
    }

    inline float value() const {
        return value_;
    }

    inline void setValue(const float &value) {
        value_ = value;
    }

    inline float fieldAt(const Vector3D &pos, Vector3D &gradient) const {
        Vector3D disp = pos - *(static_cast<const Vector3D *>(this));
        float radius2 = disp.lengthSquared();
        float value = value_ / radius2;
        gradient = (float) -2.0f * value * (disp / radius2);
        return value;
    }

    inline Vector3D pos() const {
        return *this;
    }

    inline void setPos(const Vector3D &pos) {
        Vector3D::operator=(pos);
    }

private:
    float value_;
};


class PotentialField : public QVector<Charge>, public ScalarField<PotentialField> {
public:
    PotentialField() {}
    virtual ~PotentialField() {}

    inline float fieldAt(const Vector3D &pos, Vector3D &gradient) const {
        float value = 0.0;
        Vector3D g, lg;
        const Charge *charges = this->constData();
        int length = size();
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


void animate(PotentialField &field) {
    static Vector3D *directions = nullptr;
    int i, size = field.size();

    if (directions == nullptr) {
        directions = new Vector3D[size];
        for (i = 0; i < size; i++) {
            float x = (float) rand() / RAND_MAX;
            float y = (float) rand() / RAND_MAX;
//            float z = (float) rand() / RAND_MAX;
            directions[i] = Vector3D(x / 5.0, y / 5.0, 0.0);
        }
    }

    for (i = 0; i < size; i++) {
        auto pos = field[i].pos();
        pos += directions[i];
        if (std::abs(pos.x()) > 5.5) {
            directions[i] *= Vector3D(-1.0f, 1.0f, 1.0f);
        }
        if (std::abs(pos.y()) > 3.5) {
            directions[i] *= Vector3D(1.0f, -1.0f, 1.0f);
        }
        field[i].setPos(pos);
    }
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // init charges
    PotentialField field;
    field << Charge(1.5);
    field << Charge(1.5);
    field << Charge(1.5);
    field << Charge(1.5);
    field << Charge(1.5);

    // gui
    FieldRenderer<PotentialField> renderer(field);
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

    return app.exec();
}
