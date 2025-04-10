#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "projet.h"
#include "connection.h"
#include <QDebug>
#include <QDate>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QtCharts>

QT_USE_NAMESPACE

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MW_projet)
{
    ui->setupUi(this);

    // Setup notification system first
    setupNotificationSystem();

    // Create a professional search bar
    QLineEdit *searchBar = new QLineEdit(this);
    searchBar->setObjectName("lineEdit_search");
    searchBar->setPlaceholderText("Rechercher par nom de projet...");
    searchBar->setFixedWidth(250);
    searchBar->setFixedHeight(30);

    // Add search icon to the search bar
    QAction *searchAction = new QAction(searchBar);
    searchAction->setIcon(QIcon(":/icons/search.png")); // Make sure you have this icon in your resources
    searchBar->addAction(searchAction, QLineEdit::LeadingPosition);

    // Style the search bar to look more professional
    searchBar->setStyleSheet(
        "QLineEdit {"
        "   border: 1px solid #c0c0c0;"
        "   border-radius: 15px;"
        "   padding-left: 30px;"
        "   padding-right: 10px;"
        "   background-color: white;"
        "   selection-background-color: #4D6F50;"
        "   font-size: 12px;"
        "}"
        "QLineEdit:focus {"
        "   border: 1px solid #4D6F50;"
        "}"
        );

    // Position the search bar at the top right, next to notification button
    searchBar->move(this->width() - 320, 10);

    // Connect the search bar signal
    connect(searchBar, &QLineEdit::textChanged, this, &MainWindow::on_lineEdit_search_textChanged);

    // Add clear button to search bar
    searchBar->setClearButtonEnabled(true);

    // Ensure the scroll area is set up properly
    ui->scrollArea->setWidgetResizable(true); // Make the scroll area resize its inner widget

    // Set the scroll area's widget (if not already set in Qt Designer)
    if (!ui->scrollArea->widget()) {
        QWidget *scrollWidget = new QWidget(ui->scrollArea);
        ui->scrollArea->setWidget(scrollWidget);
    }

    // Initialiser l'interface utilisateur
    setupTable();
    setupConnections();
    setupStatisticsTab();

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

    // Create initial charts
    createStatutChart();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupConnections()
{
    static bool isConnected = false; // Indicateur pour vérifier si les connexions ont déjà été établies
    if (isConnected) return; // Si déjà connecté, ne rien faire

    // Établir les connexions
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);
    connect(ui->tableWidget, &QTableWidget::cellDoubleClicked, this, &MainWindow::on_tableWidget_cellDoubleClicked);
    connect(ui->comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::on_comboBox_currentIndexChanged);
    connect(ui->comboBox_projet, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::on_comboBox_projet_currentIndexChanged);
    connect(ui->pushButton_ajouter, &QPushButton::clicked, this, &MainWindow::on_pushButton_ajouter_clicked);
    connect(ui->pushButton_annuler, &QPushButton::clicked, this, &MainWindow::on_pushButton_annuler_clicked);
    connect(ui->pushButton_charger, &QPushButton::clicked, this, &MainWindow::on_pushButton_charger_clicked);
    connect(ui->pushButton_modifier, &QPushButton::clicked, this, &MainWindow::on_pushButton_modifier_clicked);
    connect(ui->pushButton_supprimer, &QPushButton::clicked, this, &MainWindow::on_pushButton_supprimer_clicked);
    // Remove this line:
    // connect(ui->lineEdit_search, &QLineEdit::textChanged, this, &MainWindow::on_lineEdit_search_textChanged);

    isConnected = true; // Marquer comme connecté
}

