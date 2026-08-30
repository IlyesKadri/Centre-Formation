#ifndef CONNECTION_H
#define CONNECTION_H

#include <QtSql/QSqlDatabase>
#include <QSqlQuery>

class Connection
{
public:
    static Connection* instance();
    bool createConnect();
    void closeConnection();
    QSqlDatabase getDatabase();

private:
    Connection();
    ~Connection();

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    static Connection* p_instance;
    QSqlDatabase db;
};

#endif // CONNECTION_H
