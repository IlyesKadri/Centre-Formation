#ifndef STAGIAIRESWIDGET_H
#define STAGIAIRESWIDGET_H

#include <QWidget>
#include <QStandardItemModel>
#include <QTimer>
#include "stagiaire.h"

namespace Ui {
class StagiairesWidget;
}

class StagiairesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit StagiairesWidget(QWidget *parent = nullptr);
    ~StagiairesWidget();

    // Recharger la liste des stagiaires depuis Oracle
    void rafraichirDonnees();

signals:
    void stagiairesModifies();
    void statusMessage(const QString &message, int timeout = 3000);

private slots:
    void on_btnStagiaireAjouter_clicked();
    void on_btnStagiaireModifier_clicked();
    void on_btnStagiaireSupprimer_clicked();
    void on_btnStagiaireVider_clicked();
    void onSearchGlobalTextChanged(const QString &text);
    void executerRechercheGlobale();
    void on_btnStagiaireAppliquerTri_clicked();
    void on_btnStagiaireResetTri_clicked();
    void on_tableStagiaires_clicked(const QModelIndex &index);

    void onComboCrit1Changed(int index);
    void onComboCrit2Changed(int index);
    void onComboCrit3Changed(int index);

private:
    Ui::StagiairesWidget *ui;
    QStandardItemModel *stagiairesModel;
    QTimer *searchDebounceTimer;

    void setupTable();
    void chargerStagiaires(const QList<Stagiaire> &liste);
    void remplirFormulaire(const Stagiaire &s);
    void viderFormulaire();
    void actualiserOrdreCombo(class QComboBox *comboCrit, class QComboBox *comboOrdre);
};

#endif // STAGIAIRESWIDGET_H
