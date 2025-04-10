#include "projet.h"
#include <QDebug>
#include <QSqlError>

// Constructeur par défaut
Projet::Projet()
    : idPr(0), nomPr(""), statutPr(""), complexite(""), idClient(0),
      budget(0), idEmployee(0), dateDebut(QDate::currentDate()),
      dateFin(QDate::currentDate())
{
}

// Constructeur avec paramètres
Projet::Projet(QString nom, QString statut, QString complexite, float budget,
               int idClient, int idEmployee, QDate dateDebut, QDate dateFin, QSqlQuery &query)
    : idPr(getMaxIdPr(query) + 1), // Générer le prochain ID_PR
      nomPr(nom),
      statutPr(statut),
      complexite(complexite),
      idClient(idClient),
      budget(budget),
      idEmployee(idEmployee),
      dateDebut(dateDebut),
      dateFin(dateFin)
{
}

// Getters
int Projet::getIdPr() const { return idPr; }
QString Projet::getNomPr() const { return nomPr; }
QString Projet::getStatutPr() const { return statutPr; }
QString Projet::getComplexite() const { return complexite; }
float Projet::getBudget() const { return budget; }
int Projet::getIdClient() const { return idClient; }
int Projet::getIdEmployee() const { return idEmployee; }
QDate Projet::getDateDebut() const { return dateDebut; }
QDate Projet::getDateFin() const { return dateFin; }

// Setters
void Projet::setIdPr(int id) { idPr = id; }
void Projet::setNomPr(QString nom) { nomPr = nom; }
void Projet::setStatutPr(QString statut) { statutPr = statut; }
void Projet::setComplexite(QString comp) { complexite = comp; }
void Projet::setBudget(float budg) { budget = budg; }
void Projet::setIdClient(int id) { idClient = id; }
void Projet::setIdEmployee(int id) { idEmployee = id; }
void Projet::setDateDebut(QDate date) { dateDebut = date; }
void Projet::setDateFin(QDate date) { dateFin = date; }

// Méthodes de validation
bool Projet::validateNomPr(const QString &nom, QString &error) {
    if (nom.isEmpty()) {
        error = "Le nom du projet ne peut pas etre vide.";
        return false;
    }
    if (nom.length() > 20) {
        error = "Le nom du projet ne doit pas dépasser 20 caractères.";
        return false;
    }
    return true;
}

bool Projet::validateBudget(float budget, QString &error) {
    if (budget <= 0) {
        error = "Le budget doit etre supérieur à zéro.";
        return false;
    }
    return true;
}

bool Projet::validateDates(const QDate &dateDebut, const QDate &dateFin, QString &error) {
    if (dateDebut > dateFin) {
        error = "La date de début doit etre antérieure à la date de fin.";
        return false;
    }
    return true;
}

// Insérer un projet dans la base de données
bool Projet::insertIntoDatabase(QSqlQuery &query) {
    QString error;
    if (!validateNomPr(nomPr, error)) {
        QMessageBox::warning(nullptr, "Erreur", error);
        return false;
    }
    if (!validateBudget(budget, error)) {
        QMessageBox::warning(nullptr, "Erreur", error);
        return false;
    }
    if (!validateDates(dateDebut, dateFin, error)) {
        QMessageBox::warning(nullptr, "Erreur", error);
        return false;
    }

    query.prepare("INSERT INTO PROJET (ID_PR, NOM_PR, STATUT_PR, COMPLEXITE, ID_CLIENT, "
                 "BUDGET, ID_EMPLOYEE, DATE_DEBUT, DATE_FIN) "
                 "VALUES (:id, :nom, :statut, :complexite, :idClient, "
                 ":budget, :idEmployee, :dateDebut, :dateFin)");

    query.bindValue(":id", idPr);
    query.bindValue(":nom", nomPr);
    query.bindValue(":statut", statutPr);
    query.bindValue(":complexite", complexite);
    query.bindValue(":idClient", idClient);
    query.bindValue(":budget", budget);
    query.bindValue(":idEmployee", idEmployee);
    query.bindValue(":dateDebut", dateDebut);
    query.bindValue(":dateFin", dateFin);

    if (query.exec()) {
        qDebug() << "Projet ajouté à la base de données";
        return true;
    } else {
        qDebug() << "Erreur lors de l'insertion du projet:" << query.lastError().text();
        return false;
    }
}

