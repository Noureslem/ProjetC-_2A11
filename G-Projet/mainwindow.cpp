#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "projet.h"
#include "connection.h"
#include <QDebug>
#include <QDate>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MW_projet)
{
    ui->setupUi(this);

    // Initialiser l'interface utilisateur
    setupTable();
    setupConnections();

    // Charger les données
    chargerProjets();
    chargerComboBoxClients();
    chargerComboBoxEmployees();
    chargerComboBoxProjets();

    // Configurer le combobox de tri
    ui->comboBox->clear();
    ui->comboBox->addItem("Trier Par");
    ui->comboBox->addItem("Nom (A-Z)");
    ui->comboBox->addItem("Nom (Z-A)");
    ui->comboBox->addItem("Budget (croissant)");
    ui->comboBox->addItem("Budget (décroissant)");

    // Définir les dates actuelles pour les sélecteurs de date
    ui->dateEdit_debut->setDate(QDate::currentDate());
    ui->dateEdit_fin->setDate(QDate::currentDate().addMonths(3));
    ui->dateEdit_debut_2->setDate(QDate::currentDate());
    ui->dateEdit_fin_2->setDate(QDate::currentDate().addMonths(3));

    // Configurer les déclencheurs d'édition pour le tableau
    ui->tableWidget->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupConnections()
{
    // Connecter les signaux aux slots
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);

    connect(ui->tableWidget, &QTableWidget::cellDoubleClicked,
            this, &MainWindow::on_tableWidget_cellDoubleClicked);

    connect(ui->comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::on_comboBox_currentIndexChanged);
    connect(ui->pushButton_ajouter, &QPushButton::clicked,
            this, &MainWindow::on_pushButton_ajouter_clicked);

    connect(ui->pushButton_annuler, &QPushButton::clicked,
            this, &MainWindow::on_pushButton_annuler_clicked);

    connect(ui->comboBox_projet, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::on_comboBox_projet_currentIndexChanged);

    connect(ui->pushButton_charger, &QPushButton::clicked,
            this, &MainWindow::on_pushButton_charger_clicked);

    connect(ui->pushButton_modifier, &QPushButton::clicked,
            this, &MainWindow::on_pushButton_modifier_clicked);
    connect(ui->pushButton_supprimer, &QPushButton::clicked,
            this, &MainWindow::on_pushButton_supprimer_clicked);
}

void MainWindow::onTabChanged(int index) {
    if (index == 0) { // Onglet Liste des projets
        chargerProjets();
    }
}

