#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QLabel>
#include <QWidget>
#include <QStackedWidget>
#include <QPushButton>
#include <QComboBox>
#include <QVBoxLayout>
#include <QTimer>
#include "stagiaire.h"
#include "cours.h"
#include "inscription.h"
#include "dashboard.h"
#include "statistiques.h"
#include "groqrecommendationservice.h"
#include "birdemailservice.h"

#include "stagiaireswidget.h"
#include "courswidget.h"
#include "inscriptionswidget.h"
#include "statistiqueswidget.h"
#include "recommandationwidget.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Navigation sidebar
    void on_btnNavDashboard_clicked();
    void on_btnNavStagiaires_clicked();
    void on_btnNavCours_clicked();
    void on_btnNavInscriptions_clicked();
    void on_btnNavStatistiques_clicked();
    void on_btnNavRecommandations_clicked();

private:
    Ui::MainWindow *ui;

    // Modules Découplés
    StagiairesWidget *stagiairesWidget;
    CoursWidget *coursWidget;
    InscriptionsWidget *inscriptionsWidget;
    StatistiquesWidget *statistiquesWidget;
    RecommandationWidget *recommandationWidget;

    // Modèles de données
    QStandardItemModel *dashRecentInscModel;
    QStandardItemModel *dashPopularCoursModel;

    // Dashboard
    void setupDashboardTables();
    void rafraichirDashboard();

    void setNavActiveButton(int pageIndex);
};

#endif // MAINWINDOW_H
