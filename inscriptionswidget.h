#ifndef INSCRIPTIONSWIDGET_H
#define INSCRIPTIONSWIDGET_H

#include <QWidget>
#include <QStandardItemModel>
#include <QTimer>
#include <QStringList>
#include "inscription.h"
#include "birdemailservice.h"

namespace Ui {
class InscriptionsWidget;
}

class InscriptionsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit InscriptionsWidget(QWidget *parent = nullptr);
    ~InscriptionsWidget();

    void rafraichirDonnees();
    void chargerComboBoxes();
    void viderFormulaire();
    void preRemplirInscription(int idStagiaire, int idCours);

signals:
    void inscriptionsModifiees();
    void statusMessage(const QString &message, int timeout = 0);

private slots:
    void on_btnInscAjouter_clicked();
    void on_btnInscModifier_clicked();
    void on_btnInscSupprimer_clicked();
    void on_btnInscVider_clicked();
    void on_btnInscGenererPdf_clicked();
    void on_btnInscRenvoyerEmail_clicked();
    void on_tableInscriptions_clicked(const QModelIndex &index);

    void onSearchInscGlobalTextChanged(const QString &text);
    void executerRechercheInscGlobale();
    void on_btnInscAppliquerTri_clicked();
    void on_btnInscResetTri_clicked();

    void onComboCrit1Changed(int index);
    void onComboCrit2Changed(int index);
    void onComboCrit3Changed(int index);

    void onEmailSentSuccess(int idInscription, const QString &destinataire, const QString &messageId, const QString &status);
    void onEmailSentError(int idInscription, const QString &destinataire, const QString &erreur);

private:
    Ui::InscriptionsWidget *ui;
    QStandardItemModel *inscriptionsModel;
    QTimer *searchDebounceTimer;

    // Service Bird Email
    BirdEmailService *emailService;
    int emailBatchTotal;
    int emailBatchCompleted;
    int emailBatchSuccess;
    int emailBatchFailed;
    QStringList emailBatchSuccessList;
    QStringList emailBatchFailedList;

    void startEmailBatch(int totalEmails = 1);
    void finaliserEmailBatchSiTermine();
    void setupTable();
    void chargerInscriptions(const QList<Inscription> &liste);
    void actualiserOrdreCombo(class QComboBox *comboCrit, class QComboBox *comboOrdre);
};

#endif // INSCRIPTIONSWIDGET_H
