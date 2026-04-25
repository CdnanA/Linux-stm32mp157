#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QPainter>
#include <QPen>
#include <QQueue>
#include <QString>
#include <cstdio>

#define SENSITIVITY 16384.0f
#define MAX_POINTS  100

int read_value(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    int val;
    fscanf(f, "%d", &val);
    fclose(f);
    return val;
}

// Widget qui dessine le graphique
class GraphWidget : public QWidget
{
public:
    GraphWidget(QWidget *parent = nullptr) : QWidget(parent)
    {
        setMinimumHeight(300);
        setStyleSheet("background-color: #1a1a2e;");
    }

    void addPoint(float x, float y, float z)
    {
        dataX.enqueue(x);
        dataY.enqueue(y);
        dataZ.enqueue(z);

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

        int w = width();
        int h = height();
        float midY = h / 2.0f;
        float scaleY = h / 4.0f; // ±2g visible

        // Grille
        p.setPen(QPen(QColor(60, 60, 80), 1, Qt::DashLine));
        p.drawLine(0, midY, w, midY);           // ligne 0g
        p.drawLine(0, midY - scaleY, w, midY - scaleY); // +1g
        p.drawLine(0, midY + scaleY, w, midY + scaleY); // -1g

        // Labels grille
        p.setPen(QColor(120, 120, 140));
        p.setFont(QFont("DejaVu Sans", 10));
        p.drawText(5, midY - scaleY - 2, "+1g");
        p.drawText(5, midY - 2, " 0g");
        p.drawText(5, midY + scaleY + 12, "-1g");

        // Dessine les courbes
        drawCurve(p, dataX, QColor(255, 80, 80),  w, midY, scaleY);  // X rouge
        drawCurve(p, dataY, QColor(80, 255, 80),  w, midY, scaleY);  // Y vert
        drawCurve(p, dataZ, QColor(80, 150, 255), w, midY, scaleY);  // Z bleu
    }

private:
    void drawCurve(QPainter &p, const QQueue<float> &data,
                   const QColor &color, int w, float midY, float scaleY)
    {
        if (data.size() < 2) return;

        p.setPen(QPen(color, 2));

        float stepX = (float)w / MAX_POINTS;
        int startX = w - data.size() * stepX;

        for (int i = 1; i < data.size(); i++) {
            float x1 = startX + (i - 1) * stepX;
            float y1 = midY - data[i-1] * scaleY;
            float x2 = startX + i * stepX;
            float y2 = midY - data[i] * scaleY;
            p.drawLine(x1, y1, x2, y2);
        }
    }

    QQueue<float> dataX, dataY, dataZ;
};

// Widget principal
class MPUWidget : public QWidget
{
public:
    MPUWidget(QWidget *parent = nullptr) : QWidget(parent)
    {
        setStyleSheet("background-color: #0f0f23;");

        // Titre
        QLabel *title = new QLabel("MPU6050 - Accélération", this);
        title->setStyleSheet("color: white; font-size: 28px; font-weight: bold;");
        title->setAlignment(Qt::AlignCenter);

        // Graphique
        graph = new GraphWidget(this);

        // Labels valeurs
        labelX = new QLabel("X = 0.000 g", this);
        labelY = new QLabel("Y = 0.000 g", this);
        labelZ = new QLabel("Z = 0.000 g", this);

        labelX->setStyleSheet("color: #ff5050; font-size: 32px;");
        labelY->setStyleSheet("color: #50ff50; font-size: 32px;");
        labelZ->setStyleSheet("color: #5096ff; font-size: 32px;");

        labelX->setAlignment(Qt::AlignCenter);
        labelY->setAlignment(Qt::AlignCenter);
        labelZ->setAlignment(Qt::AlignCenter);

        // Légende
        QHBoxLayout *legend = new QHBoxLayout();
        QLabel *legX = new QLabel("■ X", this);
        QLabel *legY = new QLabel("■ Y", this);
        QLabel *legZ = new QLabel("■ Z", this);
        legX->setStyleSheet("color: #ff5050; font-size: 20px;");
        legY->setStyleSheet("color: #50ff50; font-size: 20px;");
        legZ->setStyleSheet("color: #5096ff; font-size: 20px;");
        legX->setAlignment(Qt::AlignCenter);
        legY->setAlignment(Qt::AlignCenter);
        legZ->setAlignment(Qt::AlignCenter);
        legend->addWidget(legX);
        legend->addWidget(legY);
        legend->addWidget(legZ);

        // Layout principal
        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->addWidget(title);
        mainLayout->addWidget(graph, 1);
        mainLayout->addLayout(legend);
        mainLayout->addWidget(labelX);
        mainLayout->addWidget(labelY);
        mainLayout->addWidget(labelZ);

        // Timer
        QTimer *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &MPUWidget::update_values);
        timer->start(100);

        update_values();
    }

private slots:
    void update_values()
    {
        int rx = read_value("/sys/bus/iio/devices/iio:device0/in_accel_x_raw");
        int ry = read_value("/sys/bus/iio/devices/iio:device0/in_accel_y_raw");
        int rz = read_value("/sys/bus/iio/devices/iio:device0/in_accel_z_raw");

        float gx = rx / SENSITIVITY;
        float gy = ry / SENSITIVITY;
        float gz = rz / SENSITIVITY;

        graph->addPoint(gx, gy, gz);

        labelX->setText(QString("X = %1 g").arg(gx, 0, 'f', 3));
        labelY->setText(QString("Y = %1 g").arg(gy, 0, 'f', 3));
        labelZ->setText(QString("Z = %1 g").arg(gz, 0, 'f', 3));
    }

private:
    GraphWidget *graph;
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
