#include <QApplication>
#include <QWidget>
#include <QStackedWidget>
#include <QPainter>
#include <QTimer>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QElapsedTimer>
#include <QMouseEvent>
#include <QQueue>
#include <cstdio>
#include <cmath>

#define ACCEL_SCALE   0.000598f
#define GYRO_SCALE    0.001064724f
#define RAD_TO_DEG    57.2957795f
#define DEG_TO_RAD    0.0174533f
#define ALPHA         0.96f
#define MAX_POINTS    100

int read_value(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    int val;
    fscanf(f, "%d", &val);
    fclose(f);
    return val;
}

// ============================================================
// PAGE 1 : Graphique accéléromètre
// ============================================================
class GraphWidget : public QWidget
{
public:
    GraphWidget(QWidget *parent = nullptr) : QWidget(parent)
    {
        setStyleSheet("background-color: #1a1a2e;");
        setMinimumHeight(250);
    }

    void addPoint(float x, float y, float z)
    {
        dataX.enqueue(x); dataY.enqueue(y); dataZ.enqueue(z);
        if (dataX.size() > MAX_POINTS) dataX.dequeue();
        if (dataY.size() > MAX_POINTS) dataY.dequeue();
        if (dataZ.size() > MAX_POINTS) dataZ.dequeue();
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        int w = width(), h = height();
        float midY = h / 2.0f;
        float scaleY = h / 4.0f;

        p.setPen(QPen(QColor(60,60,80), 1, Qt::DashLine));
        p.drawLine(0, midY, w, midY);
        p.drawLine(0, midY - scaleY, w, midY - scaleY);
        p.drawLine(0, midY + scaleY, w, midY + scaleY);

        p.setPen(QColor(120,120,140));
        p.setFont(QFont("DejaVu Sans", 10));
        p.drawText(5, midY - scaleY - 2, "+1g");
        p.drawText(5, midY - 2, " 0g");
        p.drawText(5, midY + scaleY + 12, "-1g");

        drawCurve(p, dataX, QColor(255,80,80),  w, midY, scaleY);
        drawCurve(p, dataY, QColor(80,255,80),  w, midY, scaleY);
        drawCurve(p, dataZ, QColor(80,150,255), w, midY, scaleY);
    }

private:
    void drawCurve(QPainter &p, const QQueue<float> &data,
                   const QColor &color, int w, float midY, float scaleY)
    {
        if (data.size() < 2) return;
        p.setPen(QPen(color, 2));
        float stepX = (float)w / MAX_POINTS;
        float startX = w - data.size() * stepX;
        for (int i = 1; i < data.size(); i++) {
            p.drawLine(startX + (i-1)*stepX, midY - data[i-1]*scaleY,
                       startX +  i   *stepX, midY - data[i]  *scaleY);
        }
    }

    QQueue<float> dataX, dataY, dataZ;
};

class AccelPage : public QWidget
{
public:
    AccelPage(QWidget *parent = nullptr) : QWidget(parent)
    {
        setStyleSheet("background-color: #0f0f23;");

        QLabel *title = new QLabel("Accéléromètre MPU6050", this);
        title->setStyleSheet("color: white; font-size: 22px; font-weight: bold;");
        title->setAlignment(Qt::AlignCenter);

        graph = new GraphWidget(this);

        labelX = new QLabel("X = 0.000 g", this);
        labelY = new QLabel("Y = 0.000 g", this);
        labelZ = new QLabel("Z = 0.000 g", this);
        labelX->setStyleSheet("color: #ff5050; font-size: 28px;");
        labelY->setStyleSheet("color: #50ff50; font-size: 28px;");
        labelZ->setStyleSheet("color: #5096ff; font-size: 28px;");
        labelX->setAlignment(Qt::AlignCenter);
        labelY->setAlignment(Qt::AlignCenter);
        labelZ->setAlignment(Qt::AlignCenter);

        // Légende
        QHBoxLayout *legend = new QHBoxLayout();
        QLabel *lx = new QLabel("■ X", this);
        QLabel *ly = new QLabel("■ Y", this);
        QLabel *lz = new QLabel("■ Z", this);
        lx->setStyleSheet("color: #ff5050; font-size: 18px;");
        ly->setStyleSheet("color: #50ff50; font-size: 18px;");
        lz->setStyleSheet("color: #5096ff; font-size: 18px;");
        lx->setAlignment(Qt::AlignCenter);
        ly->setAlignment(Qt::AlignCenter);
        lz->setAlignment(Qt::AlignCenter);
        legend->addWidget(lx);
        legend->addWidget(ly);
        legend->addWidget(lz);

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->addWidget(title);
        layout->addWidget(graph, 1);
        layout->addLayout(legend);
        layout->addWidget(labelX);
        layout->addWidget(labelY);
        layout->addWidget(labelZ);
    }

