#ifndef DATABASE_H
#define DATABASE_H

#include <QSqlDatabase>
#include <QSqlError>
#include <QDebug>

class Database {
public:
    static bool connectDB();
};

#endif // DATABASE_H
