#include <QApplication>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include <QTimer>
#include <QString>
#include <cstdio>

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
        // Fond noir
        setStyleSheet("background-color: black;");

        // Labels
        labelX = new QLabel("X = 0", this);
        labelY = new QLabel("Y = 0", this);
        labelZ = new QLabel("Z = 0", this);

        QString style = "color: white; font-size: 60px;";
        labelX->setStyleSheet("color: red;    font-size: 60px;");
        labelY->setStyleSheet("color: green;  font-size: 60px;");
        labelZ->setStyleSheet("color: blue;   font-size: 60px;");

        labelX->setAlignment(Qt::AlignCenter);
        labelY->setAlignment(Qt::AlignCenter);
        labelZ->setAlignment(Qt::AlignCenter);

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->addStretch();
        layout->addWidget(labelX);
        layout->addWidget(labelY);
        layout->addWidget(labelZ);
        layout->addStretch();

        // Timer qui met à jour toutes les 500ms
        QTimer *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &MPUWidget::update_values);
        timer->start(500);

        // Première lecture immédiate
        update_values();
    }

private slots:
    void update_values()
    {
        int x = read_value("/sys/bus/iio/devices/iio:device0/in_accel_x_raw");
        int y = read_value("/sys/bus/iio/devices/iio:device0/in_accel_y_raw");
        int z = read_value("/sys/bus/iio/devices/iio:device0/in_accel_z_raw");

        labelX->setText(QString("X = %1").arg(x));
        labelY->setText(QString("Y = %1").arg(y));
        labelZ->setText(QString("Z = %1").arg(z));
    }

private:
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