void MainWindow::setupTable() {
    ui->tableWidget->setColumnCount(9);

    QStringList headers = {"ID", "Nom", "Complexité", "Statut", "Budget",
                          "Client", "Employé", "Date Début", "Date Fin"};
    ui->tableWidget->setHorizontalHeaderLabels(headers);

    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void MainWindow::chargerProjets() {
    ui->tableWidget->setRowCount(0);
    originalValues.clear();

    QList<Projet> projets = Projet::fetchAllProjets(query);

    for (const Projet &projet : projets) {
        int row = ui->tableWidget->rowCount();
        ui->tableWidget->insertRow(row);

        // Obtenir les noms du client et de l'employé
        QString clientName = Projet::getClientNameById(projet.getIdClient(), query);
        QString employeeName = Projet::getEmployeeNameById(projet.getIdEmployee(), query);

        // Définir les éléments du tableau
        ui->tableWidget->setItem(row, 0, new QTableWidgetItem(QString::number(projet.getIdPr())));
        ui->tableWidget->setItem(row, 1, new QTableWidgetItem(projet.getNomPr()));
        ui->tableWidget->setItem(row, 2, new QTableWidgetItem(projet.getComplexite()));
        ui->tableWidget->setItem(row, 3, new QTableWidgetItem(projet.getStatutPr()));
        ui->tableWidget->setItem(row, 4, new QTableWidgetItem(QString::number(projet.getBudget()) + " DT"));
        ui->tableWidget->setItem(row, 5, new QTableWidgetItem(clientName));
        ui->tableWidget->setItem(row, 6, new QTableWidgetItem(employeeName));
        ui->tableWidget->setItem(row, 7, new QTableWidgetItem(projet.getDateDebut().toString("dd/MM/yyyy")));
        ui->tableWidget->setItem(row, 8, new QTableWidgetItem(projet.getDateFin().toString("dd/MM/yyyy")));

        // Stocker les valeurs originales pour suivre les modifications
        for (int col = 0; col < ui->tableWidget->columnCount(); ++col) {
            originalValues.insert({row, col}, ui->tableWidget->item(row, col)->text());
        }
    }
}

void MainWindow::chargerComboBoxClients() {
    QMap<int, QString> clients = Projet::getAllClients(query);

    ui->comboBox_client->clear();
    ui->comboBox_client_2->clear();

    for (auto it = clients.begin(); it != clients.end(); ++it) {
        ui->comboBox_client->addItem(it.value(), it.key());
        ui->comboBox_client_2->addItem(it.value(), it.key());
    }
}

void MainWindow::chargerComboBoxEmployees() {
    QMap<int, QString> employees = Projet::getAllEmployees(query);

    ui->comboBox_client_4->clear(); // Employé dans l'onglet d'ajout
    ui->comboBox_client_3->clear(); // Employé dans l'onglet de modification

    for (auto it = employees.begin(); it != employees.end(); ++it) {
        ui->comboBox_client_4->addItem(it.value(), it.key());
        ui->comboBox_client_3->addItem(it.value(), it.key());
    }
}

void MainWindow::chargerComboBoxProjets() {
    QList<Projet> projets = Projet::fetchAllProjets(query);

    ui->comboBox_projet->clear();
    ui->comboBox_2->clear(); // Pour la suppression dans l'onglet 1

    for (const Projet &projet : projets) {
        ui->comboBox_projet->addItem(projet.getNomPr(), projet.getIdPr());
        ui->comboBox_2->addItem(QString::number(projet.getIdPr()) + " - " + projet.getNomPr(), projet.getIdPr());
    }
}

void MainWindow::on_tableWidget_cellDoubleClicked(int row, int column) {
    // Obtenir l'ID du projet à partir de la première colonne
    int projectId = ui->tableWidget->item(row, 0)->text().toInt();

    // Passer à l'onglet de modification
    ui->tabWidget->setCurrentIndex(2);

    // Sélectionner le projet dans la liste déroulante
    int index = ui->comboBox_projet->findData(projectId);
    if (index != -1) {
        ui->comboBox_projet->setCurrentIndex(index);
    }

    // Remplir le formulaire
    remplirFormulaireModification(projectId);
}

void MainWindow::on_comboBox_currentIndexChanged(int index) {
    QList<Projet> projets;

    switch (index) {
        case 1: // Nom (A-Z)
            projets = Projet::getProjetsSortedByName(true, query);
            break;
        case 2: // Nom (Z-A)
            projets = Projet::getProjetsSortedByName(false, query);
            break;
        case 3: // Budget (croissant)
            projets = Projet::getProjetsSortedByBudget(true, query);
            break;
        case 4: // Budget (décroissant)
            projets = Projet::getProjetsSortedByBudget(false, query);
            break;
        default:
            projets = Projet::fetchAllProjets(query);
            break;
    }

    // Effacer le tableau
    ui->tableWidget->setRowCount(0);
    originalValues.clear();

    // Remplir le tableau avec les projets triés
    for (const Projet &projet : projets) {
        int row = ui->tableWidget->rowCount();
        ui->tableWidget->insertRow(row);

        // Obtenir les noms du client et de l'employé
        QString clientName = Projet::getClientNameById(projet.getIdClient(), query);
        QString employeeName = Projet::getEmployeeNameById(projet.getIdEmployee(), query);

        // Définir les éléments du tableau
        ui->tableWidget->setItem(row, 0, new QTableWidgetItem(QString::number(projet.getIdPr())));
        ui->tableWidget->setItem(row, 1, new QTableWidgetItem(projet.getNomPr()));
        ui->tableWidget->setItem(row, 2, new QTableWidgetItem(projet.getComplexite()));
        ui->tableWidget->setItem(row, 3, new QTableWidgetItem(projet.getStatutPr()));
        ui->tableWidget->setItem(row, 4, new QTableWidgetItem(QString::number(projet.getBudget()) + " DT"));
        ui->tableWidget->setItem(row, 5, new QTableWidgetItem(clientName));
        ui->tableWidget->setItem(row, 6, new QTableWidgetItem(employeeName));
        ui->tableWidget->setItem(row, 7, new QTableWidgetItem(projet.getDateDebut().toString("dd/MM/yyyy")));
        ui->tableWidget->setItem(row, 8, new QTableWidgetItem(projet.getDateFin().toString("dd/MM/yyyy")));

        // Stocker les valeurs originales pour suivre les modifications
        for (int col = 0; col < ui->tableWidget->columnCount(); ++col) {
            originalValues.insert({row, col}, ui->tableWidget->item(row, col)->text());
        }
    }
}

void MainWindow::on_pushButton_supprimer_clicked() {
    // Supprimer un projet
    int projectId = ui->comboBox_2->currentData().toInt();

    if (projectId <= 0) {
        QMessageBox::warning(this, "Erreur", "Veuillez sélectionner un projet à supprimer");
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(this, "Confirmation",
                                                           "Êtes-vous sûr de vouloir supprimer ce projet ?",
                                                           QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        if (Projet::deleteById(projectId, query)) {
            QMessageBox::information(this, "Succès", "Projet supprimé avec succès");
            chargerProjets();
            chargerComboBoxProjets();
        } else {
            QMessageBox::critical(this, "Erreur", "Impossible de supprimer le projet");
        }
    }
}
void MainWindow::on_pushButton_ajouter_clicked() {
    // Récupérer les données du formulaire
    QString nom = ui->lineEdit_nom->text();
    QString complexite = ui->comboBox_complexite->currentText();
    QString statut = ui->comboBox_statut->currentText();
    float budget = ui->doubleSpinBox_budget->value();
    int idClient = ui->comboBox_client->currentData().toInt();
    int idEmployee = ui->comboBox_client_4->currentData().toInt();
    QDate dateDebut = ui->dateEdit_debut->date();
    QDate dateFin = ui->dateEdit_fin->date();

    // Valider les données
    QString error;
    if (!Projet::validateNomPr(nom, error)) {
        QMessageBox::warning(this, "Erreur", error);
        return;
    }

    if (!Projet::validateBudget(budget, error)) {
        QMessageBox::warning(this, "Erreur", error);
        return;
    }

    if (!Projet::validateDates(dateDebut, dateFin, error)) {
        QMessageBox::warning(this, "Erreur", error);
        return;
    }

    // Créer l'objet projet
    Projet projet(nom, statut, complexite, budget, idClient, idEmployee, dateDebut, dateFin, query);

    // Ajouter le projet à la base de données
    if (projet.insertIntoDatabase(query)) {
        QMessageBox::information(this, "Succès", "Projet ajouté avec succès");
        viderFormulaireAjout();
        chargerProjets();
        chargerComboBoxProjets();
    } else {
        qDebug() << "Erreur SQL:" << query.lastError().text();
        QMessageBox::critical(this, "Erreur", "Impossible d'ajouter le projet");
    }
}
void MainWindow::on_pushButton_annuler_clicked() {
    // Exporter en PDF
    QString fileName = QFileDialog::getSaveFileName(this, "Exporter en PDF",
                                                  QDir::homePath() + "/projets.pdf",
                                                  "Fichiers PDF (*.pdf)");

    if (!fileName.isEmpty()) {
        if (exportProjectsToPdf(fileName)) {
            QMessageBox::information(this, "Succès", "Projets exportés en PDF avec succès");
        } else {
            QMessageBox::critical(this, "Erreur", "Impossible d'exporter les projets en PDF");
        }
    }
}
void MainWindow::on_comboBox_projet_currentIndexChanged(int index) {
    // Ne rien faire, juste mettre à jour l'interface quand l'utilisateur sélectionne un projet
}

void MainWindow::on_pushButton_charger_clicked() {
    // Charger les détails du projet dans le formulaire de modification
    int projectId = ui->comboBox_projet->currentData().toInt();

    if (projectId <= 0) {
        QMessageBox::warning(this, "Erreur", "Veuillez sélectionner un projet");
        return;
    }

    remplirFormulaireModification(projectId);
}

void MainWindow::on_pushButton_modifier_clicked() {
    // Récupérer les données du formulaire
    int id = ui->lineEdit_id->text().toInt();
    QString nom = ui->lineEdit_nom_2->text();
    QString complexite = ui->comboBox_complexite_2->currentText();
    QString statut = ui->comboBox_statut_2->currentText();
    float budget = ui->doubleSpinBox_budget_2->value();
    int idClient = ui->comboBox_client_2->currentData().toInt();
    int idEmployee = ui->comboBox_client_3->currentData().toInt();
    QDate dateDebut = ui->dateEdit_debut_2->date();
    QDate dateFin = ui->dateEdit_fin_2->date();

    // Valider les données
    if (id <= 0) {
        QMessageBox::warning(this, "Erreur", "Veuillez d'abord charger un projet");
        return;
    }

    QString error;
    if (!Projet::validateNomPr(nom, error)) {
        QMessageBox::warning(this, "Erreur", error);
        return;
    }

    if (!Projet::validateBudget(budget, error)) {
        QMessageBox::warning(this, "Erreur", error);
        return;
    }

    if (!Projet::validateDates(dateDebut, dateFin, error)) {
        QMessageBox::warning(this, "Erreur", error);
        return;
    }

    // Mettre à jour le projet dans la base de données
    bool updateSuccess = true;

    if (!Projet::updateProjet(id, "NOM_PR", nom, query)) updateSuccess = false;
    if (!Projet::updateProjet(id, "COMPLEXITE", complexite, query)) updateSuccess = false;
    if (!Projet::updateProjet(id, "STATUT_PR", statut, query)) updateSuccess = false;
    if (!Projet::updateProjet(id, "BUDGET", QString::number(budget), query)) updateSuccess = false;
    if (!Projet::updateProjet(id, "ID_CLIENT", QString::number(idClient), query)) updateSuccess = false;
    if (!Projet::updateProjet(id, "ID_EMPLOYEE", QString::number(idEmployee), query)) updateSuccess = false;
    if (!Projet::updateProjet(id, "DATE_DEBUT", dateDebut.toString("yyyy-MM-dd"), query)) updateSuccess = false;
    if (!Projet::updateProjet(id, "DATE_FIN", dateFin.toString("yyyy-MM-dd"), query)) updateSuccess = false;
    if (updateSuccess) {
        QMessageBox::information(this, "Succès", "Projet modifié avec succès");
        viderFormulaireModification();
        chargerProjets();
        chargerComboBoxProjets();
    } else{
        QMessageBox::critical(this, "Erreur", "Impossible de modifier le projet");
    }
}

void MainWindow::viderFormulaireAjout() {
    ui->lineEdit_nom->clear();
    ui->comboBox_complexite->setCurrentIndex(0);
    ui->comboBox_statut->setCurrentIndex(0);
    ui->doubleSpinBox_budget->setValue(0);
    ui->comboBox_client->setCurrentIndex(0);

    ui->comboBox_client_4->setCurrentIndex(0);
    ui->dateEdit_debut->setDate(QDate::currentDate());
    ui->dateEdit_fin->setDate(QDate::currentDate().addMonths(3));
}

void MainWindow::viderFormulaireModification() {
    ui->lineEdit_id->clear();
    ui->lineEdit_nom_2->clear();
    ui->comboBox_complexite_2->setCurrentIndex(0);
    ui->comboBox_statut_2->setCurrentIndex(0);
    ui->doubleSpinBox_budget_2->setValue(0);
    ui->comboBox_client_2->setCurrentIndex(0);
    ui->comboBox_client_2->setCurrentIndex(0);
    ui->comboBox_client_3->setCurrentIndex(0);
    ui->dateEdit_debut_2->setDate(QDate::currentDate());
    ui->dateEdit_fin_2->setDate(QDate::currentDate().addMonths(3));
}

void MainWindow::remplirFormulaireModification(int projetId) {
    Projet projet = Projet::fetchProjetById(projetId, query);

    if (projet.getIdPr() <= 0) {
        QMessageBox::warning(this, "Erreur", "Projet non trouvé");
        return;
    }

    ui->lineEdit_id->setText(QString::number(projet.getIdPr()));
    ui->lineEdit_nom_2->setText(projet.getNomPr());

    // Définir la complexité
    int complexiteIndex = ui->comboBox_complexite_2->findText(projet.getComplexite());
    if (complexiteIndex != -1) {
        ui->comboBox_complexite_2->setCurrentIndex(complexiteIndex);
    }

    // Définir le statut
    int statutIndex = ui->comboBox_statut_2->findText(projet.getStatutPr());
    if (statutIndex != -1) {
        ui->comboBox_statut_2->setCurrentIndex(statutIndex);
    }

    ui->doubleSpinBox_budget_2->setValue(projet.getBudget());

    // Définir le client
    int clientIndex = ui->comboBox_client_2->findData(projet.getIdClient());
    if (clientIndex != -1) {
        ui->comboBox_client_2->setCurrentIndex(clientIndex);
    }

    // Définir l'employé
    int employeeIndex = ui->comboBox_client_3->findData(projet.getIdEmployee());
    if (employeeIndex != -1) {
        ui->comboBox_client_3->setCurrentIndex(employeeIndex);
    }

    ui->dateEdit_debut_2->setDate(projet.getDateDebut());
    ui->dateEdit_fin_2->setDate(projet.getDateFin());
}

bool MainWindow::exportProjectsToPdf(const QString &fileName) {
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);

    QTextDocument document;
    QString html = "<html><body>";
    html += "<h1 align='center'>Liste des Projets</h1>";
    html += "<table width='100%' border='1' cellspacing='0' cellpadding='3'>";
    html += "<tr bgcolor='#4D6F50' style='color:white;'>";
    html += "<th>ID</th><th>Nom</th><th>Complexité</th><th>Statut</th>";
    html += "<th>Budget</th><th>Client</th><th>Employé</th>";
    html += "<th>Date Début</th><th>Date Fin</th>";
    html += "</tr>";

    QList<Projet> projets = Projet::fetchAllProjets(query);

    for (const Projet &projet : projets) {
        html += "<tr>";
        html += "<td>" + QString::number(projet.getIdPr()) + "</td>";
        html += "<td>" + projet.getNomPr() + "</td>";
        html += "<td>" + projet.getComplexite() + "</td>";
        html += "<td>" + projet.getStatutPr() + "</td>";
        html += "<td>" + QString::number(projet.getBudget()) + " DT</td>";
        html += "<td>" + Projet::getClientNameById(projet.getIdClient(), query) + "</td>";
        html += "<td>" + Projet::getEmployeeNameById(projet.getIdEmployee(), query) + "</td>";
        html += "<td>" + projet.getDateDebut().toString("dd/MM/yyyy") + "</td>";
        html += "<td>" + projet.getDateFin().toString("dd/MM/yyyy") + "</td>";
        html += "</tr>";
    }

    html += "</table>";
    html += "</body></html>";

    document.setHtml(html);
    document.print(&printer);

    return true;
}

bool MainWindow::exportProjectToPdf(int projetId, const QString &fileName) {
    Projet projet = Projet::fetchProjetById(projetId, query);

    if (projet.getIdPr() <= 0) {
        return false;
    }

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);

    QTextDocument document;
    QString html = "<html><body>";
    html += "<h1 align='center'>Détails du Projet</h1>";

    html += "<table width='100%' border='0' cellspacing='0' cellpadding='5'>";
    html += "<tr><td width='30%'><b>ID:</b></td><td>" + QString::number(projet.getIdPr()) + "</td></tr>";
    html += "<tr><td><b>Nom:</b></td><td>" + projet.getNomPr() + "</td></tr>";
    html += "<tr><td><b>Complexité:</b></td><td>" + projet.getComplexite() + "</td></tr>";
    html += "<tr><td><b>Statut:</b></td><td>" + projet.getStatutPr() + "</td></tr>";
    html += "<tr><td><b>Budget:</b></td><td>" + QString::number(projet.getBudget()) + " DT</td></tr>";
    html += "<tr><td><b>Client:</b></td><td>" + Projet::getClientNameById(projet.getIdClient(), query) + "</td></tr>";
    html += "<tr><td><b>Employé:</b></td><td>" + Projet::getEmployeeNameById(projet.getIdEmployee(), query) + "</td></tr>";
    html += "<tr><td><b>Date de début:</b></td><td>" + projet.getDateDebut().toString("dd/MM/yyyy") + "</td></tr>";
    html += "<tr><td><b>Date de fin:</b></td><td>" + projet.getDateFin().toString("dd/MM/yyyy") + "</td></tr>";
    html += "</table>";

    html += "</body></html>";

    document.setHtml(html);
    document.print(&printer);

    return true;
}
