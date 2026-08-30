#include "statistiqueswidget.h"
#include "ui_statistiques.h"
#include <QMessageBox>
#include <QPieSeries>
#include <QPieSlice>
#include <QBarSeries>
#include <QBarSet>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include <QFont>
#include <QColor>
#include <QBrush>

StatistiquesWidget::StatistiquesWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::StatistiquesWidget)
    , statsFormationModel(new QStandardItemModel(this))
    , statsNiveauModel(new QStandardItemModel(this))
    , statsInscCoursModel(new QStandardItemModel(this))
    , statsStatutsModel(new QStandardItemModel(this))
    , chartViewFormation(nullptr)
    , chartViewNiveau(nullptr)
    , chartViewInscCours(nullptr)
    , chartViewStatuts(nullptr)
{
    ui->setupUi(this);

    setupTables();
    setupCharts();
    setActiveTab(0);
    rafraichirStatistiques();
}

StatistiquesWidget::~StatistiquesWidget()
{
    delete ui;
}

void StatistiquesWidget::setupTables()
{
    QString tableStyle =
        "QTableView {"
        "    background-color: #0f172a;"
        "    color: #f8fafc;"
        "    gridline-color: #334155;"
        "    border: 1px solid #334155;"
        "    border-radius: 8px;"
        "    selection-background-color: #0284c7;"
        "    selection-color: #ffffff;"
        "    font-size: 13px;"
        "    outline: none;"
        "}"
        "QHeaderView {"
        "    background-color: #1e293b;"
        "    border: none;"
        "}"
        "QHeaderView::section {"
        "    background-color: #1e293b;"
        "    color: #38bdf8;"
        "    font-weight: bold;"
        "    font-size: 13px;"
        "    padding: 0px 10px;"
        "    border: none;"
        "    border-right: 1px solid #334155;"
        "    border-bottom: 1px solid #334155;"
        "}";

    auto formatTable = [tableStyle](QTableView *tbl, QStandardItemModel *model, const QStringList &headers, bool threeCols) {
        model->setHorizontalHeaderLabels(headers);
        tbl->setModel(model);
        tbl->setStyleSheet(tableStyle);
        tbl->verticalHeader()->setVisible(false); // Masque la colonne grise des numéros de ligne
        tbl->setShowGrid(true);
        tbl->setAlternatingRowColors(true);
        tbl->setSelectionBehavior(QAbstractItemView::SelectRows);
        tbl->setSelectionMode(QAbstractItemView::SingleSelection);
        tbl->verticalHeader()->setDefaultSectionSize(34);
        tbl->horizontalHeader()->setFixedHeight(40);
        tbl->horizontalHeader()->setHighlightSections(false);
        tbl->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        if (!threeCols) {
            tbl->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
            tbl->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
            tbl->setColumnWidth(1, 100);
        } else {
            tbl->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
            tbl->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
            tbl->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
            tbl->setColumnWidth(2, 80);
        }
    };

    formatTable(ui->tableStatsFormation, statsFormationModel, {"Filière", "Stagiaires"}, false);
    formatTable(ui->tableStatsNiveau, statsNiveauModel, {"Niveau", "Stagiaires"}, false);
    formatTable(ui->tableStatsInscCours, statsInscCoursModel, {"Cours", "Inscrits"}, false);
    formatTable(ui->tableStatsStatuts, statsStatutsModel, {"Entité", "Statut", "Total"}, true);
}

