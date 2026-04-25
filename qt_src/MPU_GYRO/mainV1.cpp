#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <QTimer>
#include <QLabel>
#include <QVBoxLayout>
#include <cstdio>
#include <cmath>
#include <QElapsedTimer>

#define ACCEL_SCALE   0.000598f
#define GYRO_SCALE    0.001064724f
#define RAD_TO_DEG    57.2957795f
#define DEG_TO_RAD    0.0174533f
#define ALPHA         0.96f       // filtre complementaire

int read_value(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    int val;
    fscanf(f, "%d", &val);
    fclose(f);
    return val;
}

class Cube3D : public QWidget
{
public:
    float angleX = 0.0f;
    float angleY = 0.0f;
    float angleZ = 0.0f;

    Cube3D(QWidget *parent = nullptr) : QWidget(parent)
    {
        setStyleSheet("background-color: #0a0a1a;");
        setMinimumHeight(280);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        int cx = width() / 2;
        int cy = height() / 2;
        float s = qMin(width(), height()) * 0.3f;

        // 8 sommets du cube
        float verts[8][3] = {
            {-1,-1,-1}, { 1,-1,-1}, { 1, 1,-1}, {-1, 1,-1},
            {-1,-1, 1}, { 1,-1, 1}, { 1, 1, 1}, {-1, 1, 1}
        };

        // Rotation
        float rx = angleX * DEG_TO_RAD;
        float ry = angleY * DEG_TO_RAD;
        float rz = angleZ * DEG_TO_RAD;

        // Projette les 8 sommets en 2D
        QPointF pts[8];
        for (int i = 0; i < 8; i++) {
            float x = verts[i][0];
            float y = verts[i][1];
            float z = verts[i][2];

            // Rotation X
            float y1 = y * cos(rx) - z * sin(rx);
            float z1 = y * sin(rx) + z * cos(rx);
            y = y1; z = z1;

            // Rotation Y
            float x1 = x * cos(ry) + z * sin(ry);
            float z2 = -x * sin(ry) + z * cos(ry);
            x = x1; z = z2;

            // Rotation Z
            float x2 = x * cos(rz) - y * sin(rz);
            float y2 = x * sin(rz) + y * cos(rz);
            x = x2; y = y2;

            // Projection perspective
            float dist = 4.0f;
            float fz = 1.0f / (dist - z);
            pts[i] = QPointF(cx + x * fz * s * dist,
                             cy + y * fz * s * dist);
        }

        // 6 faces avec couleurs différentes
        struct Face { int v[4]; QColor color; };
        Face faces[] = {
            {{0,1,2,3}, QColor(200, 60,  60,  200)},  // arrière rouge
            {{4,5,6,7}, QColor(60,  200, 60,  200)},  // avant vert
            {{0,1,5,4}, QColor(60,  60,  200, 200)},  // bas bleu
            {{2,3,7,6}, QColor(200, 200, 60,  200)},  // haut jaune
            {{0,3,7,4}, QColor(200, 60,  200, 200)},  // gauche magenta
            {{1,2,6,5}, QColor(60,  200, 200, 200)},  // droite cyan
        };

        for (auto &face : faces) {
            QPolygonF poly;
            for (int j = 0; j < 4; j++)
                poly << pts[face.v[j]];
            p.setBrush(face.color);
            p.setPen(QPen(Qt::white, 1));
            p.drawPolygon(poly);
        }
    }
};

class MainWidget : public QWidget
{
public:
    MainWidget(QWidget *parent = nullptr) : QWidget(parent)
    {
        setStyleSheet("background-color: #0a0a1a;");

        cube = new Cube3D(this);

        labelX = new QLabel("X =  0.00°", this);
        labelY = new QLabel("Y =  0.00°", this);
        labelZ = new QLabel("Z =  0.00°", this);

        labelX->setStyleSheet("color: #ff5050; font-size: 28px;");
        labelY->setStyleSheet("color: #50ff50; font-size: 28px;");
        labelZ->setStyleSheet("color: #5096ff; font-size: 28px;");
        labelX->setAlignment(Qt::AlignCenter);
        labelY->setAlignment(Qt::AlignCenter);
        labelZ->setAlignment(Qt::AlignCenter);

        QLabel *title = new QLabel("MPU6050 - Orientation 3D", this);
        title->setStyleSheet("color: white; font-size: 24px; font-weight: bold;");
        title->setAlignment(Qt::AlignCenter);

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->addWidget(title);
        layout->addWidget(cube, 1);
        layout->addWidget(labelX);
        layout->addWidget(labelY);
        layout->addWidget(labelZ);

        // Timer 50ms = 20Hz
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

        // Lecture accéléromètre en g
        float ax = read_value("/sys/bus/iio/devices/iio:device0/in_accel_x_raw") * ACCEL_SCALE;
        float ay = read_value("/sys/bus/iio/devices/iio:device0/in_accel_y_raw") * ACCEL_SCALE;
        float az = read_value("/sys/bus/iio/devices/iio:device0/in_accel_z_raw") * ACCEL_SCALE;

        // Lecture gyroscope en deg/s
        float gx = read_value("/sys/bus/iio/devices/iio:device0/in_anglvel_x_raw") * GYRO_SCALE;
        float gy = read_value("/sys/bus/iio/devices/iio:device0/in_anglvel_y_raw") * GYRO_SCALE;

        // Angles depuis accéléromètre
        float accel_angleX = atan2(ay, az) * RAD_TO_DEG;
        float accel_angleY = atan2(-ax, sqrt(ay*ay + az*az)) * RAD_TO_DEG;

        // Filtre complémentaire
        cube->angleX = ALPHA * (cube->angleX + gx * dt) + (1.0f - ALPHA) * accel_angleX;
        cube->angleY = ALPHA * (cube->angleY + gy * dt) + (1.0f - ALPHA) * accel_angleY;

        cube->update();

        labelX->setText(QString("X = %1°").arg(cube->angleX, 0, 'f', 1));
        labelY->setText(QString("Y = %1°").arg(cube->angleY, 0, 'f', 1));
        labelZ->setText(QString("Z = %1°").arg(cube->angleZ, 0, 'f', 1));
    }

private:
    Cube3D   *cube;
    QLabel   *labelX, *labelY, *labelZ;
    QElapsedTimer lastTime;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWidget w;
    w.showFullScreen();
    return app.exec();
}
