#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "projet.h"
#include <QDebug>
#include <QDate>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QtCharts>
#include <QAction>
#include <QLineEdit>
#include <QIcon>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSqlError>
#include <QFile>

QT_USE_NAMESPACE

    // Event filter for notification widgets
    class NotificationEventFilter : public QObject
{
public:
    NotificationEventFilter(int id, MainWindow* mainWindow)
        : QObject(mainWindow), notificationId(id), mainWindow(mainWindow) {}

    bool eventFilter(QObject* watched, QEvent* event) override {
        if (event->type() == QEvent::MouseButtonRelease) {
            mainWindow->onNotificationClicked(notificationId);
            return true;
        }
        return QObject::eventFilter(watched, event);
    }

private:
    int notificationId;
    MainWindow* mainWindow;
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MW_projet)
    , unreadCount(0)
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

    // Position the search bar at the top right
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

    // Start the notification timer to check for new notifications
    notificationTimer = new QTimer(this);
    connect(notificationTimer, &QTimer::timeout, this, &MainWindow::checkForNewNotifications);
    notificationTimer->start(30000); // Check every 30 seconds

    // Load initial notifications
    loadNotifications();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupNotificationSystem    ()
{
    // Create notification button
    notificationButton = new QPushButton(this);
    notificationButton->setFixedSize(40, 40);
    notificationButton->setIconSize(QSize(24, 24));
    notificationButton->setCursor(Qt::PointingHandCursor);
    notificationButton->setToolTip("Notifications");
    notificationButton->setStyleSheet(
        "QPushButton {"
        "   border: none;"
        "   border-radius: 20px;"
        "   background-color: transparent;"
        "}"
        "QPushButton:hover {"
        "   background-color: rgba(200, 200, 200, 0.3);"
        "}"
        "QPushButton:pressed {"
        "   background-color: rgba(150, 150, 150, 0.5);"
        "}"
        );
    notificationButton->move(this->width() - 60, 10);

    // Create notification menu
    notificationMenu = new QMenu(this);
    notificationMenu->setStyleSheet(
        "QMenu {"
        "   background-color: white;"
        "   border: 1px solid #c0c0c0;"
        "   border-radius: 5px;"
        "   padding: 5px;"
        "}"
        "QMenu::item {"
        "   padding: 5px 30px 5px 30px;"
        "   border-radius: 3px;"
        "}"
        "QMenu::item:selected {"
        "   background-color: #f0f0f0;"
        "}"
        );

    // Connect notification button to show menu
    connect(notificationButton, &QPushButton::clicked, this, &MainWindow::onNotificationButtonClicked);

    // Initialize unread count
    unreadCount = 0;
    updateNotificationButton();
}

void MainWindow::updateNotificationButton()
{
    if (unreadCount > 0) {
        // Try to use custom bell icon if available
        if (QFile::exists(":/Tools Interface/bell-active.png")) {
            notificationButton->setIcon(QIcon(":/Tools Interface/bell-active.png"));
        } else {
            // Fallback to standard icon
            notificationButton->setIcon(style()->standardIcon(QStyle::SP_MessageBoxInformation));
        }

        // Add a badge with the count
        QLabel* badge = new QLabel(notificationButton);
        badge->setText(QString::number(unreadCount));
        badge->setAlignment(Qt::AlignCenter);
        badge->setStyleSheet(
            "QLabel {"
            "   color: white;"
            "   background-color: #E74C3C;"
            "   border-radius: 10px;"
            "   font-weight: bold;"
            "   font-size: 10px;"
            "   min-width: 20px;"
            "   min-height: 20px;"
            "}"
            );
        badge->setGeometry(notificationButton->width() - 20, 0, 20, 20);
        badge->show();

        // Store the badge as a property to delete it later
        QLabel* oldBadge = notificationButton->findChild<QLabel*>();
        if (oldBadge && oldBadge != badge) {
            oldBadge->deleteLater();
        }
    } else {
        // Try to use custom bell icon if available
        if (QFile::exists(":/Tools Interface/bell-inactive.png")) {
            notificationButton->setIcon(QIcon(":/Tools Interface/bell-inactive.png"));
        } else {
            // Fallback to standard icon
            notificationButton->setIcon(style()->standardIcon(QStyle::SP_MessageBoxQuestion));
        }

        // Remove any existing badge
        QLabel* badge = notificationButton->findChild<QLabel*>();
        if (badge) {
            badge->deleteLater();
        }
    }
}

void MainWindow::onNotificationButtonClicked()
{
    // Clear the menu
    notificationMenu->clear();

    // Load notifications
    loadNotifications();

    // Show the menu below the notification button
    notificationMenu->popup(notificationButton->mapToGlobal(QPoint(0, notificationButton->height())));
}

