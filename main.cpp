#include <QApplication>
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

class Charge : public QVector3D, public ScalarField<Charge> {
public:
    Charge(const QVector3D &pos, const float &value) : QVector3D(pos), value_(value) {}

    Charge() : Charge(QVector3D(), 0.0) {}

    Charge(const Charge &other) : QVector3D(other), value_(other.value_) {}

    Charge &operator=(const Charge &other) {
        QVector3D::operator=(other);
        value_ = other.value_;
        return *this;
    }

    inline float value() const {
        return value_;
    }

    inline void setValue(const float &value) {
        value_ = value;
    }

    inline float valueAndGradientAt(const QVector3D &pos, QVector3D &gradient) const {
        QVector3D disp = pos - *(static_cast<const QVector3D *>(this));
        float radius2 = inlinemath::lengthSquared(disp);
        float value = value_ / radius2;
        gradient = -2.0 * value * (disp / radius2);
        return value;
    }

    inline QVector3D pos() const {
        return *this;
    }

    inline void setPos(const QVector3D &pos) {
        *this = Charge(pos, value_);
    }

private:
    float value_;
};


class PotentialField : public QVector<Charge>, public ScalarField<PotentialField> {
public:
    PotentialField() {}
    virtual ~PotentialField() {}

    inline float valueAndGradientAt(const QVector3D &pos, QVector3D &gradient) const {
        float value = 0.0;
        QVector3D g, lg;
        const Charge *charges = this->constData();
        int length = size();
        for (int i = 0; i < length; i++) {
            value += charges[i].valueAndGradientAt(pos, lg);
            g += lg;
        }
        gradient = g;
        return value;
    }
};



class DrawingArea : public QWidget {
public:
    DrawingArea(const PotentialField &field, QWidget *parent = nullptr) : QWidget(parent),
        field_(field),
        mouse_(0, 0),
        mousePressed_(false) {
        setFrustum(2.0, 100.0, -2.0, 75.0);
    }

    void setFrustum(float front, float frontZoom, float back, float backZoom) {
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

        QPainter p(this);

        if (mousePressed_) {
            // move first charge
//            Charge &c = charges.front();
//            QPointF transformed = p.transform().inverted().map(QPointF(mouse_));
//            qDebug() << mouse_ << transformed;
//            c.setX(transformed.x());
//            c.setY(transformed.y());
        }

        QVector3D lightSource(0.0, 0.0, 50.0);
        QPen pen(p.pen());

        QTime time;
        time.start();

        int w = size().width();
        int h = size().height();
        Ray *rays = rays_.get();
        int index = 0;
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                Ray &r = rays[index++];

                QVector3D i;
                QVector3D normal;
                if (field_.intersect(r.p, r.direction, r.length, i, normal)) {
                    inlinemath::normalize(normal);
                    QVector3D lightVec(i - lightSource);
                    inlinemath::normalize(lightVec);
                    float light = inlinemath::dotProduct(normal, lightVec);
                    pen.setColor(QColor::fromRgbF(light, light, light));
                }
                else {
                    pen.setColor(Qt::black);
                }
                p.setPen(pen);
                p.drawPoint(x, y);
            }
        }
        qDebug() << __func__ << time.elapsed() << "ms";
    }

    void resizeEvent(QResizeEvent *event) {
        (void) event;
        updateTransforms();
    }

    void mousePressEvent(QMouseEvent *me) override {
        qDebug() << __func__;
        mousePressed_ = true;
        mouseMoveEvent(me); // dirty
    }

    void mouseMoveEvent(QMouseEvent *me) override {
        qDebug() << __func__;
        mouse_ = me->pos();
//        polygons.clear();
        update();
    }

    void mouseReleaseEvent(QMouseEvent *me) override {
        (void) me;
        qDebug() << __func__;
        mousePressed_ = false;
    }

private:
    const PotentialField &field_;

    float front_;
    float frontZoom_;
    float back_;
    float backZoom_;

    QTransform frontTransform_;
    QTransform frontTransformInverted_;
    QTransform backTransform_;
    QTransform backTransformInverted_;

    struct Ray {
        QVector3D p;
        QVector3D direction;
        float length;
    };
    std::unique_ptr<Ray> rays_;

    QPoint mouse_;
    bool mousePressed_;

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
                QVector3D front(frontTransformInverted_.map(pos));
                front.setZ(front_);
                QVector3D back(backTransformInverted_.map(pos));
                back.setZ(back_);

                QVector3D direction(back - front);
                float length = inlinemath::length(direction);
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


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // init charges
    PotentialField field;
    field << Charge(QVector3D(0.0, 2.0, 0.0), 1.0);
    field << Charge(QVector3D(-2.0, 0.0, 0.0), 1.0);
    field << Charge(QVector3D(2.0, 0.0, 0.0), 1.0);
    field << Charge(QVector3D(0.0, -2.0, 0.0), 1.0);

    // gui
    DrawingArea da(field);
    da.resize(1280, 720);
    da.show();

    return a.exec();
}