// Supprimer un projet par ID
bool Projet::deleteById(int id, QSqlQuery &query) {
    query.prepare("DELETE FROM PROJET WHERE ID_PR = :id");
    query.bindValue(":id", id);

    if (query.exec()) {
        if (query.numRowsAffected() > 0) {
            qDebug() << "Projet supprimé avec succès!";
            return true;
        } else {
            qDebug() << "Aucun projet trouvé avec l'ID:" << id;
            return false;
        }
    } else {
        qDebug() << "Erreur lors de la suppression du projet:" << query.lastError().text();
        return false;
    }
}
// Mettre à jour un projet
bool Projet::updateProjet(int id, const QString &column, const QString &newValue, QSqlQuery &query) {
    if (column == "DATE_DEBUT" || column == "DATE_FIN") {
        QDate date = QDate::fromString(newValue, "yyyy-MM-dd");
        if (!date.isValid()) {
            qDebug() << "Erreur : Format de date invalide ->" << newValue;
            return false;
        }

        // Formater la date pour Oracle (DD-MM-YYYY)
        QString formattedDate = date.toString("dd-MM-yyyy");

        query.prepare(QString("UPDATE PROJET SET %1 = TO_DATE(:newValue, 'DD-MM-YYYY') WHERE ID_PR = :id").arg(column));
        query.bindValue(":newValue", formattedDate);
    } else {
        query.prepare(QString("UPDATE PROJET SET %1 = :newValue WHERE ID_PR = :id").arg(column));
        query.bindValue(":newValue", newValue);
    }

    query.bindValue(":id", id); // Ajoute ça pour éviter l'oubli

    if (query.exec()) {
        if (query.numRowsAffected() > 0) {
            qDebug() << "Projet mis à jour avec succès!";
            return true;
        } else {
            qDebug() << "Aucun projet trouvé avec l'ID:" << id;
            return false;
        }
    } else {
        qDebug() << "Erreur lors de la mise à jour du projet:" << query.lastError().text();
        return false;
    }
}


// Récupérer tous les projets
QList<Projet> Projet::fetchAllProjets(QSqlQuery &query) {
    QList<Projet> projets;

    if (query.exec("SELECT * FROM PROJET")) {
        while (query.next()) {
            Projet projet;
            projet.setIdPr(query.value("ID_PR").toInt());
            projet.setNomPr(query.value("NOM_PR").toString());
            projet.setStatutPr(query.value("STATUT_PR").toString());
            projet.setComplexite(query.value("COMPLEXITE").toString());
            projet.setIdClient(query.value("ID_CLIENT").toInt());
            projet.setBudget(query.value("BUDGET").toFloat());
            projet.setIdEmployee(query.value("ID_EMPLOYEE").toInt());
            projet.setDateDebut(query.value("DATE_DEBUT").toDate());
            projet.setDateFin(query.value("DATE_FIN").toDate());

            projets.append(projet);
        }
    } else {
        qDebug() << "Erreur lors de la récupération des projets:" << query.lastError().text();
    }

    return projets;
}

// Récupérer un projet par ID
Projet Projet::fetchProjetById(int id, QSqlQuery &query) {
    Projet projet;

    query.prepare("SELECT * FROM PROJET WHERE ID_PR = :id");
    query.bindValue(":id", id);

    if (query.exec() && query.next()) {
        projet.setIdPr(query.value("ID_PR").toInt());
        projet.setNomPr(query.value("NOM_PR").toString());
        projet.setStatutPr(query.value("STATUT_PR").toString());
        projet.setComplexite(query.value("COMPLEXITE").toString());
        projet.setIdClient(query.value("ID_CLIENT").toInt());
        projet.setBudget(query.value("BUDGET").toFloat());
        projet.setIdEmployee(query.value("ID_EMPLOYEE").toInt());
        projet.setDateDebut(query.value("DATE_DEBUT").toDate());
        projet.setDateFin(query.value("DATE_FIN").toDate());
    } else {
        qDebug() << "Erreur lors de la récupération du projet par ID:" << query.lastError().text();
    }

    return projet;
}

// Obtenir le nom du client par ID
QString Projet::getClientNameById(int id, QSqlQuery &query) {
    query.prepare("SELECT  NOM_CLI, PRENOM_CLI FROM CLIENT WHERE ID_CLI = :id");
    query.bindValue(":id", id);

    if (query.exec() && query.next()) {
        return query.value("NOM_CLI").toString() + " " +
               query.value("PRENOM_CLI").toString();
    } else {
        qDebug() << "Erreur lors de la récupération du nom du client:" << query.lastError().text();
        return "Client inconnu";
    }
}

// Obtenir le nom de l'employé par ID
QString Projet::getEmployeeNameById(int id, QSqlQuery &query) {
    query.prepare("SELECT  NOM_EMP, PRENOM_EMP FROM EMPLOYEE WHERE ID_EMP = :id");
    query.bindValue(":id", id);

    if (query.exec() && query.next()) {
        return query.value("NOM_EMP").toString() + " " +
               query.value("PRENOM_EMP").toString();
    } else {
        qDebug() << "Erreur lors de la récupération du nom de l'employé:" << query.lastError().text();
        return "Employé inconnu";
    }
}

