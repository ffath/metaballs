#include <QApplication>
#include <QLinkedList>
#include <QPainter>
#include <QPointF>
#include <QRectF>
#include <QSizeF>
#include <QThread>
#include <QTimer>
#include <QWidget>

#include <vector>
#include <list>

#include <math.h>

#include <QDebug>

static const qreal isovalue = 1.0;
static const qreal max_segment_length = 0.1;
static const qreal epsilon = 0.001;


typedef QLinkedList<QPointF> Polygon;
static std::vector<Polygon> polygons;



class Charge : public QPointF {
public:
    Charge(qreal value) : Charge(value, 0.0, 0.0) {}
    Charge(qreal charge, qreal x, qreal y) : QPointF(x, y), charge_(charge) {}
    Charge(const Charge &other) : QPointF(other), charge_(other.charge_) {}

    inline qreal charge() const {
        return charge_;
    }

    qreal valueAt(qreal x, qreal y) const {
        qreal dx = x - this->x();
        qreal dy = y - this->y();

        if (dx == 0.0 && dy == 0.0)
            return 0.0;

        // not the actual value of a point charge but who cares ?
        return charge_ / (dx * dx + dy * dy);
    }

    QPointF gradientAt(qreal x, qreal y) const {
        qreal dx = x - this->x();
        qreal dy = y - this->y();
        qreal dx2 = dx * dx;
        qreal dy2 = dy * dy;
        qreal dx4 = dx2 * dx2;
        qreal dy4 = dy2 * dy2;
        qreal num = -2.0 * charge_;
        qreal denom = dx4 + 2.0 * dx2 * dy2 + dy4;

//        qDebug() << __func__ << "x =" << dx << ", y =" << dy << ", num =" << num << ", denom =" << denom;

        if (denom == 0.0)
            return QPointF();

        return QPointF((num * dx) / denom, (num * dy) / denom);
    }

private:
    qreal charge_;
};

static std::vector<Charge> charges;


qreal fieldAt(qreal x, qreal y) {
    qreal value = 0.0;
    std::for_each(charges.begin(), charges.end(), [=, &value](const Charge &charge) {
        value += charge.valueAt(x, y);
    });
//    qDebug() << __func__ << x << y << value;
    return value;
}

QPointF gradientAt(qreal x, qreal y) {
    QPointF gradient(0.0, 0.0);
    std::for_each(charges.begin(), charges.end(), [=, &gradient](const Charge &charge) {
        gradient += charge.gradientAt(x, y);
    });
//    qDebug() << __func__ << x << y << gradient << sqrt(QPointF::dotProduct(gradient, gradient));
    return gradient;
}

class DrawingArea : public QWidget {
public:
    DrawingArea(QWidget *parent = nullptr) : QWidget(parent), zoom_(100.0) {}

    void setZoom(qreal zoom) {
        zoom_ = zoom;
        update();
    }

    qreal zoom() {
        return zoom_;
    }

protected:
    void paintEvent(QPaintEvent *pe) override {
        (void) pe;

        qDebug() << __func__;

        QPainter p(this);

        QSizeF s = size() / 2.0;
        p.translate(QPoint(s.width(), s.height()));
        p.scale(zoom_, zoom_);
        s /= zoom_;

        QPen pen(p.pen());
        pen.setWidth(0);
        p.setPen(pen);

        std::for_each(polygons.begin(), polygons.end(), [&p](const Polygon &polygon) {
            QPointF prev = polygon.last();
            std::for_each(polygon.begin(), polygon.end(), [&](const QPointF &cur) {
                p.drawLine(prev, cur);
                prev = cur;
            });
        });
    }

private:
    qreal zoom_;
};

