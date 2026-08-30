#include "mainwindow.h"
#include "connection.h"

#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    if (!Connection::instance()->createConnect()) {
        qDebug() << "Connexion Oracle échouée.";
        return -1;
    }

    MainWindow w;
    w.show();

    return a.exec();
}
