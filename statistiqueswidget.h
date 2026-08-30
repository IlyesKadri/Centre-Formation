#ifndef STATISTIQUESWIDGET_H
#define STATISTIQUESWIDGET_H

#include <QWidget>
#include <QStandardItemModel>
#include <QChart>
#include <QChartView>
#include "statistiques.h"

namespace Ui {
class StatistiquesWidget;
}

class StatistiquesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit StatistiquesWidget(QWidget *parent = nullptr);
    ~StatistiquesWidget();

    void rafraichirStatistiques();

signals:
    void statusMessage(const QString &message, int timeout = 0);

private slots:
    void on_btnStatsRafraichir_clicked();
    void on_btnStatTabFormation_clicked();
    void on_btnStatTabNiveau_clicked();
    void on_btnStatTabInscCours_clicked();
    void on_btnStatTabStatuts_clicked();

private:
    Ui::StatistiquesWidget *ui;

    // Modèles de données des tableaux récapitulatifs
    QStandardItemModel *statsFormationModel;
    QStandardItemModel *statsNiveauModel;
    QStandardItemModel *statsInscCoursModel;
    QStandardItemModel *statsStatutsModel;

    // Vues graphiques QtCharts
    QChartView *chartViewFormation;
    QChartView *chartViewNiveau;
    QChartView *chartViewInscCours;
    QChartView *chartViewStatuts;

    void setupTables();
    void setupCharts();
    void setActiveTab(int tabIndex);
    void updateKpis();
};

#endif // STATISTIQUESWIDGET_H