void MainWindow::setupNotificationSystem()
{
    // Create notification button with bell icon
    notificationButton = new QPushButton(this);
    notificationButton->setIcon(QIcon(":/icons/bell.png"));
    notificationButton->setIconSize(QSize(24, 24));
    notificationButton->setFixedSize(40, 40);
    notificationButton->setStyleSheet("QPushButton { border-radius: 20px; background-color: transparent; } "
                                      "QPushButton:hover { background-color: #e0e0e0; }");

    // Position the button in the top right corner
    notificationButton->move(this->width() - 60, 10);

    // Create notification menu
    notificationMenu = new QMenu(this);
    notificationMenu->setStyleSheet("QMenu { width: 300px; }");

    // Initialize notification data
    unreadCount = 0;

    // Connect button to show menu
    connect(notificationButton, &QPushButton::clicked, [this]() {
        // Update notification widget before showing
        notificationMenu->clear();
        QWidgetAction* widgetAction = new QWidgetAction(notificationMenu);
        widgetAction->setDefaultWidget(createNotificationWidget());
        notificationMenu->addAction(widgetAction);

        // Show menu below button
        notificationMenu->popup(notificationButton->mapToGlobal(QPoint(0, notificationButton->height())));

        // Reset unread count when opened
        unreadCount = 0;
        updateNotificationButton();
    });

    // Remove the connect for resized signal

    // Add some sample notifications
    addNotification("Bienvenue dans le système de gestion de projets!");
    addNotification("Vous avez 3 projets en cours.");
    addNotification("Un nouveau client a été ajouté.");

    // Setup timer to add random notifications (for demo purposes)
    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, [this]() {
        QStringList messages = {
            "Un projet a été mis à jour.",
            "Nouvelle tâche assignée.",
            "Réunion planifiée pour demain.",
            "Date limite approchante pour le projet XYZ.",
            "Budget mis à jour pour le projet ABC."
        };
        int index = QRandomGenerator::global()->bounded(messages.size());
        addNotification(messages[index]);
    });
    timer->start(60000);  // Add a notification every minute
}

void MainWindow::addNotification(const QString& message)
{
    // Add timestamp to notification
    QString timestamp = QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm");
    QString fullMessage = message + " (" + timestamp + ")";

    // Add to beginning of list (newest first)
    notifications.prepend(fullMessage);

    // Limit number of notifications
    if (notifications.size() > 20) {
        notifications.removeLast();
    }

    // Increment unread count
    unreadCount++;

    // Update button appearance
    updateNotificationButton();
}

QWidget* MainWindow::createNotificationWidget()
{
    // Create widget to hold notifications
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);

    // Add header
    QLabel* headerLabel = new QLabel("Notifications");
    headerLabel->setStyleSheet("font-weight: bold; font-size: 16px; padding: 8px;");
    layout->addWidget(headerLabel);

    // Add separator
    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    layout->addWidget(line);

    // Create scroll area for notifications
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // Create widget to hold notification items
    QWidget* scrollWidget = new QWidget();
    QVBoxLayout* scrollLayout = new QVBoxLayout(scrollWidget);
    scrollLayout->setSpacing(0);
    scrollLayout->setContentsMargins(0, 0, 0, 0);

    if (notifications.isEmpty()) {
        // Show message if no notifications
        QLabel* emptyLabel = new QLabel("Aucune notification");
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet("color: gray; padding: 20px;");
        scrollLayout->addWidget(emptyLabel);
    } else {
        // Add each notification
        for (const QString& notification : notifications) {
            QWidget* itemWidget = new QWidget();
            QHBoxLayout* itemLayout = new QHBoxLayout(itemWidget);

            // Add blue dot for new notifications
            if (notifications.indexOf(notification) < unreadCount) {
                QLabel* dotLabel = new QLabel();
                dotLabel->setFixedSize(8, 8);
                dotLabel->setStyleSheet("background-color: #1e88e5; border-radius: 4px;");
                itemLayout->addWidget(dotLabel);
            } else {
                // Add spacing for alignment
                itemLayout->addSpacing(8);
            }

            // Add notification text
            QLabel* textLabel = new QLabel(notification);
            textLabel->setWordWrap(true);
            textLabel->setStyleSheet("padding: 8px 4px;");
            itemLayout->addWidget(textLabel, 1);

            // Add item to scroll area
            scrollLayout->addWidget(itemWidget);

            // Add separator except for last item
            if (notifications.indexOf(notification) < notifications.size() - 1) {
                QFrame* itemLine = new QFrame();
                itemLine->setFrameShape(QFrame::HLine);
                itemLine->setFrameShadow(QFrame::Sunken);
                itemLine->setStyleSheet("color: #e0e0e0;");
                scrollLayout->addWidget(itemLine);
            }
        }
    }

    // Add "Mark all as read" button
    QPushButton* markReadButton = new QPushButton("Marquer tout comme lu");
    markReadButton->setStyleSheet("text-align: center; padding: 8px; color: #1e88e5;");
    connect(markReadButton, &QPushButton::clicked, [this]() {
        unreadCount = 0;
        updateNotificationButton();
        notificationMenu->close();
    });

    // Set up scroll area
    scrollArea->setWidget(scrollWidget);
    scrollArea->setFixedHeight(300);  // Set maximum height

    // Add components to main layout
    layout->addWidget(scrollArea);
    layout->addWidget(markReadButton);

    return widget;
}

