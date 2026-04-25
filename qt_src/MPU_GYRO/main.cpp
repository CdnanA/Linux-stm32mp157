#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <QTimer>
#include <QLabel>
#include <QVBoxLayout>
#include <QElapsedTimer>
#include <cstdio>
#include <cmath>

#define ACCEL_SCALE   0.000598f
#define GYRO_SCALE    0.001064724f
#define RAD_TO_DEG    57.2957795f
#define DEG_TO_RAD    0.0174533f
#define ALPHA         0.96f

int read_value(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    int val;
    fscanf(f, "%d", &val);
    fclose(f);
    return val;
}

// Rotation d'un point 3D
struct Point3D { float x, y, z; };

Point3D rotateX(Point3D p, float a) {
    return {p.x,
            p.y * cos(a) - p.z * sin(a),
            p.y * sin(a) + p.z * cos(a)};
}
Point3D rotateY(Point3D p, float a) {
    return { p.x * cos(a) + p.z * sin(a),
             p.y,
            -p.x * sin(a) + p.z * cos(a)};
}
Point3D rotateZ(Point3D p, float a) {
    return {p.x * cos(a) - p.y * sin(a),
            p.x * sin(a) + p.y * cos(a),
            p.z};
}

QPointF project(Point3D p, float cx, float cy, float scale) {
    float dist = 5.0f;
    float fz = 1.0f / (dist - p.z);
    return QPointF(cx + p.x * fz * scale * dist,
                   cy + p.y * fz * scale * dist);
}

class PlaneWidget : public QWidget
{
public:
    float angleX = 0.0f;  // pitch
    float angleY = 0.0f;  // roll
    float angleZ = 0.0f;  // yaw

    PlaneWidget(QWidget *parent = nullptr) : QWidget(parent)
    {
        setStyleSheet("background-color: #0a0f1e;");
        setMinimumHeight(300);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        float cx = width() / 2.0f;
        float cy = height() / 2.0f;
        float s  = qMin(width(), height()) * 0.35f;

        float rx = angleX * DEG_TO_RAD;
        float ry = angleY * DEG_TO_RAD;
        float rz = angleZ * DEG_TO_RAD;

        // Définition de l'avion en 3D
        // Fuselage (corps)
        QVector<Point3D> fuselage = {
            {-2.0f,  0.0f,  0.0f},  // nez
            { 0.5f,  0.1f,  0.0f},
            { 1.5f,  0.1f,  0.0f},
            { 2.0f,  0.0f,  0.0f},  // queue
            { 1.5f, -0.1f,  0.0f},
            { 0.5f, -0.1f,  0.0f},
        };

        // Aile gauche
        QVector<Point3D> wingLeft = {
            { 0.0f,  0.0f,  0.0f},
            {-0.3f,  0.0f, -2.0f},
            { 0.8f,  0.0f, -2.0f},
            { 1.0f,  0.0f,  0.0f},
        };

        // Aile droite
        QVector<Point3D> wingRight = {
            { 0.0f,  0.0f,  0.0f},
            {-0.3f,  0.0f,  2.0f},
            { 0.8f,  0.0f,  2.0f},
            { 1.0f,  0.0f,  0.0f},
        };

        // Empennage horizontal (petite aile arrière)
        QVector<Point3D> tailH = {
            { 1.5f,  0.0f,  0.0f},
            { 1.3f,  0.0f, -0.8f},
            { 2.0f,  0.0f, -0.8f},
            { 2.0f,  0.0f,  0.0f},
        };
        QVector<Point3D> tailH2 = {
            { 1.5f,  0.0f,  0.0f},
            { 1.3f,  0.0f,  0.8f},
            { 2.0f,  0.0f,  0.8f},
            { 2.0f,  0.0f,  0.0f},
        };

        // Empennage vertical
        QVector<Point3D> tailV = {
            { 1.5f,  0.0f,  0.0f},
            { 1.5f, -0.8f,  0.0f},
            { 2.0f, -0.8f,  0.0f},
            { 2.0f,  0.0f,  0.0f},
        };

        // Applique les rotations et projette
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

        // Dessine le sol (horizon)
        p.setPen(QPen(QColor(40, 40, 80), 1, Qt::DashLine));
        p.drawLine(0, cy, width(), cy);
        p.setPen(QColor(60, 60, 100));
        p.setFont(QFont("DejaVu Sans", 10));
        p.drawText(5, cy - 3, "horizon");

        // Dessine l'avion
        p.setPen(QPen(Qt::white, 2));

        // Corps
        p.setBrush(QColor(180, 180, 200, 220));
        p.drawPolygon(transform(fuselage));

        // Ailes
        p.setBrush(QColor(100, 150, 220, 200));
        p.drawPolygon(transform(wingLeft));
        p.drawPolygon(transform(wingRight));

        // Empennages
        p.setBrush(QColor(80, 120, 180, 200));
        p.drawPolygon(transform(tailH));
        p.drawPolygon(transform(tailH2));

        // Empennage vertical
        p.setBrush(QColor(60, 100, 160, 200));
        p.drawPolygon(transform(tailV));

        // Indicateur direction nez
        Point3D nose = {-2.0f, 0.0f, 0.0f};
        Point3D rNose = rotateX(nose, rx);
        rNose = rotateY(rNose, ry);
        rNose = rotateZ(rNose, rz);
        QPointF noseP = project(rNose, cx, cy, s);
        p.setPen(QPen(QColor(255, 200, 0), 2));
        p.setBrush(QColor(255, 200, 0));
        p.drawEllipse(noseP, 5, 5);
    }
};

class MainWidget : public QWidget
{
public:
    MainWidget(QWidget *parent = nullptr) : QWidget(parent)
    {
        setStyleSheet("background-color: #0a0f1e;");

        QLabel *title = new QLabel("MPU6050 - Orientation Avion", this);
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

        QTimer *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &MainWidget::update_imu);
        timer->start(50);

        lastTime.start();
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

        float accel_pitch = atan2(ay, az) * RAD_TO_DEG;
        float accel_roll  = atan2(-ax, sqrt(ay*ay + az*az)) * RAD_TO_DEG;

        // Filtre complémentaire
        plane->angleX = ALPHA * (plane->angleX + gx * dt) + (1.0f - ALPHA) * accel_pitch;
        plane->angleY = ALPHA * (plane->angleY + gy * dt) + (1.0f - ALPHA) * accel_roll;
        plane->angleZ += gz * dt;  // yaw intégré gyro seul

        plane->update();

        labelPitch->setText(QString("Pitch = %1°").arg(plane->angleX, 0, 'f', 1));
        labelRoll ->setText(QString("Roll  = %1°").arg(plane->angleY, 0, 'f', 1));
        labelYaw  ->setText(QString("Yaw   = %1°").arg(plane->angleZ, 0, 'f', 1));
    }

private:
    PlaneWidget *plane;
    QLabel *labelPitch, *labelRoll, *labelYaw;
    QElapsedTimer lastTime;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWidget w;
    w.showFullScreen();
    return app.exec();
}
