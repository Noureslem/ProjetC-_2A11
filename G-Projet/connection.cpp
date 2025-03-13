#include "connection.h"

bool Database::connectDB() {
    QSqlDatabase db = QSqlDatabase::addDatabase("QODBC");
    db.setDatabaseName("Source-Projet2A");  // Data source name
    db.setUserName("stratedgge");           // Username
    db.setPassword("consulting");           // Password

    if (!db.open()) {
        qDebug() << "Database connection failed: " << db.lastError().text();
        return false;
    }

    qDebug() << "Database connected successfully!";
    return true;
}
