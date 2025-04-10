#ifndef PROJET_H
#define PROJET_H

#include <QString>
#include <QDate>
#include <QSqlQuery>
#include <QList>
#include <QMessageBox>

class Projet {
private:
    int idPr;               // ID_PR
    QString nomPr;          // NOM_PR
    QString statutPr;       // STATUT_PR
    QString complexite;     // COMPLEXITE
    int idClient;           // ID_CLIENT
    float budget;           // BUDGET
    int idEmployee;        // ID_EMPLOYEE
    QDate dateDebut;        // DATE_DEBUT
    QDate dateFin;          // DATE_FIN

public:
    // Constructeurs
    Projet();
    Projet(QString nom, QString statut, QString complexite, float budget,
           int idClient, int idEmployee, QDate dateDebut, QDate dateFin, QSqlQuery &query);

    // Getters
    int getIdPr() const;
    QString getNomPr() const;
    QString getStatutPr() const;
    QString getComplexite() const;
    float getBudget() const;
    int getIdClient() const;
    int getIdEmployee() const;
    QDate getDateDebut() const;
    QDate getDateFin() const;

    // Setters
    void setIdPr(int id);
    void setNomPr(QString nom);
    void setStatutPr(QString statut);
    void setComplexite(QString complexite);
    void setBudget(float budget);
    void setIdClient(int idClient);
    void setIdEmployee(int idEmployee);
    void setDateDebut(QDate dateDebut);
    void setDateFin(QDate dateFin);

    // Méthodes de validation
    static bool validateNomPr(const QString &nom, QString &error);
    static bool validateBudget(float budget, QString &error);
    static bool validateDates(const QDate &dateDebut, const QDate &dateFin, QString &error);
    // Opérations CRUD
    bool insertIntoDatabase(QSqlQuery &query);
    static bool deleteById(int id, QSqlQuery &query);
    static bool updateProjet(int id, const QString &column, const QString &newValue, QSqlQuery &query);

    // Méthodes de récupération
    static QList<Projet> fetchAllProjets(QSqlQuery &query);
    static Projet fetchProjetById(int id, QSqlQuery &query);
    static QString getClientNameById(int id, QSqlQuery &query);
    static QString getEmployeeNameById(int id, QSqlQuery &query);

    // Méthodes de tri et filtrage
    static QList<Projet> getProjetsSortedByName(bool ascending, QSqlQuery &query);
    static QList<Projet> getProjetsSortedByBudget(bool ascending, QSqlQuery &query);
    static QList<Projet> getProjetsByName(const QString &name, QSqlQuery &query);

    // Méthodes utilitaires
    static int getMaxIdPr(QSqlQuery &query);
    static QMap<int, QString> getAllClients(QSqlQuery &query);
    static QMap<int, QString> getAllEmployees(QSqlQuery &query);
};

#endif // PROJET_H