void MainWindow::updateNotificationButton()
{
    if (unreadCount > 0) {
        // Show unread count badge
        notificationButton->setStyleSheet("QPushButton { border-radius: 20px; background-color: transparent; } "
                                          "QPushButton:hover { background-color: #e0e0e0; } "
                                          "QPushButton::after { content: '" + QString::number(unreadCount) + "'; "
                                                                           "position: absolute; top: 0; right: 0; "
                                                                           "background-color: #ff4081; color: white; "
                                                                           "border-radius: 10px; min-width: 20px; min-height: 20px; "
                                                                           "font-size: 12px; font-weight: bold; }");
    } else {
        // No badge
        notificationButton->setStyleSheet("QPushButton { border-radius: 20px; background-color: transparent; } "
                                          "QPushButton:hover { background-color: #e0e0e0; }");
    }
}
void MainWindow::setupStatisticsTab()
{
    // Create a new tab for statistics
    QWidget *statisticsTab = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(statisticsTab);

    // Add a title
    QLabel *titleLabel = new QLabel("Statistiques des Projets");
    titleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);

    // Create a horizontal layout for the charts
    QHBoxLayout *chartsLayout = new QHBoxLayout();

    // Create containers for the charts
    QWidget *barChartContainer = new QWidget();
    QVBoxLayout *barChartLayout = new QVBoxLayout(barChartContainer);
    QLabel *barChartLabel = new QLabel("Nombre de Projets par Statut");
    barChartLabel->setAlignment(Qt::AlignCenter);
    QFont chartFont = barChartLabel->font();
    chartFont.setBold(true);
    barChartLabel->setFont(chartFont);
    barChartLayout->addWidget(barChartLabel);

    // Create the chart view
    chartView = new QChartView();
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setMinimumHeight(300);
    barChartLayout->addWidget(chartView);

    // Add the chart containers to the charts layout
    chartsLayout->addWidget(barChartContainer);

    // Add the charts layout to the main layout
    mainLayout->addLayout(chartsLayout);

    // Add the statistics tab to the tab widget
    ui->tabWidget->addTab(statisticsTab, "Statistiques");
}

void MainWindow::onTabChanged(int index) {
    if (index == 0) { // Onglet Liste des projets
        chargerProjets();
    } else if (index == 3) { // Onglet Statistiques (assuming it's the 4th tab)
        createStatutChart();
    }
}
void MainWindow::setupTable() {
    ui->tableWidget->setColumnCount(9);

    QStringList headers = {"ID", "Nom", "Complexité", "Statut", "Budget",
                           "Client", "Employé", "Date Début", "Date Fin"};
    ui->tableWidget->setHorizontalHeaderLabels(headers);

    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void MainWindow::searchProjects(const QString &searchText) {
    // If search text is empty, load all projects
    if (searchText.isEmpty()) {
        chargerProjets();
        return;
    }

    // Clear the table
    ui->tableWidget->setRowCount(0);
    originalValues.clear();

    // Prepare the query to search by project name
    QSqlQuery searchQuery;
    searchQuery.prepare("SELECT * FROM PROJET WHERE UPPER (NOM_PR) LIKE UPPER (:search)");
    searchQuery.bindValue(":search", "%" + searchText + "%");

    if (searchQuery.exec()) {
        while (searchQuery.next()) {
            int row = ui->tableWidget->rowCount();
            ui->tableWidget->insertRow(row);

            int idPr = searchQuery.value("ID_PR").toInt();
            QString nomPr = searchQuery.value("NOM_PR").toString();
            QString complexite = searchQuery.value("COMPLEXITE").toString();
            QString statutPr = searchQuery.value("STATUT_PR").toString();
            float budget = searchQuery.value("BUDGET").toFloat();
            int idClient = searchQuery.value("ID_CLIENT").toInt();
            int idEmployee = searchQuery.value("ID_EMPLOYEE").toInt();
            QDate dateDebut = searchQuery.value("DATE_DEBUT").toDate();
            QDate dateFin = searchQuery.value("DATE_FIN").toDate();

            // Get client and employee names
            QString clientName = Projet::getClientNameById(idClient, query);
            QString employeeName = Projet::getEmployeeNameById(idEmployee, query);

            // Set table items
            ui->tableWidget->setItem(row, 0, new QTableWidgetItem(QString::number(idPr)));
            ui->tableWidget->setItem(row, 1, new QTableWidgetItem(nomPr));
            ui->tableWidget->setItem(row, 2, new QTableWidgetItem(complexite));
            ui->tableWidget->setItem(row, 3, new QTableWidgetItem(statutPr));
            ui->tableWidget->setItem(row, 4, new QTableWidgetItem(QString::number(budget) + " DT"));
            ui->tableWidget->setItem(row, 5, new QTableWidgetItem(clientName));
            ui->tableWidget->setItem(row, 6, new QTableWidgetItem(employeeName));
            ui->tableWidget->setItem(row, 7, new QTableWidgetItem(dateDebut.toString("dd/MM/yyyy")));
            ui->tableWidget->setItem(row, 8, new QTableWidgetItem(dateFin.toString("dd/MM/yyyy")));

            // Store original values
            for (int col = 0; col < ui->tableWidget->columnCount(); ++col) {
                originalValues.insert({row, col}, ui->tableWidget->item(row, col)->text());
            }
        }
    } else {
        qDebug() << "Search query failed:" << searchQuery.lastError().text();
    }
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

    // Update charts if we're on the statistics tab
    if (ui->tabWidget->currentIndex() == 3) {
        createStatutChart();
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

            // Update charts
            if (ui->tabWidget->currentIndex() == 3) {
                createStatutChart();
            }
        } else {
            QMessageBox::critical(this, "Erreur", "Impossible de supprimer le projet");
        }
    }
}