// Trier les projets par nom
QList<Projet> Projet::getProjetsSortedByName(bool ascending, QSqlQuery &query) {
    QList<Projet> projets;
    QString order = ascending ? "ASC" : "DESC";

    if (query.exec(QString("SELECT * FROM PROJET ORDER BY NOM_PR %1").arg(order))) {
        while (query.next()) {
            Projet projet;
            projet.setIdPr(query.value("ID_PR").toInt());
            projet.setNomPr(query.value("NOM_PR").toString());
            projet.setStatutPr(query.value("STATUT_PR").toString());
            projet.setComplexite(query.value("COMPLEXITE").toString());
            projet.setIdClient(query.value("ID_CLIENT").toInt());
            projet.setBudget(query.value("BUDGET").toFloat());
            projet.setIdEmployee(query.value("ID_EMPLOYEE").toInt());
            projet.setDateDebut(query.value("DATE_DEBUT").toDate());
            projet.setDateFin(query.value("DATE_FIN").toDate());

            projets.append(projet);
        }
    } else {
        qDebug() << "Erreur lors du tri des projets par nom:" << query.lastError().text();
    }

    return projets;
}

// Trier les projets par budget
QList<Projet> Projet::getProjetsSortedByBudget(bool ascending, QSqlQuery &query) {
    QList<Projet> projets;
    QString order = ascending ? "ASC" : "DESC";

    if (query.exec(QString("SELECT * FROM PROJET ORDER BY BUDGET %1").arg(order))) {
        while (query.next()) {
            Projet projet;
            projet.setIdPr(query.value("ID_PR").toInt());
            projet.setNomPr(query.value("NOM_PR").toString());
            projet.setStatutPr(query.value("STATUT_PR").toString());
            projet.setComplexite(query.value("COMPLEXITE").toString());
            projet.setIdClient(query.value("ID_CLIENT").toInt());
            projet.setBudget(query.value("BUDGET").toFloat());
            projet.setIdEmployee(query.value("ID_EMPLOYEE").toInt());
            projet.setDateDebut(query.value("DATE_DEBUT").toDate());
            projet.setDateFin(query.value("DATE_FIN").toDate());

            projets.append(projet);
        }
    } else {
        qDebug() << "Erreur lors du tri des projets par budget:" << query.lastError().text();
    }

    return projets;
}

// Rechercher des projets par nom
QList<Projet> Projet::getProjetsByName(const QString &name, QSqlQuery &query) {
    QList<Projet> projets;

    query.prepare("SELECT * FROM PROJET WHERE UPPER(NOM_PR) LIKE UPPER(:name)");
    query.bindValue(":name", "%" + name + "%");

    if (query.exec()) {
        while (query.next()) {
            Projet projet;
            projet.setIdPr(query.value("ID_PR").toInt());
            projet.setNomPr(query.value("NOM_PR").toString());
            projet.setStatutPr(query.value("STATUT_PR").toString());
            projet.setComplexite(query.value("COMPLEXITE").toString());
            projet.setIdClient(query.value("ID_CLIENT").toInt());
            projet.setBudget(query.value("BUDGET").toFloat());
            projet.setIdEmployee(query.value("ID_EMPLOYEE").toInt());
            projet.setDateDebut(query.value("DATE_DEBUT").toDate());
            projet.setDateFin(query.value("DATE_FIN").toDate());

            projets.append(projet);
        }
    } else {
        qDebug() << "Erreur lors de la recherche de projets par nom:" << query.lastError().text();
    }

    return projets;
}

// Obtenir l'ID maximum des projets
int Projet::getMaxIdPr(QSqlQuery &query) {
    int maxId = 0;
    if (query.exec("SELECT MAX(ID_PR) FROM PROJET")) {
        if (query.next()) {
            maxId = query.value(0).toInt();
        }
    } else {
        qDebug() << "Erreur lors de la récupération de l'ID maximum:" << query.lastError().text();
    }
    return maxId;
}

// Récupérer tous les clients
QMap<int, QString> Projet::getAllClients(QSqlQuery &query) {
    QMap<int, QString> clients;

    if (query.exec("SELECT ID_CLI, NOM_CLI, PRENOM_CLI FROM CLIENT ORDER BY NOM_CLI")) {
        while (query.next()) {
            int id = query.value("ID_CLI").toInt();
            QString nom = query.value("NOM_CLI").toString();
            QString prenom = query.value("PRENOM_CLI").toString();
            clients[id] = nom + " " + prenom;
        }
    } else {
        qDebug() << "Erreur lors de la récupération des clients:" << query.lastError().text();
    }

    return clients;
}
// Récupérer tous les employés
QMap<int, QString> Projet::getAllEmployees(QSqlQuery &query) {
    QMap<int, QString> employees;

    if (query.exec("SELECT ID_EMP, NOM_EMP, PRENOM_EMP FROM EMPLOYEE ORDER BY NOM_EMP")) {
        while (query.next()) {
            int id = query.value("ID_EMP").toInt();
            QString nom = query.value("NOM_EMP").toString();
            QString prenom = query.value("PRENOM_EMP").toString();
            employees[id] = nom + " " + prenom;
        }
    } else {
        qDebug() << "Erreur lors de la récupération des employés:" << query.lastError().text();
    }

    return employees;
}