void StatistiquesWidget::setupCharts()
{
    auto createChartHelper = [](const QString &title) -> QChartView* {
        QChart *chart = new QChart();
        chart->setBackgroundBrush(QBrush(QColor("#1e293b")));
        chart->setTitle(title);
        chart->setTitleFont(QFont("Segoe UI", 14, QFont::Bold));
        chart->setTitleBrush(QBrush(QColor("#f8fafc")));
        chart->legend()->setLabelBrush(QBrush(QColor("#cbd5e1")));
        chart->legend()->setFont(QFont("Segoe UI", 10));
        chart->setAnimationOptions(QChart::SeriesAnimations);

        QChartView *view = new QChartView(chart);
        view->setRenderHint(QPainter::Antialiasing);
        view->setStyleSheet("background-color: transparent; border: none;");
        view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        return view;
    };

    chartViewFormation = createChartHelper("🎓 Répartition des Stagiaires par Filière de Formation");
    ui->layFormationChart->addWidget(chartViewFormation);

    chartViewNiveau = createChartHelper("📊 Répartition des Stagiaires selon le Niveau d'Étude");
    ui->layNiveauChart->addWidget(chartViewNiveau);

    chartViewInscCours = createChartHelper("📚 Nombre d'Inscriptions par Cours");
    ui->layInscCoursChart->addWidget(chartViewInscCours);

    chartViewStatuts = createChartHelper("🏷️ Répartition des Statuts (Stagiaires & Inscriptions)");
    ui->layStatutsChart->addWidget(chartViewStatuts);
}

void StatistiquesWidget::setActiveTab(int tabIndex)
{
    ui->stackedStats->setCurrentIndex(tabIndex);

    QString activeStyle =
        "QPushButton { background-color: #0284c7; color: #ffffff; border: 1px solid #38bdf8; "
        "border-radius: 8px; padding: 10px 18px; font-size: 13px; font-weight: bold; }";
    QString inactiveStyle =
        "QPushButton { background-color: #1e293b; color: #94a3b8; border: 1px solid #334155; "
        "border-radius: 8px; padding: 10px 18px; font-size: 13px; font-weight: 500; }"
        "QPushButton:hover { background-color: #334155; color: #ffffff; }";

    ui->btnStatTabFormation->setStyleSheet(tabIndex == 0 ? activeStyle : inactiveStyle);
    ui->btnStatTabNiveau->setStyleSheet(tabIndex == 1 ? activeStyle : inactiveStyle);
    ui->btnStatTabInscCours->setStyleSheet(tabIndex == 2 ? activeStyle : inactiveStyle);
    ui->btnStatTabStatuts->setStyleSheet(tabIndex == 3 ? activeStyle : inactiveStyle);
}

void StatistiquesWidget::on_btnStatTabFormation_clicked() { setActiveTab(0); }
void StatistiquesWidget::on_btnStatTabNiveau_clicked() { setActiveTab(1); }
void StatistiquesWidget::on_btnStatTabInscCours_clicked() { setActiveTab(2); }
void StatistiquesWidget::on_btnStatTabStatuts_clicked() { setActiveTab(3); }

void StatistiquesWidget::updateKpis()
{
    double txOcc = StatistiquesService::getTauxOccupation();
    ui->lblKpiOccupation->setText(QString("%1 %").arg(QString::number(txOcc, 'f', 1)));

    double moyNotes = StatistiquesService::getMoyenneNotes();
    ui->lblKpiNotes->setText(QString("%1 / 20").arg(QString::number(moyNotes, 'f', 2)));

    QString topForm = StatistiquesService::getFormationLaPlusDemandee();
    ui->lblKpiTopFormation->setText(topForm.isEmpty() ? "N/A" : topForm);

    QString topCours = StatistiquesService::getCoursLePlusDemande();
    ui->lblKpiTopCours->setText(topCours.isEmpty() ? "N/A" : topCours);
}

