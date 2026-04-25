#include <QApplication>
#include <QLabel>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QLabel label("Hello Qt!");
    label.setAlignment(Qt::AlignCenter);
    label.setStyleSheet("background-color: red; color: white; font-size: 60px;");
    label.showFullScreen();
    return app.exec();
}
