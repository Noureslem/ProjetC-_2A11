#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_mainwindow.h"
#include <QMainWindow>
#include <QMessageBox>
#include <QMap>
#include <QPair>
#include <QFileDialog>
#include <QPrinter>
#include <QTextDocument>
#include <QSqlQuery>
#include <QtCharts> // Include Qt Charts
#include <QChartView>
#include <QBarSet>
#include <QBarSeries>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include <QPieSeries>
// Add these includes at the top of your mainwindow.h file
#include <QPushButton>
#include <QMenu>
#include <QWidgetAction>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QLabel>
#include <QDateTime>
#include <QTimer>
#include <QRandomGenerator>
QT_BEGIN_NAMESPACE
namespace Ui {
class MW_projet;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
private slots:
    // Navigation des onglets
    void onTabChanged(int index);

    // Onglet 1: Liste des projets
    void on_tableWidget_cellDoubleClicked(int row, int column);
    void on_comboBox_currentIndexChanged(int index);
    void on_pushButton_supprimer_clicked(); // Supprimer un projet
    void on_lineEdit_search_textChanged(const QString &arg1);

    // Onglet 2: Ajouter un projet
    void on_pushButton_ajouter_clicked();
    void on_pushButton_annuler_clicked(); // Exporter en PDF

    // Onglet 3: Modifier un projet
    void on_comboBox_projet_currentIndexChanged(int index);
    void on_pushButton_charger_clicked();
    void on_pushButton_modifier_clicked();
private:
    Ui::MW_projet *ui;
    QSqlQuery query;
    QMap<QPair<int, int>, QString> originalValues; // Pour suivre les modifications dans le tableau

    // Charts
    QChartView *chartView;
    QChartView *pieChartView;

    // Méthodes d'initialisation
    void setupConnections();
    void setupTable();
    void setupStatisticsTab();

    // Méthodes de chargement des données
    void chargerProjets();
    void chargerComboBoxClients();
    void chargerComboBoxEmployees();
    void chargerComboBoxProjets();

    // Méthodes de gestion des formulaires
    void viderFormulaireAjout();
    void viderFormulaireModification();
    void remplirFormulaireModification(int projetId);

    // Méthodes d'exportation PDF
    bool exportProjectsToPdf(const QString &fileName);
    bool exportProjectToPdf(int projetId, const QString &fileName);

    // Méthodes pour créer les graphiques
    void createStatutChart(); // Graphique en barres
    void createStatutPieChart(); // Graphique en camembert

    // Méthode pour obtenir les données pour les graphiques
    QMap<QString, int> getProjectCountsByStatus();
    void searchProjects(const QString &searchText);
    // jdid
    // Notification system
    QPushButton* notificationButton;
    QMenu* notificationMenu;
    QList<QString> notifications;
    int unreadCount;
    void setupNotificationSystem();
    void addNotification(const QString& message);
    QWidget* createNotificationWidget();
    void updateNotificationButton();
protected:
    void resizeEvent(QResizeEvent* event) override {
        QMainWindow::resizeEvent(event);

        // Update notification button position
        if (notificationButton) {
            notificationButton->move(this->width() - 60, 10);
        }

        // Update search bar position
        QLineEdit* searchBar = findChild<QLineEdit*>("lineEdit_search");
        if (searchBar) {
            searchBar->move(this->width() - 320, 10);
        }
    }
};

#endif // MAINWINDOW_H