void MainWindow::loadNotifications()
{
    // Clear the menu
    notificationMenu->clear();

    // Query to get notifications, ordered by date (newest first) and read status (unread first)
    QSqlQuery notificationQuery;
    notificationQuery.prepare(
        "SELECT NOTIFICATION_ID, NOTIFICATION_MESSAGE, NOTIFICATION_DATE, IS_READ "
        "FROM NOTIFICATIONS "
        "ORDER BY IS_READ ASC, NOTIFICATION_DATE DESC"
        );

    if (notificationQuery.exec()) {
        int count = 0;
        unreadCount = 0;

        while (notificationQuery.next() && count < 10) { // Limit to 10 notifications
            int id = notificationQuery.value("NOTIFICATION_ID").toInt();
            QString message = notificationQuery.value("NOTIFICATION_MESSAGE").toString();
            QDateTime date = notificationQuery.value("NOTIFICATION_DATE").toDateTime();
            bool isRead = notificationQuery.value("IS_READ").toInt() == 1;

            if (!isRead) {
                unreadCount++;
            }

            QWidget* notificationWidget = createNotificationWidget(id, message, date, isRead);
            QWidgetAction* widgetAction = new QWidgetAction(notificationMenu);
            widgetAction->setDefaultWidget(notificationWidget);
            notificationMenu->addAction(widgetAction);

            count++;
        }

        // Add separator
        notificationMenu->addSeparator();

        // Add "Mark All as Read" action
        QAction* markAllAsReadAction = new QAction("Marquer tout comme lu", notificationMenu);
        connect(markAllAsReadAction, &QAction::triggered, this, &MainWindow::onMarkAllAsReadClicked);
        notificationMenu->addAction(markAllAsReadAction);

        // Update notification button
        updateNotificationButton();
    } else {
        qDebug() << "Failed to load notifications:" << notificationQuery.lastError().text();
    }
}

QWidget* MainWindow::createNotificationWidget(int id, const QString& message, const QDateTime& date, bool isRead)
{
    QWidget* widget = new QWidget();
    widget->setMinimumWidth(300);
    widget->setMaximumWidth(400);

    QVBoxLayout* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(3);

    // Message label
    QLabel* messageLabel = new QLabel(message);
    messageLabel->setWordWrap(true);
    messageLabel->setStyleSheet(isRead ? "color: #666666;" : "color: #000000; font-weight: bold;");
    layout->addWidget(messageLabel);

    // Date label
    QLabel* dateLabel = new QLabel(date.toString("dd/MM/yyyy hh:mm"));
    dateLabel->setStyleSheet("color: #999999; font-size: 10px;");
    layout->addWidget(dateLabel);

    // Make the widget clickable
    widget->setProperty("notification_id", id);
    widget->setCursor(Qt::PointingHandCursor);
    widget->setStyleSheet(
        "QWidget {"
        "   background-color: " + QString(isRead ? "white" : "#F0F7FF") + ";"
                                                  "   border-radius: 3px;"
                                                  "   padding: 5px;"
                                                  "}"
                                                  "QWidget:hover {"
                                                  "   background-color: #f0f0f0;"
                                                  "}"
        );

    // Connect the widget to the notification clicked slot
    widget->installEventFilter(new NotificationEventFilter(id, this));

    return widget;
}

void MainWindow::onNotificationClicked(int notificationId)
{
    // Mark the notification as read
    markNotificationAsRead(notificationId);

    // Get notification details
    QSqlQuery query;
    query.prepare(
        "SELECT PROJECT_ID, NOTIFICATION_TYPE, RELATED_ENTITY_ID, RELATED_ENTITY_TYPE "
        "FROM NOTIFICATIONS "
        "WHERE NOTIFICATION_ID = :id"
        );
    query.bindValue(":id", notificationId);

    if (query.exec() && query.next()) {
        int projectId = query.value("PROJECT_ID").toInt();
        QString type = query.value("NOTIFICATION_TYPE").toString();

        // Navigate to the appropriate tab based on notification type
        if (projectId > 0) {
            // Navigate to the project details
            ui->tabWidget->setCurrentIndex(0); // Projects list tab

            // Find the project in the table and select it
            for (int row = 0; row < ui->tableWidget->rowCount(); ++row) {
                if (ui->tableWidget->item(row, 0)->text().toInt() == projectId) {
                    ui->tableWidget->selectRow(row);
                    break;
                }
            }
        }
    }

    // Reload notifications
    loadNotifications();
}

void MainWindow::markNotificationAsRead(int notificationId)
{
    QSqlQuery query;
    query.prepare("UPDATE NOTIFICATIONS SET IS_READ = 1 WHERE NOTIFICATION_ID = :id");
    query.bindValue(":id", notificationId);

    if (!query.exec()) {
        qDebug() << "Failed to mark notification as read:" << query.lastError().text();
    }
}

void MainWindow::onMarkAllAsReadClicked()
{
    markAllNotificationsAsRead();
    loadNotifications();
}

void MainWindow::markAllNotificationsAsRead()
{
    QSqlQuery query;
    query.prepare("UPDATE NOTIFICATIONS SET IS_READ = 1 WHERE IS_READ = 0");

    if (!query.exec()) {
        qDebug() << "Failed to mark all notifications as read:" << query.lastError().text();
    }
}

void MainWindow::checkForNewNotifications()
{
    // This method is called periodically to check for new notifications
    loadNotifications();
}

