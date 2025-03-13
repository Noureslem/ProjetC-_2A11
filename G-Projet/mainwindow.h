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
#include <QSqlQuery>\

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

    // Méthodes d'initialisation
    void setupConnections();
    void setupTable();

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
};
#endif // MAINWINDOW_H