int moveVertices(Polygon &polygon) {
    int moved = 0;
    QPointF prev = polygon.last();
    auto it = polygon.begin();
    do {
        QPointF &pt = *it++;
        QPointF next = (it != polygon.end()) ? *it : polygon.first();

        // compute normal
        QPointF norm = next - prev;
        norm = QPointF(-norm.y(), norm.x());
        norm /= sqrt(QPointF::dotProduct(norm, norm));

//        qDebug() << prev << pt << next << norm;

        // get normalized gradient
        qreal gradval = 1.0;
        QPointF gradvec = gradientAt(pt.x(), pt.y());
        if (!gradvec.isNull()) {
            gradval = sqrt(QPointF::dotProduct(gradvec, gradvec));
            gradvec /= gradval;
        }

        // then move in the gradient direction
        qreal value = fieldAt(pt.x(), pt.y());
        qreal delta = isovalue - value;

        prev = pt;

        if (abs(delta) > epsilon) {
            if (value > 4)
                gradval = abs(delta) * 2.0;
            pt += norm * (delta / gradval);
//            pt += gradvec * (delta / gradval);
            moved++;
        }
    } while (it != polygon.end());

    qDebug() << __func__ << moved;
    return moved;
}

int resample0(Polygon &polygon) {
    int added = 0;
    auto it = polygon.begin();
    QPointF prev = polygon.last();
    while (it != polygon.end()) {
        QPointF &pt = *it;

        // resample ?
        QPointF vect = pt - prev;
        if (QPointF::dotProduct(vect, vect) > max_segment_length * max_segment_length) {
            // too big, cut in half
            it = polygon.insert(it, (pt + prev) / 2.0);
            added ++;
            continue; // try again with the newly inserted point
        }

        // next
        prev = pt;
        it++;
    }

    qDebug() << __func__ << added;
    return added;
}

int resample(Polygon &polygon) {
    int added = 0;
    auto it = polygon.begin();
    QPointF prev = polygon.last();
    while (it != polygon.end()) {
        QPointF &pt = *it;

        // resample ?
        QPointF vect = pt - prev;
        if (QPointF::dotProduct(vect, vect) > max_segment_length * max_segment_length) {
            // too big, cut in half
            /*it =*/ polygon.insert(it, (pt + prev) / 2.0);
            added ++;
//            continue; // try again with the newly inserted point
        }

        // next
        prev = pt;
        it++;
    }

    qDebug() << __func__ << added;
    return added;
}

bool process() {
    bool did_something = false;

    qDebug() << __func__;

//    std::for_each(polygons.begin(), polygons.begin() + 1, [&did_something](Polygon &polygon) {
    std::for_each(polygons.begin(), polygons.end(), [&did_something](Polygon &polygon) {
        if (moveVertices(polygon) || resample(polygon)) {
            did_something = true;
        }
    });
    return did_something;
}

class Timer : public QTimer {
protected:
    void timerEvent(QTimerEvent *e) {
        qDebug() << __func__;
        if (!process()) {
            stop();
        }
        QTimer::timerEvent(e);
    }
};

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // init charges
//    charges.push_back(Charge(1.0, 0.0, 0.0));
    charges.push_back(Charge(1.0, 0.0, 1.0));
    charges.push_back(Charge(1.0, -2.0, -1.0));
    charges.push_back(Charge(1.0, 2.0, -1.0));

    // init polygons
    std::for_each(charges.begin(), charges.end(), [](const Charge &charge) {
        Polygon polygon;
        for (qreal alpha = 0.0; alpha < 2.0 * M_PI; alpha += M_PI / 2.0) {
            QPointF p(cos(alpha), sin(alpha));
            p *= charge.charge();
            p += charge;
            polygon << p;
        }
//        QRectF r(-0.25, -0.25, 0.5, 0.5);
//        r.moveCenter(charge);
//        polygon << r.topLeft() << r.topRight() << r.bottomRight() << r.bottomLeft();
        polygons.push_back(polygon);
    });

    // gui
    DrawingArea da;
    da.resize(1280, 720);
    da.show();

    // timer
    Timer timer;
    timer.setInterval(50);
    QObject::connect(&timer, SIGNAL(timeout()), &da, SLOT(update()));
    timer.start();

    return a.exec();
}