void MainWindow::on_pushButton_ajouter_clicked()
{
    // Désactiver le bouton pour éviter les clics multiples
    ui->pushButton_ajouter->setEnabled(false);

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
        ui->pushButton_ajouter->setEnabled(true); // Réactiver le bouton en cas d'erreur
        return;
    }

    if (!Projet::validateBudget(budget, error)) {
        QMessageBox::warning(this, "Erreur", error);
        ui->pushButton_ajouter->setEnabled(true); // Réactiver le bouton en cas d'erreur
        return;
    }

    if (!Projet::validateDates(dateDebut, dateFin, error)) {
        QMessageBox::warning(this, "Erreur", error);
        ui->pushButton_ajouter->setEnabled(true); // Réactiver le bouton en cas d'erreur
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

        // Update charts
        if (ui->tabWidget->currentIndex() == 3) {
            createStatutChart();
        }
    } else {
        qDebug() << "Erreur SQL:" << query.lastError().text();
        QMessageBox::critical(this, "Erreur", "Impossible d'ajouter le projet");
    }

    // Réactiver le bouton après l'exécution
    ui->pushButton_ajouter->setEnabled(true);
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

        // Update charts
        if (ui->tabWidget->currentIndex() == 3) {
            createStatutChart();
        }
    } else {
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

QMap<QString, int> MainWindow::getProjectCountsByStatus() {
    QMap<QString, int> statusCounts;

    // Query to count projects by status
    query.prepare("SELECT STATUT_PR, COUNT(*) as count FROM PROJET GROUP BY STATUT_PR");

    if (query.exec()) {
        while (query.next()) {
            QString status = query.value(0).toString();
            int count = query.value(1).toInt();
            statusCounts[status] = count;
        }
    } else {
        qDebug() << "Query failed:" << query.lastError().text();
    }

    return statusCounts;
}

void MainWindow::createStatutChart() {
    // Get project counts by status
    QMap<QString, int> statusCounts = getProjectCountsByStatus();

    // Create chart
    QChart *chart = new QChart();
    chart->setTitle("Nombre de Projets par Statut");
    chart->setAnimationOptions(QChart::SeriesAnimations);

    // Create series
    QBarSeries *series = new QBarSeries();

    // Create bar set
    QBarSet *set = new QBarSet("Projets");

    // Add data to the set
    QStringList statuses;
    for (auto it = statusCounts.begin(); it != statusCounts.end(); ++it) {
        statuses << it.key();
        *set << it.value();
    }

    // Add set to series
    series->append(set);
    chart->addSeries(series);

    // Create axes
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(statuses);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("Nombre de Projets");

    // Set Y-axis range with some padding
    int maxValue = 0;
    for (auto it = statusCounts.begin(); it != statusCounts.end(); ++it) {
        if (it.value() > maxValue) maxValue = it.value();
    }
    axisY->setRange(0, maxValue > 0 ? maxValue * 1.1 : 10);

    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    // Set the chart on the chart view
    chartView->setChart(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
}

// Add this slot function to handle search text changes
void MainWindow::on_lineEdit_search_textChanged(const QString &arg1) {
    searchProjects(arg1);
}