    void update_data(float gx, float gy, float gz)
    {
        graph->addPoint(gx, gy, gz);
        labelX->setText(QString("X = %1 g").arg(gx, 0, 'f', 3));
        labelY->setText(QString("Y = %1 g").arg(gy, 0, 'f', 3));
        labelZ->setText(QString("Z = %1 g").arg(gz, 0, 'f', 3));
    }

private:
    GraphWidget *graph;
    QLabel *labelX, *labelY, *labelZ;
};

// ============================================================
// PAGE 2 : Avion 3D
// ============================================================
struct Point3D { float x, y, z; };

Point3D rotateX(Point3D p, float a) {
    return {p.x, p.y*cos(a)-p.z*sin(a), p.y*sin(a)+p.z*cos(a)};
}
Point3D rotateY(Point3D p, float a) {
    return {p.x*cos(a)+p.z*sin(a), p.y, -p.x*sin(a)+p.z*cos(a)};
}
Point3D rotateZ(Point3D p, float a) {
    return {p.x*cos(a)-p.y*sin(a), p.x*sin(a)+p.y*cos(a), p.z};
}
QPointF project(Point3D p, float cx, float cy, float scale) {
    float dist = 5.0f;
    float fz = 1.0f / (dist - p.z);
    return QPointF(cx + p.x*fz*scale*dist, cy + p.y*fz*scale*dist);
}

class PlaneWidget : public QWidget
{
public:
    float angleX = 0.0f;
    float angleY = 0.0f;
    float angleZ = 0.0f;

