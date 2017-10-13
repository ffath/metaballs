#include <QApplication>
#include <QLinkedList>
#include <QMouseEvent>
#include <QPainter>
#include <QPoint>
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


class Charge : public QPointF {
public:
    Charge(qreal value) : Charge(value, 0.0, 0.0) {}
    Charge(qreal charge, qreal x, qreal y) : QPointF(x, y), charge_(charge) {}
    Charge(qreal charge, QPointF pt) : QPointF(pt), charge_(charge) {}
    Charge(const Charge &other) : QPointF(other), charge_(other.charge_) {}

    inline qreal charge() const {
        return charge_;
    }

    qreal valueAt(qreal x, qreal y) const {
        qreal dx = x - this->x();
        qreal dy = y - this->y();

        if (dx == 0.0 && dy == 0.0)
            return INFINITY;

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
    return value;
}


QPointF gradientAt(qreal x, qreal y) {
    QPointF gradient(0.0, 0.0);
    std::for_each(charges.begin(), charges.end(), [=, &gradient](const Charge &charge) {
        gradient += charge.gradientAt(x, y);
    });
    return gradient;
}


class Polygon : public QLinkedList<QPointF> {
public:
    Polygon() {
    }

    Polygon(const Charge &charge) {
        for (qreal alpha = 0.0; alpha < 2.0 * M_PI; alpha += M_PI / 2.0) {
            QPointF p(cos(alpha), sin(alpha));
            p *= charge.charge();
            p += charge;
            append(p);
        }
    }

    int resample() {
        int added = 0;
        QPointF prev = last();
        for (auto it = begin(); it != end(); it++) {
            QPointF &pt = *it;
            QPointF vect = pt - prev;
            if (QPointF::dotProduct(vect, vect) > max_segment_length * max_segment_length) {
                // too big, cut in half
                insert(it, (pt + prev) / 2.0);
                added ++;
            }

            // next
            prev = pt;
        }

        return added;
    }

    int moveVertices() {
        int moved = 0;
        QPointF prev = last();
        auto it = begin();
        do {
            QPointF &pt = *it++;
            QPointF next = (it != end()) ? *it : first();

            // compute normal
            QPointF norm = next - prev;
            norm = QPointF(-norm.y(), norm.x());
            norm /= sqrt(QPointF::dotProduct(norm, norm));

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
                // pt is not close enough
                qreal disp = delta / gradval;
                if (value > 4 || abs(disp) > 0.5) {
                    // (value > 4 ) => too close from the charge, go away fast
                    // |disp| > 0.5 => too fast, risk of going too far
                    disp = signbit(disp) ? -0.5 : 0.5;
                }
                pt += norm * disp;
//                pt += gradvec * (delta / gradval);
                moved++;
            }
        } while (it != end());

        return moved;
    }
};

static std::vector<Polygon> polygons;


class DrawingArea : public QWidget {
public:
    DrawingArea(QWidget *parent = nullptr) : QWidget(parent),
        zoom_(100.0),
        mouse_(0, 0),
        mousePressed_(false) {
    }

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

        QPainter p(this);

        QSizeF s = size() / 2.0;
        p.translate(QPoint(s.width(), s.height()));
        p.scale(zoom_, zoom_);
        s /= zoom_;

        if (mousePressed_) {
            // move first charge
            Charge &c = charges.front();
            QPointF transformed = p.transform().inverted().map(QPointF(mouse_));
            qDebug() << mouse_ << transformed;
            c.setX(transformed.x());
            c.setY(transformed.y());
        }

        if (polygons.empty()) {
            // compute polygons
            std::for_each(charges.begin(), charges.end(), [](const Charge &charge) {
                Polygon polygon(charge);
                while (polygon.moveVertices() || polygon.resample());
                polygons.push_back(polygon);
            });
        }

        QPen pen(p.pen());
        pen.setWidth(0);
        p.setPen(pen);

        std::for_each(polygons.begin(), polygons.end(), [&p](const Polygon &polygon) {
            // draw polygon
            QPointF prev = polygon.last();
            std::for_each(polygon.begin(), polygon.end(), [&](const QPointF &cur) {
                p.drawLine(prev, cur);
                prev = cur;
            });
        });
    }

    void mousePressEvent(QMouseEvent *me) override {
        qDebug() << __func__;
        mousePressed_ = true;
        mouseMoveEvent(me); // dirty
    }

    void mouseMoveEvent(QMouseEvent *me) override {
        qDebug() << __func__;
        mouse_ = me->pos();
        polygons.clear();
        update();
    }

    void mouseReleaseEvent(QMouseEvent *me) override {
        (void) me;
        qDebug() << __func__;
        mousePressed_ = false;
    }

private:
    qreal zoom_;
    QPoint mouse_;
    bool mousePressed_;
};

bool process() {
    bool did_something = false;

    qDebug() << __func__;

    std::for_each(polygons.begin(), polygons.begin() + 1, [&did_something](Polygon &polygon) {
//    std::for_each(polygons.begin(), polygons.end(), [&did_something](Polygon &polygon) {
        if (polygon.moveVertices() || polygon.resample()) {
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
    charges.push_back(Charge(1.0, 0.0, 2.0));
    charges.push_back(Charge(1.0, -2.0, -0.0));
    charges.push_back(Charge(1.0, 2.0, -0.0));
//    charges.push_back(Charge(1.0, 0.0, -2.0));

    qDebug() << fieldAt(0.0, 0.0) << gradientAt(0.0, 0.0);

#if 0
    // debug: init polygons to make the timer draw the steps
    std::for_each(charges.begin(), charges.end(), [](const Charge &charge) {
        polygons.push_back(Polygon(charge));
    });
#endif
    // gui
    DrawingArea da;
    da.resize(1280, 720);
    da.show();

    // timer
    Timer timer;
    timer.setInterval(100);
    QObject::connect(&timer, SIGNAL(timeout()), &da, SLOT(update()));
    timer.start();

    return a.exec();
}