void StatistiquesWidget::rafraichirStatistiques()
{
    // 1. Mise à jour des KPI Oracle
    updateKpis();

    // 2. Stagiaires par Formation (Grand Donut Chart + Table)
    statsFormationModel->removeRows(0, statsFormationModel->rowCount());
    auto listForm = StatistiquesService::getStagiairesParFormation();
    int totalStag = 0;
    for (const auto &pair : listForm) totalStag += pair.second;

    for (const auto &pair : listForm) {
        QList<QStandardItem*> row;
        row.append(new QStandardItem(pair.first));
        QStandardItem *countItem = new QStandardItem(QString::number(pair.second));
        countItem->setTextAlignment(Qt::AlignCenter);
        row.append(countItem);
        statsFormationModel->appendRow(row);
    }

    if (chartViewFormation && chartViewFormation->chart()) {
        QChart *chart = chartViewFormation->chart();
        chart->removeAllSeries();
        QPieSeries *pie = new QPieSeries();
        pie->setHoleSize(0.42);

        const QStringList colors = {"#38bdf8", "#34d399", "#fbbf24", "#a78bfa", "#f472b6", "#60a5fa", "#e879f9"};
        int cIdx = 0;
        for (const auto &pair : listForm) {
            double pct = (totalStag > 0) ? (static_cast<double>(pair.second) / totalStag * 100.0) : 0.0;
            QPieSlice *slice = pie->append(QString("%1 (%2%)").arg(pair.first).arg(QString::number(pct, 'f', 0)), pair.second);
            slice->setColor(QColor(colors[cIdx % colors.size()]));
            slice->setLabelColor(QColor("#f8fafc"));
            slice->setLabelFont(QFont("Segoe UI", 10, QFont::Bold));
            slice->setLabelVisible(true);
            cIdx++;
        }
        chart->addSeries(pie);
        chart->legend()->setAlignment(Qt::AlignRight);
    }

    // 3. Stagiaires par Niveau (Grand Donut Chart + Table)
    statsNiveauModel->removeRows(0, statsNiveauModel->rowCount());
    auto listNiv = StatistiquesService::getStagiairesParNiveau();
    for (const auto &pair : listNiv) {
        QList<QStandardItem*> row;
        row.append(new QStandardItem(pair.first));
        QStandardItem *countItem = new QStandardItem(QString::number(pair.second));
        countItem->setTextAlignment(Qt::AlignCenter);
        row.append(countItem);
        statsNiveauModel->appendRow(row);
    }

    if (chartViewNiveau && chartViewNiveau->chart()) {
        QChart *chart = chartViewNiveau->chart();
        chart->removeAllSeries();
        QPieSeries *pie = new QPieSeries();
        pie->setHoleSize(0.42);

        for (const auto &pair : listNiv) {
            double pct = (totalStag > 0) ? (static_cast<double>(pair.second) / totalStag * 100.0) : 0.0;
            QPieSlice *slice = pie->append(QString("%1 (%2%)").arg(pair.first).arg(QString::number(pct, 'f', 0)), pair.second);
            if (pair.first == "AVANCE") slice->setColor(QColor("#8b5cf6"));
            else if (pair.first == "INTERMEDIAIRE") slice->setColor(QColor("#0284c7"));
            else slice->setColor(QColor("#38bdf8"));
            slice->setLabelColor(QColor("#f8fafc"));
            slice->setLabelFont(QFont("Segoe UI", 11, QFont::Bold));
            slice->setLabelVisible(true);
        }
        chart->addSeries(pie);
        chart->legend()->setAlignment(Qt::AlignRight);
    }

    // 4. Inscriptions par Cours (Bar Chart + Table)
    statsInscCoursModel->removeRows(0, statsInscCoursModel->rowCount());
    auto listInscC = StatistiquesService::getInscriptionsParCours();
    for (const auto &pair : listInscC) {
        QList<QStandardItem*> row;
        row.append(new QStandardItem(pair.first));
        QStandardItem *countItem = new QStandardItem(QString::number(pair.second));
        countItem->setTextAlignment(Qt::AlignCenter);
        row.append(countItem);
        statsInscCoursModel->appendRow(row);
    }

    if (chartViewInscCours && chartViewInscCours->chart()) {
        QChart *chart = chartViewInscCours->chart();
        chart->removeAllSeries();
        for (auto axis : chart->axes()) {
            chart->removeAxis(axis);
            delete axis;
        }

        QBarSet *set = new QBarSet("Inscriptions Enregistrées");
        set->setColor(QColor("#0284c7"));
        set->setLabelColor(QColor("#ffffff"));

        QStringList categories;
        int maxVal = 1;
        for (const auto &pair : listInscC) {
            *set << pair.second;
            categories << pair.first;
            if (pair.second > maxVal) maxVal = pair.second;
        }

        QBarSeries *barSeries = new QBarSeries();
        barSeries->append(set);
        barSeries->setLabelsVisible(true);
        barSeries->setLabelsPosition(QAbstractBarSeries::LabelsOutsideEnd);
        chart->addSeries(barSeries);

        QBarCategoryAxis *axisX = new QBarCategoryAxis();
        axisX->append(categories);
        axisX->setLabelsColor(QColor("#cbd5e1"));
        axisX->setLabelsFont(QFont("Segoe UI", 10));
        chart->addAxis(axisX, Qt::AlignBottom);
        barSeries->attachAxis(axisX);

        QValueAxis *axisY = new QValueAxis();
        axisY->setRange(0, maxVal + 2);
        axisY->setLabelsColor(QColor("#cbd5e1"));
        axisY->setLabelFormat("%d");
        chart->addAxis(axisY, Qt::AlignLeft);
        barSeries->attachAxis(axisY);

        chart->legend()->setVisible(false);
    }

    // 5. Répartition des Statuts (Donut Chart + Table)
    statsStatutsModel->removeRows(0, statsStatutsModel->rowCount());
    auto listStatStag = StatistiquesService::getRepartitionStatutsStagiaire();
    for (const auto &pair : listStatStag) {
        QList<QStandardItem*> row;
        row.append(new QStandardItem("Stagiaire"));
        row.append(new QStandardItem(pair.first));
        QStandardItem *countItem = new QStandardItem(QString::number(pair.second));
        countItem->setTextAlignment(Qt::AlignCenter);
        row.append(countItem);
        statsStatutsModel->appendRow(row);
    }

    auto listStatInsc = StatistiquesService::getRepartitionStatutsInscription();
    for (const auto &pair : listStatInsc) {
        QList<QStandardItem*> row;
        row.append(new QStandardItem("Inscription"));
        row.append(new QStandardItem(pair.first));
        QStandardItem *countItem = new QStandardItem(QString::number(pair.second));
        countItem->setTextAlignment(Qt::AlignCenter);
        row.append(countItem);
        statsStatutsModel->appendRow(row);
    }

    if (chartViewStatuts && chartViewStatuts->chart()) {
        QChart *chart = chartViewStatuts->chart();
        chart->removeAllSeries();
        for (auto axis : chart->axes()) {
            chart->removeAxis(axis);
            delete axis;
        }

        QPieSeries *pie = new QPieSeries();
        pie->setHoleSize(0.42);

        for (const auto &pair : listStatStag) {
            QPieSlice *s = pie->append("Stagiaire : " + pair.first, pair.second);
            s->setLabelVisible(true);
            s->setLabelColor(QColor("#f8fafc"));
            s->setLabelFont(QFont("Segoe UI", 10, QFont::Bold));
        }
        for (const auto &pair : listStatInsc) {
            QPieSlice *s = pie->append("Inscription : " + pair.first, pair.second);
            s->setLabelVisible(true);
            s->setLabelColor(QColor("#f8fafc"));
            s->setLabelFont(QFont("Segoe UI", 10, QFont::Bold));
        }

        chart->addSeries(pie);
        chart->legend()->setAlignment(Qt::AlignRight);
    }
}

void StatistiquesWidget::on_btnStatsRafraichir_clicked()
{
    rafraichirStatistiques();
    emit statusMessage("✓ Données statistiques rafraîchies depuis Oracle XE", 3000);
    QMessageBox::information(this, "Statistiques Oracle", "Les données statistiques ont été rafraîchies depuis Oracle XE.");
}
