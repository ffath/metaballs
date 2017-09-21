#include <QApplication>
#include <QPainter>
#include <QPointF>
#include <QPolygonF>
#include <QRectF>
#include <QSizeF>
#include <QTimer>
#include <QWidget>

#include <vector>
#include <list>

#include <math.h>

#include <QDebug>



class Charge : public QPointF {
public:
    Charge(qreal value) : Charge(value, 0.0, 0.0) {}
    Charge(qreal charge, qreal x, qreal y) : QPointF(x, y), charge_(charge) {}
    Charge(const Charge &other) : QPointF(other), charge_(other.charge_) {}

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

std::vector<Charge> charges;
std::vector<QPolygonF> polygons;

qreal fieldAt(qreal x, qreal y) {
    qreal value = 0.0;
    std::for_each(charges.begin(), charges.end(), [=, &value](const Charge &charge){ value += charge.valueAt(x, y);});
    qDebug() << __func__ << x << y << value;
    return value;
}

QPointF gradientAt(qreal x, qreal y) {
    QPointF gradient(0.0, 0.0);
    std::for_each(charges.begin(), charges.end(), [=, &gradient](const Charge &charge){
        gradient += charge.gradientAt(x, y);
    });
    qDebug() << __func__ << x << y << gradient << sqrt(QPointF::dotProduct(gradient, gradient));
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

        std::for_each(polygons.begin(), polygons.end(), [&p](const QPolygonF &polygon) {
            p.drawPolygon(polygon);
        });
#if 0
        qreal x, y;
        QPen pen(p.pen());
//        pen.setCapStyle(Qt::RoundCap);
        pen.setWidth(0);
        p.setPen(pen);

//        qreal alpha;
//        for (alpha = 0.0; alpha < 2.0 * M_PI; alpha += M_PI / 8.0) {
//            x = cos(alpha);
//            y = sin(alpha);

//            qreal field = fieldAt(x, y);
//            QPointF gradient = gradientAt(x, y);
//            QPointF pt(x, y);
//            p.drawLine(pt, pt + (gradient / (8.0)));
//        }

        for (x = -s.width(); x < s.width(); x += 0.25) {
            for (y = -s.height(); y < s.height(); y += 0.25) {
                qreal field = fieldAt(x, y);

                if (field == 0.0)
                    continue;

                QPointF gradient = gradientAt(x, y);
                QPointF p1(x, y);
                QPointF p2(p1 + gradient / 8.0);
                p.drawLine(p1, p2);
            }
        }
#endif
    }

private:
    qreal zoom_;
};

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    DrawingArea da;

    // init charges
    charges.push_back(Charge(1.0, 0.0, 0.0));
//    charges.push_back(Charge(1.0, -1.5, 0.0));
//    charges.push_back(Charge(1.0, 1.5, 0.0));

    // init polygons
    std::for_each(charges.begin(), charges.end(), [](const Charge &charge){
        QRectF r(-0.5, -0.5, 1.0, 1.0);
        r.moveCenter(charge);
        polygons.push_back(QPolygonF(r));
    });

    da.resize(800, 600);
    da.show();

    return a.exec();
}