void MainWindow::addNotification(int projectId, const QString& type, const QString& message, int relatedEntityId, const QString& relatedEntityType)
{
    QSqlQuery query;
    query.prepare(
        "INSERT INTO NOTIFICATIONS "
        "(NOTIFICATION_ID, PROJECT_ID, NOTIFICATION_TYPE, NOTIFICATION_MESSAGE, RELATED_ENTITY_ID, RELATED_ENTITY_TYPE) "
        "VALUES "
        "(NOTIFICATIONS_SEQ.NEXTVAL, :projectId, :type, :message, :relatedEntityId, :relatedEntityType)"
        );
    query.bindValue(":projectId", projectId > 0 ? projectId : QVariant(QVariant::Int));
    query.bindValue(":type", type);
    query.bindValue(":message", message);
    query.bindValue(":relatedEntityId", relatedEntityId > 0 ? relatedEntityId : QVariant(QVariant::Int));
    query.bindValue(":relatedEntityType", relatedEntityType.isEmpty() ? QVariant(QVariant::String) : relatedEntityType);

    if (!query.exec()) {
        qDebug() << "Failed to add notification:" << query.lastError().text();
    } else {
        // Reload notifications
        loadNotifications();
    }
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

    isConnected = true; // Marquer comme connecté
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
        // Get project name before deletion for notification
        QString projectName;
        QSqlQuery nameQuery;
        nameQuery.prepare("SELECT NOM_PR FROM PROJET WHERE ID_PR = :id");
        nameQuery.bindValue(":id", projectId);
        if (nameQuery.exec() && nameQuery.next()) {
            projectName = nameQuery.value("NOM_PR").toString();
        }

        // First, delete all notifications related to this project
        QSqlQuery deleteNotificationsQuery;
        deleteNotificationsQuery.prepare("DELETE FROM NOTIFICATIONS WHERE PROJECT_ID = :projectId");
        deleteNotificationsQuery.bindValue(":projectId", projectId);

        if (!deleteNotificationsQuery.exec()) {
            qDebug() << "Failed to delete notifications:" << deleteNotificationsQuery.lastError().text();
            QMessageBox::critical(this, "Erreur", "Impossible de supprimer les notifications associées au projet");
            return;
        }

        // Now delete the project
        if (Projet::deleteById(projectId, query)) {
            QMessageBox::information(this, "Succès", "Projet supprimé avec succès");

            // Add a system notification (not linked to the deleted project)
            addNotification(-1, "PROJECT_DELETED",
                            "Le projet \"" + projectName + "\" a été supprimé");

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

        // Get the new project ID
        int newProjectId = 0;
        QSqlQuery idQuery;
        idQuery.prepare("SELECT MAX(ID_PR) as ID FROM PROJET WHERE NOM_PR = :nom");
        idQuery.bindValue(":nom", nom);
        if (idQuery.exec() && idQuery.next()) {
            newProjectId = idQuery.value("ID").toInt();
        }

        // Add notification
        addNotification(-1, "PROJECT_ADDED",
                        "Nouveau projet \"" + nom + "\" ajouté");

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

            // Add notification
            addNotification(-1, "PDF_EXPORT",
                            "Les projets ont été exportés en PDF");
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

    // Get original project data for notification
    Projet originalProjet = Projet::fetchProjetById(id, query);
    QStringList changes;

    if (originalProjet.getNomPr() != nom) {
        changes << "Nom: " + originalProjet.getNomPr() + " → " + nom;
    }
    if (originalProjet.getComplexite() != complexite) {
        changes << "Complexité: " + originalProjet.getComplexite() + " → " + complexite;
    }
    if (originalProjet.getStatutPr() != statut) {
        changes << "Statut: " + originalProjet.getStatutPr() + " → " + statut;
    }
    if (originalProjet.getBudget() != budget) {
        changes << "Budget: " + QString::number(originalProjet.getBudget()) + " → " + QString::number(budget);
    }
    if (originalProjet.getIdClient() != idClient) {
        QString oldClient = Projet::getClientNameById(originalProjet.getIdClient(), query);
        QString newClient = Projet::getClientNameById(idClient, query);
        changes << "Client: " + oldClient + " → " + newClient;
    }
    if (originalProjet.getIdEmployee() != idEmployee) {
        QString oldEmployee = Projet::getEmployeeNameById(originalProjet.getIdEmployee(), query);
        QString newEmployee = Projet::getEmployeeNameById(idEmployee, query);
        changes << "Employé: " + oldEmployee + " → " + newEmployee;
    }
    if (originalProjet.getDateDebut() != dateDebut) {
        changes << "Date début: " + originalProjet.getDateDebut().toString("dd/MM/yyyy") + " → " + dateDebut.toString("dd/MM/yyyy");
    }
    if (originalProjet.getDateFin() != dateFin) {
        changes << "Date fin: " + originalProjet.getDateFin().toString("dd/MM/yyyy") + " → " + dateFin.toString("dd/MM/yyyy");
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

        // Add notification
        if (!changes.isEmpty()) {
            QString changeMessage = "Projet \"" + nom + "\" modifié: " + changes.join(", ");
            addNotification(id, "PROJECT_MODIFIED", changeMessage);
        }

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
