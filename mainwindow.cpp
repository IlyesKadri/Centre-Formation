#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "connection.h"

#include <QMessageBox>
#include <QDate>
#include <QFileDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QDebug>
#include <QTabWidget>
#include <QScrollArea>
#include <QSqlQuery>
#include <QSqlError>

#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , stagiairesWidget(nullptr)
    , coursWidget(nullptr)
    , inscriptionsWidget(nullptr)
    , statistiquesWidget(nullptr)
    , recommandationWidget(nullptr)
    , dashRecentInscModel(new QStandardItemModel(this))
    , dashPopularCoursModel(new QStandardItemModel(this))
{
    ui->setupUi(this);

    // Initialisation du module découplé Stagiaires
    stagiairesWidget = new StagiairesWidget(this);
    ui->stagiairesContainerLayout->addWidget(stagiairesWidget);
    connect(stagiairesWidget, &StagiairesWidget::stagiairesModifies, this, [this]() {
        if (inscriptionsWidget) inscriptionsWidget->chargerComboBoxes();
        if (recommandationWidget) recommandationWidget->chargerStagiaires();
        rafraichirDashboard();
        if (statistiquesWidget) statistiquesWidget->rafraichirStatistiques();
    });
    connect(stagiairesWidget, &StagiairesWidget::statusMessage, this, [this](const QString &msg, int to) {
        ui->statusbar->showMessage(msg, to);
    });

    // Initialisation du module découplé Cours
    coursWidget = new CoursWidget(this);
    ui->coursContainerLayout->addWidget(coursWidget);
    connect(coursWidget, &CoursWidget::coursModifies, this, [this]() {
        if (inscriptionsWidget) inscriptionsWidget->chargerComboBoxes();
        rafraichirDashboard();
        if (statistiquesWidget) statistiquesWidget->rafraichirStatistiques();
    });
    connect(coursWidget, &CoursWidget::statusMessage, this, [this](const QString &msg, int to) {
        ui->statusbar->showMessage(msg, to);
    });

    // Initialisation du module découplé Inscriptions
    inscriptionsWidget = new InscriptionsWidget(this);
    ui->inscriptionsContainerLayout->addWidget(inscriptionsWidget);
    connect(inscriptionsWidget, &InscriptionsWidget::inscriptionsModifiees, this, [this]() {
        if (coursWidget) coursWidget->rafraichirDonnees();
        rafraichirDashboard();
        if (statistiquesWidget) statistiquesWidget->rafraichirStatistiques();
    });
    connect(inscriptionsWidget, &InscriptionsWidget::statusMessage, this, [this](const QString &msg, int to) {
        ui->statusbar->showMessage(msg, to);
    });

    // Initialisation du module découplé Statistiques
    statistiquesWidget = new StatistiquesWidget(this);
    ui->statistiquesContainerLayout->addWidget(statistiquesWidget);
    connect(statistiquesWidget, &StatistiquesWidget::statusMessage, this, [this](const QString &msg, int to) {
        ui->statusbar->showMessage(msg, to);
    });

    // Initialisation du module découplé Recommandations IA
    recommandationWidget = new RecommandationWidget(this);
    ui->recommandationsContainerLayout->addWidget(recommandationWidget);
    connect(recommandationWidget, &RecommandationWidget::statusMessage, this, [this](const QString &msg, int to) {
        ui->statusbar->showMessage(msg, to);
    });
    connect(recommandationWidget, &RecommandationWidget::inscriptionDemandee, this, [this](int idStagiaire, int idCours) {
        setNavActiveButton(3);
        if (inscriptionsWidget) {
            inscriptionsWidget->preRemplirInscription(idStagiaire, idCours);
        }
    });

    // Configuration des tables et des modules
    setupDashboardTables();

    // Initialisation des données Dashboard
    rafraichirDashboard();

    // Navigation par défaut sur le Dashboard
    setNavActiveButton(0);
}

MainWindow::~MainWindow()
{
    delete ui;
}

// ============================================================
// NAVIGATION SIDEBAR
// ============================================================

