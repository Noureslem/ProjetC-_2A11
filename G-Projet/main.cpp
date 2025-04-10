#include "mainwindow.h"
#include "connection.h"
#include <QApplication>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Create connection to database using the static method
    bool test = Database::connectDB();
    // Set application icon
    a.setWindowIcon(QIcon(":/Tools Interface/mon_icone.png"));

    if (test) {
        MainWindow w;
        w.show();
        return a.exec();
    } else {
        QMessageBox::critical(nullptr, "Database Error",
                              "Failed to connect to the database. The application will now exit.");
        return 1;
    }
}
