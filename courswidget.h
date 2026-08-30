#ifndef COURSWIDGET_H
#define COURSWIDGET_H

#include <QWidget>
#include <QStandardItemModel>
#include <QTimer>
#include "cours.h"

namespace Ui {
class CoursWidget;
}

class CoursWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CoursWidget(QWidget *parent = nullptr);
    ~CoursWidget();

    // Recharger la liste des cours depuis Oracle XE
    void rafraichirDonnees();

signals:
    void coursModifies();
    void statusMessage(const QString &message, int timeout = 3000);

private slots:
    void on_btnCoursAjouter_clicked();
    void on_btnCoursModifier_clicked();
    void on_btnCoursSupprimer_clicked();
    void on_btnCoursVider_clicked();
    void onSearchGlobalTextChanged(const QString &text);
    void executerRechercheGlobale();
    void on_btnCoursAppliquerTri_clicked();
    void on_btnCoursResetTri_clicked();
    void on_tableCours_clicked(const QModelIndex &index);

    void onComboCrit1Changed(int index);
    void onComboCrit2Changed(int index);
    void onComboCrit3Changed(int index);

private:
    Ui::CoursWidget *ui;
    QStandardItemModel *coursModel;
    QTimer *searchDebounceTimer;

    void setupTable();
    void chargerCours(const QList<Cours> &liste);
    void remplirFormulaire(const Cours &c);
    void viderFormulaire();
    void actualiserOrdreCombo(class QComboBox *comboCrit, class QComboBox *comboOrdre);
};

#endif // COURSWIDGET_H
