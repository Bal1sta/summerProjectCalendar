#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

<<<<<<< HEAD
    app.setStyle("Fusion");

    MainWindow w;
    // w.show();
=======
    app.setStyle("Fusion"); // Рекомендуется Fusion + палитры

    MainWindow w;
    w.show();
>>>>>>> 33978bc2a58cb07d16ba6eb66d32f282a4862a9b

    return app.exec();
}