    PlaneWidget(QWidget *parent = nullptr) : QWidget(parent)
    {
        setStyleSheet("background-color: #0a0f1e;");
        setMinimumHeight(280);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        float cx = width()/2.0f, cy = height()/2.0f;
        float s  = qMin(width(), height()) * 0.35f;
        float rx = angleX * DEG_TO_RAD;
        float ry = angleY * DEG_TO_RAD;
        float rz = angleZ * DEG_TO_RAD;

        QVector<Point3D> fuselage = {
            {-2.0f, 0.0f, 0.0f}, {0.5f, 0.1f, 0.0f},
            {1.5f,  0.1f, 0.0f}, {2.0f, 0.0f, 0.0f},
            {1.5f, -0.1f, 0.0f}, {0.5f,-0.1f, 0.0f},
        };
        QVector<Point3D> wingL = {
            {0.0f,0.0f,0.0f},{-0.3f,0.0f,-2.0f},
            {0.8f,0.0f,-2.0f},{1.0f,0.0f,0.0f},
        };
        QVector<Point3D> wingR = {
            {0.0f,0.0f,0.0f},{-0.3f,0.0f,2.0f},
            {0.8f,0.0f,2.0f},{1.0f,0.0f,0.0f},
        };
        QVector<Point3D> tailH = {
            {1.5f,0.0f,0.0f},{1.3f,0.0f,-0.8f},
            {2.0f,0.0f,-0.8f},{2.0f,0.0f,0.0f},
        };
        QVector<Point3D> tailH2 = {
            {1.5f,0.0f,0.0f},{1.3f,0.0f,0.8f},
            {2.0f,0.0f,0.8f},{2.0f,0.0f,0.0f},
        };
        QVector<Point3D> tailV = {
            {1.5f,0.0f,0.0f},{1.5f,-0.8f,0.0f},
            {2.0f,-0.8f,0.0f},{2.0f,0.0f,0.0f},
        };

        auto transform = [&](QVector<Point3D> &pts) -> QPolygonF {
            QPolygonF poly;
            for (auto &pt : pts) {
                Point3D r = rotateX(pt, rx);
                r = rotateY(r, ry);
                r = rotateZ(r, rz);
                poly << project(r, cx, cy, s);
            }
            return poly;
        };

        // Horizon
        p.setPen(QPen(QColor(40,40,80), 1, Qt::DashLine));
        p.drawLine(0, cy, width(), cy);
        p.setPen(QColor(60,60,100));
        p.setFont(QFont("DejaVu Sans", 10));
        p.drawText(5, cy-3, "horizon");

        p.setPen(QPen(Qt::white, 2));
        p.setBrush(QColor(180,180,200,220));
        p.drawPolygon(transform(fuselage));
        p.setBrush(QColor(100,150,220,200));
        p.drawPolygon(transform(wingL));
        p.drawPolygon(transform(wingR));
        p.setBrush(QColor(80,120,180,200));
        p.drawPolygon(transform(tailH));
        p.drawPolygon(transform(tailH2));
        p.setBrush(QColor(60,100,160,200));
        p.drawPolygon(transform(tailV));
    }
};

class PlanePage : public QWidget
{
public:
    float angleX = 0.0f;
    float angleY = 0.0f;
    float angleZ = 0.0f;

    PlanePage(QWidget *parent = nullptr) : QWidget(parent)
    {
        setStyleSheet("background-color: #0a0f1e;");

        QLabel *title = new QLabel("Orientation Avion 3D", this);
        title->setStyleSheet("color: white; font-size: 22px; font-weight: bold;");
        title->setAlignment(Qt::AlignCenter);

        plane = new PlaneWidget(this);

        labelPitch = new QLabel("Pitch = 0.0°", this);
        labelRoll  = new QLabel("Roll  = 0.0°", this);
        labelYaw   = new QLabel("Yaw   = 0.0°", this);
        labelPitch->setStyleSheet("color: #ff5050; font-size: 26px;");
        labelRoll ->setStyleSheet("color: #50ff50; font-size: 26px;");
        labelYaw  ->setStyleSheet("color: #5096ff; font-size: 26px;");
        labelPitch->setAlignment(Qt::AlignCenter);
        labelRoll ->setAlignment(Qt::AlignCenter);
        labelYaw  ->setAlignment(Qt::AlignCenter);

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->addWidget(title);
        layout->addWidget(plane, 1);
        layout->addWidget(labelPitch);
        layout->addWidget(labelRoll);
        layout->addWidget(labelYaw);
    }

    void update_data(float pitch, float roll, float yaw)
    {
        plane->angleX = pitch;
        plane->angleY = roll;
        plane->angleZ = yaw;
        plane->update();
        labelPitch->setText(QString("Pitch = %1°").arg(pitch, 0, 'f', 1));
        labelRoll ->setText(QString("Roll  = %1°").arg(roll,  0, 'f', 1));
        labelYaw  ->setText(QString("Yaw   = %1°").arg(yaw,   0, 'f', 1));
    }

private:
    PlaneWidget *plane;
    QLabel *labelPitch, *labelRoll, *labelYaw;
};

