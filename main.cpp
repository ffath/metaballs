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

#include <vector>
#include <list>

#include <math.h>

#include <QDebug>

#include "scalarfield.h"

static const qreal isovalue = 1.0;
static const qreal max_segment_length = 0.1;
static const qreal epsilon = 0.001;


class Charge : public QVector3D, public ScalarField {
public:
    Charge(const QVector3D &pos, const qreal &value) : QVector3D(pos), value_(value) {}

    Charge() : Charge(QVector3D(), 0.0) {}

    Charge(const Charge &other) : QVector3D(other), value_(other.value_) {}

    Charge &operator=(const Charge &other) {
        QVector3D::operator=(other);
        value_ = other.value_;
        return *this;
    }

    qreal value() const {
        return value_;
    }

    void setValue(const qreal &value) {
        value_ = value;
    }

    qreal valueAt(const QVector3D &pos) const override {
        QVector3D disp(pos - *this);
        return value_ / QVector3D::dotProduct(disp, disp);
    }

    QVector3D gradientAt(const QVector3D &pos) const override {
        QVector3D disp(pos - *this);
        qreal radius = sqrt(QVector3D::dotProduct(disp, disp));
        return -2.0 * value_ * (disp.normalized() / (radius * radius * radius));
    }

    QVector3D pos() const {
        return *this;
    }

    void setPos(const QVector3D &pos) {
        *this = Charge(pos, value_);
    }

private:
    qreal value_;
};


class PotentialField : public QList<Charge>, public ScalarField {
public:
    PotentialField() {}
    virtual ~PotentialField() {}

    qreal valueAt(const QVector3D &pos) const override {
        qreal value = 0.0;
        std::for_each(this->begin(), this->end(), [=, &value](const Charge &charge) {
            value += charge.valueAt(pos);
        });
        return value;
    }

    inline QVector3D gradientAt(const QVector3D &pos) const override {
        QVector3D gradient;
        std::for_each(this->begin(), this->end(), [=, &gradient](const Charge &charge) {
            gradient += charge.gradientAt(pos);
        });
        return gradient;
    }
};



class DrawingArea : public QWidget {
public:
    DrawingArea(const ScalarField &field, QWidget *parent = nullptr) : QWidget(parent),
        field_(field),
        mouse_(0, 0),
        mousePressed_(false) {
        setFrustum(2.0, 100.0, -2.0, 75.0);
    }

    void setFrustum(qreal front, qreal frontZoom, qreal back, qreal backZoom) {
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

        QSize sz(size());
        QTime time;
        time.start();
        for (int x = 0; x < sz.width(); x++) {
            for (int y = 0; y < sz.height(); y++) {
                QPointF pos(x, y);
                QVector3D front(frontTransformInverted_.map(pos));
                front.setZ(front_);
                QVector3D back(backTransformInverted_.map(pos));
                back.setZ(back_);

                QVector3D i;
                if (field_.intersect(front, back, i)) {
                    QVector3D norm = field_.gradientAt(i);
                    norm.normalize();
                    QVector3D lightVec(i - lightSource);
                    lightVec.normalize();
                    qreal light = QVector3D::dotProduct(norm, lightVec);
                    pen.setColor(QColor::fromRgbF(light, light, light));
                }
//                if ((x + y) % 2) {
//                    pen.setColor(Qt::white);
//                }
                else {
                    pen.setColor(Qt::black);
                }
                p.setPen(pen);
                p.drawPoint(x, y);
            }
        }
        qDebug() << time.elapsed() << "ms";
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
    const ScalarField &field_;
    qreal front_;
    qreal frontZoom_;
    qreal back_;
    qreal backZoom_;
    QTransform frontTransform_;
    QTransform frontTransformInverted_;
    QTransform backTransform_;
    QTransform backTransformInverted_;
    QPoint mouse_;
    bool mousePressed_;

    void updateTransforms() {
        QSizeF s(size() / 2.0);
        frontTransform_ = QTransform::fromTranslate(s.width(), s.height()).scale(frontZoom_, -frontZoom_);
        frontTransformInverted_ = frontTransform_.inverted();
        backTransform_ = QTransform::fromTranslate(s.width(), s.height()).scale(backZoom_, -backZoom_);
        backTransformInverted_ = backTransform_.inverted();
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
