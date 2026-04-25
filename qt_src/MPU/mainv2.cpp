#include <QApplication>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include <QTimer>
#include <QString>
#include <cstdio>

#define SENSITIVITY 16384.0f  // LSB/g pour ±2g

int read_value(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    int val;
    fscanf(f, "%d", &val);
    fclose(f);
    return val;
}

class MPUWidget : public QWidget
{
public:
    MPUWidget(QWidget *parent = nullptr) : QWidget(parent)
    {
        setStyleSheet("background-color: black;");

        labelTitle = new QLabel("MPU6050 - Acceleration", this);
        labelX = new QLabel("X = 0.00 g", this);
        labelY = new QLabel("Y = 0.00 g", this);
        labelZ = new QLabel("Z = 0.00 g", this);

        labelTitle->setStyleSheet("color: white; font-size: 35px;");
        labelX->setStyleSheet("color: red;   font-size: 55px;");
        labelY->setStyleSheet("color: green; font-size: 55px;");
        labelZ->setStyleSheet("color: blue;  font-size: 55px;");

        labelTitle->setAlignment(Qt::AlignCenter);
        labelX->setAlignment(Qt::AlignCenter);
        labelY->setAlignment(Qt::AlignCenter);
        labelZ->setAlignment(Qt::AlignCenter);

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->addStretch();
        layout->addWidget(labelTitle);
        layout->addSpacing(30);
        layout->addWidget(labelX);
        layout->addWidget(labelY);
        layout->addWidget(labelZ);
        layout->addStretch();

        QTimer *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &MPUWidget::update_values);
        timer->start(200);

        update_values();
    }

private slots:
    void update_values()
    {
        int raw_x = read_value("/sys/bus/iio/devices/iio:device0/in_accel_x_raw");
        int raw_y = read_value("/sys/bus/iio/devices/iio:device0/in_accel_y_raw");
        int raw_z = read_value("/sys/bus/iio/devices/iio:device0/in_accel_z_raw");

        float gx = raw_x / SENSITIVITY;
        float gy = raw_y / SENSITIVITY;
        float gz = raw_z / SENSITIVITY;

        labelX->setText(QString("X = %1 g").arg(gx, 0, 'f', 3));
        labelY->setText(QString("Y = %1 g").arg(gy, 0, 'f', 3));
        labelZ->setText(QString("Z = %1 g").arg(gz, 0, 'f', 3));
    }

private:
    QLabel *labelTitle;
    QLabel *labelX;
    QLabel *labelY;
    QLabel *labelZ;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MPUWidget w;
    w.showFullScreen();
    return app.exec();
}
