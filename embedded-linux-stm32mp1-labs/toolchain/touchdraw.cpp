// touchdraw.cpp
#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QVector>
#include <QPoint>

class DrawWidget : public QWidget {
public:
    DrawWidget(QWidget *parent = nullptr) : QWidget(parent) {
        setAttribute(Qt::WA_AcceptTouchEvents);
        setWindowTitle("Touch Draw");
    }

protected:
    void mousePressEvent(QMouseEvent *ev) override {
        points.append(ev->pos());
        update();
    }

    void mouseMoveEvent(QMouseEvent *ev) override {
        if (ev->buttons() & Qt::LeftButton) {
            points.append(ev->pos());
            update();
        }
    }

    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setPen(QPen(Qt::blue, 8, Qt::SolidLine, Qt::RoundCap));
        for (const QPoint &pt : points)
            p.drawPoint(pt);
    }

private:
    QVector<QPoint> points;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    DrawWidget w;
    w.showFullScreen();
    return app.exec();
}