void MainWindow::setNavActiveButton(int pageIndex)
{
    ui->stackedWidget->setCurrentIndex(pageIndex);

    ui->btnNavDashboard->setChecked(pageIndex == 0);
    ui->btnNavStagiaires->setChecked(pageIndex == 1);
    ui->btnNavCours->setChecked(pageIndex == 2);
    ui->btnNavInscriptions->setChecked(pageIndex == 3);
    ui->btnNavStatistiques->setChecked(pageIndex == 4);
    ui->btnNavRecommandations->setChecked(pageIndex == 5);

    switch (pageIndex) {
    case 0:
        ui->lblPageTitle->setText("Tableau de Bord");
        ui->lblPageSubtitle->setText("Vue d'ensemble et indicateurs clés");
        rafraichirDashboard();
        break;
    case 1:
        ui->lblPageTitle->setText("Gestion des Stagiaires");
        ui->lblPageSubtitle->setText("Ajout, modification, suppression et recherche multicritère");
        if (stagiairesWidget) {
            stagiairesWidget->rafraichirDonnees();
        }
        break;
    case 2:
        ui->lblPageTitle->setText("Gestion des Cours");
        ui->lblPageSubtitle->setText("Catalogue des formations et plannings");
        if (coursWidget) {
            coursWidget->rafraichirDonnees();
        }
        break;
    case 3:
        ui->lblPageTitle->setText("Gestion des Inscriptions");
        ui->lblPageSubtitle->setText("Inscriptions aux cours et impression d'attestations PDF");
        if (inscriptionsWidget) {
            inscriptionsWidget->rafraichirDonnees();
        }
        break;
    case 4:
        ui->lblPageTitle->setText("Statistiques & Rapports");
        ui->lblPageSubtitle->setText("Analyses graphiques et répartition des données Oracle");
        if (statistiquesWidget) {
            statistiquesWidget->rafraichirStatistiques();
        }
        break;
    case 5:
        ui->lblPageTitle->setText("Recommandations Intelligentes");
        ui->lblPageSubtitle->setText("Système expert d'orientation pédagogique basé sur l'IA et Oracle");
        if (recommandationWidget) {
            recommandationWidget->chargerStagiaires();
        }
        break;
    }
}

void MainWindow::on_btnNavDashboard_clicked() { setNavActiveButton(0); }
void MainWindow::on_btnNavStagiaires_clicked() { setNavActiveButton(1); }
void MainWindow::on_btnNavCours_clicked() { setNavActiveButton(2); }
void MainWindow::on_btnNavInscriptions_clicked() { setNavActiveButton(3); }
void MainWindow::on_btnNavStatistiques_clicked() { setNavActiveButton(4); }
void MainWindow::on_btnNavRecommandations_clicked() { setNavActiveButton(5); }


// ============================================================
// MODULE DASHBOARD
// ============================================================

void MainWindow::setupDashboardTables()
{
    // Table Dernières Inscriptions
    dashRecentInscModel->setHorizontalHeaderLabels({"Stagiaire", "Cours", "Date", "Statut"});
    ui->tableDashRecentInsc->setModel(dashRecentInscModel);
    ui->tableDashRecentInsc->horizontalHeader()->setStretchLastSection(true);
    ui->tableDashRecentInsc->setColumnWidth(0, 140);
    ui->tableDashRecentInsc->setColumnWidth(1, 140);
    ui->tableDashRecentInsc->setColumnWidth(2, 90);

    // Table Cours Populaires
    dashPopularCoursModel->setHorizontalHeaderLabels({"Nom du Cours", "Nombre d'Inscrits"});
    ui->tableDashPopularCours->setModel(dashPopularCoursModel);
    ui->tableDashPopularCours->horizontalHeader()->setStretchLastSection(true);
    ui->tableDashPopularCours->setColumnWidth(0, 220);
}

void MainWindow::rafraichirDashboard()
{
    // 1. Récupération des métriques globaux
    DashboardMetrics m = DashboardService::getMetrics();
    ui->lblCard1Value->setText(QString::number(m.totalStagiaires));
    ui->lblCard2Value->setText(QString::number(m.totalCours));
    ui->lblCard3Value->setText(QString::number(m.totalInscriptions));
    ui->lblCard4Value->setText(QString::number(m.coursDisponibles));

    // 2. Table Dernières Inscriptions
    dashRecentInscModel->removeRows(0, dashRecentInscModel->rowCount());
    QList<Inscription> recents = DashboardService::getDernieresInscriptions(5);
    for (const Inscription &ins : recents) {
        QList<QStandardItem*> row;
        row.append(new QStandardItem(ins.getNomStagiaire()));
        row.append(new QStandardItem(ins.getNomCours()));
        row.append(new QStandardItem(ins.getDateInscription().toString("dd/MM/yyyy")));
        row.append(new QStandardItem(ins.getStatut()));
        dashRecentInscModel->appendRow(row);
    }

    // 3. Table Cours les plus Populaires
    dashPopularCoursModel->removeRows(0, dashPopularCoursModel->rowCount());
    QList<QPair<QString, int>> pop = DashboardService::getCoursPopulaires(5);
    for (const auto &pair : pop) {
        QList<QStandardItem*> row;
        row.append(new QStandardItem(pair.first));
        row.append(new QStandardItem(QString::number(pair.second)));
        dashPopularCoursModel->appendRow(row);
    }
}




