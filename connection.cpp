#include "connection.h"
#include <QSqlError>
#include <QDebug>
#include <QProcessEnvironment>

Connection* Connection::p_instance = nullptr;

Connection::Connection()
{
    db = QSqlDatabase::addDatabase("QODBC");
}

Connection* Connection::instance()
{
    if (p_instance == nullptr) {
        p_instance = new Connection();
    }

    return p_instance;
}

bool Connection::createConnect()
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString uid = env.value("CF_DB_UID", "CENTRE_FORMATION");
    QString pwd = env.value("CF_DB_PWD");

    if (pwd.isEmpty()) {
        qDebug() << "Erreur : la variable d'environnement CF_DB_PWD n'est pas definie.";
        return false;
    }

    db.setDatabaseName(
        QString("Driver={Oracle in XE};"
                "Dbq=XE;"
                "Uid=%1;"
                "Pwd=%2;").arg(uid, pwd)
        );

    if (db.open()) {
        qDebug() << "Connexion à la base de données réussie";
        return true;
    }

    qDebug() << "Erreur de connexion :"
             << db.lastError().text();

    return false;
}

void Connection::closeConnection()
{
    if (db.isOpen()) {
        db.close();
    }
}

QSqlDatabase Connection::getDatabase()
{
    return db;
}

Connection::~Connection()
{
    closeConnection();
}