// ============================================================
// WIDGET PRINCIPAL avec swipe
// ============================================================
class SwipeWidget : public QWidget
{
public:
    SwipeWidget(QWidget *parent = nullptr) : QWidget(parent)
    {
        stack = new QStackedWidget(this);

        accelPage = new AccelPage(this);
        planePage = new PlanePage(this);

        stack->addWidget(accelPage);   // page 0
        stack->addWidget(planePage);   // page 1

        // Indicateurs de page (points en bas)
        dot0 = new QLabel("●", this);
        dot1 = new QLabel("○", this);
        dot0->setStyleSheet("color: white; font-size: 20px;");
        dot1->setStyleSheet("color: gray;  font-size: 20px;");
        dot0->setAlignment(Qt::AlignCenter);
        dot1->setAlignment(Qt::AlignCenter);

        QHBoxLayout *dots = new QHBoxLayout();
        dots->addWidget(dot0);
        dots->addWidget(dot1);

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setContentsMargins(0,0,0,0);
        layout->setSpacing(0);
        layout->addWidget(stack, 1);
        layout->addLayout(dots);

        // Timer IMU
        QTimer *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &SwipeWidget::update_imu);
        timer->start(50);
        lastTime.start();
    }

protected:
    // Détection du swipe
    void mousePressEvent(QMouseEvent *ev) override
    {
        swipeStartX = ev->x();
    }

    void mouseReleaseEvent(QMouseEvent *ev) override
    {
        int diff = ev->x() - swipeStartX;
        int current = stack->currentIndex();

        if (diff < -80 && current < stack->count() - 1) {
            // Swipe gauche → page suivante
            stack->setCurrentIndex(current + 1);
            updateDots();
        } else if (diff > 80 && current > 0) {
            // Swipe droite → page précédente
            stack->setCurrentIndex(current - 1);
            updateDots();
        }
    }

private slots:
    void update_imu()
    {
        float dt = lastTime.elapsed() / 1000.0f;
        lastTime.restart();

        float ax = read_value("/sys/bus/iio/devices/iio:device0/in_accel_x_raw") * ACCEL_SCALE;
        float ay = read_value("/sys/bus/iio/devices/iio:device0/in_accel_y_raw") * ACCEL_SCALE;
        float az = read_value("/sys/bus/iio/devices/iio:device0/in_accel_z_raw") * ACCEL_SCALE;

        float gx = read_value("/sys/bus/iio/devices/iio:device0/in_anglvel_x_raw") * GYRO_SCALE;
        float gy = read_value("/sys/bus/iio/devices/iio:device0/in_anglvel_y_raw") * GYRO_SCALE;
        float gz = read_value("/sys/bus/iio/devices/iio:device0/in_anglvel_z_raw") * GYRO_SCALE;

        // Accéléromètre en g
        accelPage->update_data(ax, ay, az);

        // Filtre complémentaire pour l'avion
        float accel_pitch = atan2(ay, az) * RAD_TO_DEG;
        float accel_roll  = atan2(-ax, sqrt(ay*ay + az*az)) * RAD_TO_DEG;

        pitch = ALPHA * (pitch + gx * dt) + (1.0f - ALPHA) * accel_pitch;
        roll  = ALPHA * (roll  + gy * dt) + (1.0f - ALPHA) * accel_roll;
        yaw  += gz * dt;

        planePage->update_data(pitch, roll, yaw);
    }

    void updateDots()
    {
        int idx = stack->currentIndex();
        dot0->setStyleSheet(idx == 0 ? "color: white; font-size: 20px;"
                                      : "color: gray;  font-size: 20px;");
        dot1->setStyleSheet(idx == 1 ? "color: white; font-size: 20px;"
                                      : "color: gray;  font-size: 20px;");
    }

private:
    QStackedWidget *stack;
    AccelPage      *accelPage;
    PlanePage      *planePage;
    QLabel         *dot0, *dot1;
    QElapsedTimer   lastTime;
    float pitch = 0, roll = 0, yaw = 0;
    int swipeStartX = 0;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    SwipeWidget w;
    w.showFullScreen();
    return app.exec();
}
