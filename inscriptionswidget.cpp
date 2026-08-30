#include "inscriptionswidget.h"
#include "ui_inscriptions.h"
#include "connection.h"
#include "stagiaire.h"
#include "cours.h"

#include <QMessageBox>
#include <QDate>
#include <QFileDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QSqlQuery>
#include <QSqlError>

InscriptionsWidget::InscriptionsWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::InscriptionsWidget)
    , inscriptionsModel(new QStandardItemModel(this))
    , searchDebounceTimer(new QTimer(this))
    , emailService(new BirdEmailService(this))
    , emailBatchTotal(0)
    , emailBatchCompleted(0)
    , emailBatchSuccess(0)
    , emailBatchFailed(0)
{
    ui->setupUi(this);

    // Masquer les champs ID (gérés automatiquement par Oracle)
    ui->lblInscId->hide();
    ui->txtInscId->hide();

    // Initialisation formulaire
    ui->dateInscDate->setDate(QDate::currentDate());

    // Configuration de la table
    setupTable();

    // Connexions du service Bird Email
    connect(emailService, &BirdEmailService::emailEnvoyeSucces, this, &InscriptionsWidget::onEmailSentSuccess);
    connect(emailService, &BirdEmailService::emailEchec, this, &InscriptionsWidget::onEmailSentError);

    // Configuration recherche globale avec debounce 250 ms
    searchDebounceTimer->setSingleShot(true);
    connect(searchDebounceTimer, &QTimer::timeout, this, &InscriptionsWidget::executerRechercheInscGlobale);
    connect(ui->txtSearchInscGlobal, &QLineEdit::textChanged, this, &InscriptionsWidget::onSearchInscGlobalTextChanged);

    // Initialisation dynamique des options d'ordre de tri selon le critère
    connect(ui->comboTriInscCrit1, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &InscriptionsWidget::onComboCrit1Changed);
    connect(ui->comboTriInscCrit2, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &InscriptionsWidget::onComboCrit2Changed);
    connect(ui->comboTriInscCrit3, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &InscriptionsWidget::onComboCrit3Changed);

    actualiserOrdreCombo(ui->comboTriInscCrit1, ui->comboTriInscOrdre1);
    actualiserOrdreCombo(ui->comboTriInscCrit2, ui->comboTriInscOrdre2);
    actualiserOrdreCombo(ui->comboTriInscCrit3, ui->comboTriInscOrdre3);

    // Chargement initial
    chargerComboBoxes();
    rafraichirDonnees();
}

InscriptionsWidget::~InscriptionsWidget()
{
    delete ui;
}

void InscriptionsWidget::setupTable()
{
    inscriptionsModel->setHorizontalHeaderLabels({
        "ID", "Stagiaire", "Cours", "Date Inscription", "Statut", "Note (/20)",
        "ID Stagiaire", "ID Cours"
    });

    ui->tableInscriptions->setModel(inscriptionsModel);
    ui->tableInscriptions->horizontalHeader()->setStretchLastSection(true);
    ui->tableInscriptions->setColumnHidden(0, true); // Masquer ID Inscription
    ui->tableInscriptions->setColumnHidden(6, true); // Masquer ID Stagiaire
    ui->tableInscriptions->setColumnHidden(7, true); // Masquer ID Cours

    ui->tableInscriptions->setColumnWidth(1, 200); // Stagiaire
    ui->tableInscriptions->setColumnWidth(2, 180); // Cours
    ui->tableInscriptions->setColumnWidth(3, 120); // Date Inscription
    ui->tableInscriptions->setColumnWidth(4, 100); // Statut
    ui->tableInscriptions->setColumnWidth(5, 90);  // Note
}

void InscriptionsWidget::rafraichirDonnees()
{
    executerRechercheInscGlobale();
}

void InscriptionsWidget::chargerInscriptions(const QList<Inscription> &liste)
{
    inscriptionsModel->removeRows(0, inscriptionsModel->rowCount());

    for (const Inscription &ins : liste) {
        QList<QStandardItem*> row;
        row.append(new QStandardItem(QString::number(ins.getIdInscription())));
        row.append(new QStandardItem(ins.getNomStagiaire()));
        row.append(new QStandardItem(ins.getNomCours()));
        row.append(new QStandardItem(ins.getDateInscription().toString("dd/MM/yyyy")));
        row.append(new QStandardItem(ins.getStatut()));
        row.append(new QStandardItem(QString::number(ins.getNote(), 'f', 2)));
        row.append(new QStandardItem(QString::number(ins.getIdStagiaire())));
        row.append(new QStandardItem(QString::number(ins.getIdCours())));

        inscriptionsModel->appendRow(row);
    }
}

void InscriptionsWidget::chargerComboBoxes()
{
    // Sauvegarder les sélections actuelles si existantes
    int curStagId = ui->comboInscStagiaire->currentData().toInt();
    int curCoursId = ui->comboInscCours->currentData().toInt();

    ui->comboInscStagiaire->clear();
    ui->comboInscCours->clear();

    QList<Stagiaire> stagiaires = Stagiaire::afficher();
    for (const Stagiaire &s : stagiaires) {
        ui->comboInscStagiaire->addItem(
            QString("%1 %2 (%3)").arg(s.getNom(), s.getPrenom(), s.getFormation()),
            s.getIdStagiaire()
        );
    }

    QList<Cours> cours = Cours::afficher();
    for (const Cours &c : cours) {
        int inscrits = Cours::compterInscriptionsActuelles(c.getIdCours());
        QString dispo = QString("%1/%2").arg(inscrits).arg(c.getCapacite());
        ui->comboInscCours->addItem(
            QString("%1 [%2] - %3€ (%4)").arg(c.getNom()).arg(c.getStatut()).arg(c.getPrix()).arg(dispo),
            c.getIdCours()
        );
    }

    if (curStagId > 0) {
        int idx = ui->comboInscStagiaire->findData(curStagId);
        if (idx >= 0) ui->comboInscStagiaire->setCurrentIndex(idx);
    }
    if (curCoursId > 0) {
        int idx = ui->comboInscCours->findData(curCoursId);
        if (idx >= 0) ui->comboInscCours->setCurrentIndex(idx);
    }
}

void InscriptionsWidget::viderFormulaire()
{
    ui->txtInscId->clear();
    ui->dateInscDate->setDate(QDate::currentDate());
    ui->comboInscStatut->setCurrentIndex(0);
    ui->doubleSpinInscNote->setValue(15.0);

    if (ui->comboInscStagiaire->count() > 0) ui->comboInscStagiaire->setCurrentIndex(0);
    if (ui->comboInscCours->count() > 0) ui->comboInscCours->setCurrentIndex(0);

    ui->tableInscriptions->clearSelection();
}

void InscriptionsWidget::preRemplirInscription(int idStagiaire, int idCours)
{
    chargerComboBoxes();
    viderFormulaire();

    int idxStag = ui->comboInscStagiaire->findData(idStagiaire);
    if (idxStag >= 0) ui->comboInscStagiaire->setCurrentIndex(idxStag);

    int idxCours = ui->comboInscCours->findData(idCours);
    if (idxCours >= 0) ui->comboInscCours->setCurrentIndex(idxCours);
}

void InscriptionsWidget::on_tableInscriptions_clicked(const QModelIndex &index)
{
    int row = index.row();

    ui->txtInscId->setText(inscriptionsModel->item(row, 0)->text());

    QDate dInsc = QDate::fromString(inscriptionsModel->item(row, 3)->text(), "dd/MM/yyyy");
    if (dInsc.isValid()) ui->dateInscDate->setDate(dInsc);

    ui->comboInscStatut->setCurrentText(inscriptionsModel->item(row, 4)->text());
    ui->doubleSpinInscNote->setValue(inscriptionsModel->item(row, 5)->text().toDouble());

    int idStagiaire = inscriptionsModel->item(row, 6)->text().toInt();
    int idCours = inscriptionsModel->item(row, 7)->text().toInt();

    int idxStag = ui->comboInscStagiaire->findData(idStagiaire);
    if (idxStag >= 0) ui->comboInscStagiaire->setCurrentIndex(idxStag);

    int idxCours = ui->comboInscCours->findData(idCours);
    if (idxCours >= 0) ui->comboInscCours->setCurrentIndex(idxCours);
}

void InscriptionsWidget::startEmailBatch(int totalEmails)
{
    emailBatchTotal = totalEmails;
    emailBatchCompleted = 0;
    emailBatchSuccess = 0;
    emailBatchFailed = 0;
    emailBatchSuccessList.clear();
    emailBatchFailedList.clear();
}

void InscriptionsWidget::finaliserEmailBatchSiTermine()
{
    if (emailBatchCompleted < emailBatchTotal) {
        emit statusMessage(QString("⏳ Traitement des emails Bird (%1/%2 terminés)...").arg(emailBatchCompleted).arg(emailBatchTotal), 3000);
        return;
    }

    if (emailBatchTotal <= 1) {
        if (emailBatchSuccess == 1) {
            emit statusMessage(QString("✓ Email envoyé avec succès via Bird (%1)").arg(emailBatchSuccessList.value(0)), 6000);
            QMessageBox::information(this, "Confirmation Inscription & Email",
                "✓ L'inscription est correctement enregistrée dans Oracle et l'email de confirmation a été envoyé avec succès.");
        } else {
            emit statusMessage("⚠ Inscription enregistrée, mais email non envoyé", 6000);
            QMessageBox::warning(this, "Notification Email",
                QString("⚠ L'inscription est correctement enregistrée dans Oracle,\n"
                        "mais l'email de confirmation n'a pas pu être envoyé :\n\n%1")
                    .arg(emailBatchFailedList.value(0)));
        }
    } else {
        if (emailBatchFailed == 0) {
            QString msg = QString("✓ Les %1 emails ont été envoyés avec succès via Bird.").arg(emailBatchTotal);
            emit statusMessage(msg, 6000);
            QMessageBox::information(this, "Confirmation Emails", msg);
        } else if (emailBatchSuccess > 0 && emailBatchFailed > 0) {
            QString detail = QString("⚠ %1 email(s) sur %2 ont été envoyés avec succès.\n\n"
                                     "✓ %3 email(s) envoyé(s)\n"
                                     "❌ %4 email(s) en échec")
                                 .arg(emailBatchSuccess)
                                 .arg(emailBatchTotal)
                                 .arg(emailBatchSuccess)
                                 .arg(emailBatchFailed);
            if (!emailBatchFailedList.isEmpty()) {
                detail += "\n\nDétails des échecs :\n• " + emailBatchFailedList.join("\n• ");
            }
            emit statusMessage(QString("⚠ %1 email(s) sur %2 envoyé(s)").arg(emailBatchSuccess).arg(emailBatchTotal), 6000);
            QMessageBox::warning(this, "Notification Emails", detail);
        } else {
            QString msg = QString("❌ Aucun des %1 emails n'a pu être envoyé via Bird.").arg(emailBatchTotal);
            if (!emailBatchFailedList.isEmpty()) {
                msg += "\n\nDétails :\n• " + emailBatchFailedList.join("\n• ");
            }
            emit statusMessage(QString("❌ Échec des %1 envois d'emails").arg(emailBatchTotal), 6000);
            QMessageBox::critical(this, "Erreur Envoi Emails", msg);
        }
    }
}

void InscriptionsWidget::onEmailSentSuccess(int idInscription, const QString &destinataire, const QString &messageId, const QString &status)
{
    Q_UNUSED(status);
    Q_UNUSED(messageId);
    emailBatchCompleted++;
    emailBatchSuccess++;
    emailBatchSuccessList.append(QString("Inscription #%1 (%2)").arg(idInscription).arg(destinataire));
    finaliserEmailBatchSiTermine();
}

void InscriptionsWidget::onEmailSentError(int idInscription, const QString &destinataire, const QString &erreur)
{
    emailBatchCompleted++;
    emailBatchFailed++;
    emailBatchFailedList.append(QString("Inscription #%1 (%2) : %3").arg(idInscription).arg(destinataire.isEmpty() ? "inconnu" : destinataire).arg(erreur));
    finaliserEmailBatchSiTermine();
}

void InscriptionsWidget::on_btnInscAjouter_clicked()
{
    if (ui->comboInscStagiaire->count() == 0 || ui->comboInscCours->count() == 0) {
        QMessageBox::warning(this, "Données manquantes", "Veuillez d'abord créer des stagiaires et des cours dans la base Oracle.");
        return;
    }

    int idStag = ui->comboInscStagiaire->currentData().toInt();
    int idCours = ui->comboInscCours->currentData().toInt();

    Inscription ins(
        0,
        idStag,
        idCours,
        ui->dateInscDate->date(),
        ui->comboInscStatut->currentText(),
        ui->doubleSpinInscNote->value()
    );

    QString erreur;
    if (!ins.estValide(erreur)) {
        QMessageBox::warning(this, "Validation impossible", erreur);
        return;
    }

    // Vérification Règle Métier (anti-doublon et capacité maximale)
    QString msgMetier;
    if (!Inscription::verifierRegleMetier(idStag, idCours, 0, msgMetier)) {
        QMessageBox::critical(this, "Inscription Impossible", msgMetier);
        return;
    }

    if (ins.ajouter()) {
        chargerComboBoxes();
        rafraichirDonnees();
        emit inscriptionsModifiees();

        // Récupérer l'ID de la nouvelle inscription créée dans Oracle
        QSqlQuery maxQ(Connection::instance()->getDatabase());
        int newIdInsc = 0;
        if (maxQ.exec("SELECT MAX(ID_INSCRIPTION) FROM INSCRIPTION") && maxQ.next()) {
            newIdInsc = maxQ.value(0).toInt();
        }

        // Déclencher l'envoi automatique de l'email de confirmation via Bird Email API
        if (newIdInsc > 0) {
            startEmailBatch(1);
            emailService->envoyerConfirmationInscription(newIdInsc);
            emit statusMessage("✓ Inscription enregistrée dans Oracle. Notification Bird en cours...", 4000);
        } else {
            QMessageBox::information(
                this,
                "Inscription Réussie",
                "L'inscription a été enregistrée avec succès dans la base Oracle XE."
            );
        }

        viderFormulaire();
    } else {
        QMessageBox::critical(this, "Erreur Oracle", "Échec de l'enregistrement de l'inscription.");
    }
}

void InscriptionsWidget::on_btnInscModifier_clicked()
{
    int idInsc = ui->txtInscId->text().toInt();
    if (idInsc <= 0) {
        QMessageBox::warning(this, "Avertissement", "Veuillez sélectionner une inscription dans la liste à modifier.");
        return;
    }

    int idStag = ui->comboInscStagiaire->currentData().toInt();
    int idCours = ui->comboInscCours->currentData().toInt();

    Inscription ins(
        idInsc,
        idStag,
        idCours,
        ui->dateInscDate->date(),
        ui->comboInscStatut->currentText(),
        ui->doubleSpinInscNote->value()
    );

    QString erreur;
    if (!ins.estValide(erreur)) {
        QMessageBox::warning(this, "Validation impossible", erreur);
        return;
    }

    // Vérification Règle Métier pour la modification
    QString msgMetier;
    if (!Inscription::verifierRegleMetier(idStag, idCours, idInsc, msgMetier)) {
        QMessageBox::critical(this, "Modification Impossible", msgMetier);
        return;
    }

    if (ins.modifier()) {
        QMessageBox::information(this, "Succès", "Inscription mise à jour avec succès.");
        chargerComboBoxes();
        rafraichirDonnees();
        emit inscriptionsModifiees();
        viderFormulaire();
    } else {
        QMessageBox::critical(this, "Erreur Oracle", "Échec de la modification de l'inscription.");
    }
}

void InscriptionsWidget::on_btnInscSupprimer_clicked()
{
    int idInsc = ui->txtInscId->text().toInt();
    if (idInsc <= 0) {
        QMessageBox::warning(this, "Avertissement", "Veuillez sélectionner une inscription à supprimer.");
        return;
    }

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(
        this,
        "Confirmation de suppression",
        QString("Êtes-vous sûr de vouloir supprimer l'inscription ID %1 ?").arg(idInsc),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        Inscription ins;
        ins.setIdInscription(idInsc);

        if (ins.supprimer()) {
            QMessageBox::information(this, "Succès", "Inscription supprimée avec succès.");
            chargerComboBoxes();
            rafraichirDonnees();
            emit inscriptionsModifiees();
            viderFormulaire();
        } else {
            QMessageBox::critical(this, "Erreur Oracle", "Échec de la suppression.");
        }
    }
}

void InscriptionsWidget::on_btnInscVider_clicked()
{
    viderFormulaire();
}

void InscriptionsWidget::on_btnInscGenererPdf_clicked()
{
    int idInsc = ui->txtInscId->text().toInt();
    if (idInsc <= 0) {
        QMessageBox::warning(this, "Avertissement", "Veuillez d'abord sélectionner une inscription dans le tableau pour générer l'attestation PDF.");
        return;
    }

    QString statut = ui->comboInscStatut->currentText().trimmed().toUpper();
    if (statut != "TERMINE") {
        QMessageBox::warning(
            this,
            "Génération Impossible",
            QString("Impossible de générer l'attestation PDF !\n\nL'attestation de réussite ne peut être délivrée que si le statut de l'inscription est « TERMINE ».\n\nStatut actuel de cette inscription : %1")
                .arg(ui->comboInscStatut->currentText())
        );
        return;
    }

    QString defaultName = QString("Attestation_Inscription_%1.pdf").arg(idInsc);
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "Enregistrer l'Attestation de Formation PDF",
        defaultName,
        "Fichiers PDF (*.pdf)"
    );

    if (filePath.isEmpty()) return;

    if (!filePath.endsWith(".pdf", Qt::CaseInsensitive)) {
        filePath += ".pdf";
    }

    QString err;
    if (Inscription::genererAttestationPdf(idInsc, filePath, err)) {
        QMessageBox::StandardButton reply = QMessageBox::information(
            this,
            "Attestation PDF Générée",
            QString("L'attestation de formation a été générée avec succès !\n\nFichier : %1\n\nVoulez-vous l'ouvrir maintenant ?").arg(filePath),
            QMessageBox::Yes | QMessageBox::No
        );

        if (reply == QMessageBox::Yes) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
        }
    } else {
        QMessageBox::critical(this, "Erreur Génération PDF", QString("Échec de la génération du PDF :\n%1").arg(err));
    }
}

void InscriptionsWidget::on_btnInscRenvoyerEmail_clicked()
{
    int idInsc = ui->txtInscId->text().toInt();
    if (idInsc <= 0) {
        QMessageBox::warning(this, "Avertissement", "Veuillez d'abord sélectionner une inscription dans le tableau pour renvoyer l'email.");
        return;
    }

    startEmailBatch(1);
    emailService->envoyerConfirmationInscription(idInsc);
    emit statusMessage("📧 Renvoi de l'email de confirmation en cours via Bird...", 4000);
}

void InscriptionsWidget::onSearchInscGlobalTextChanged(const QString &text)
{
    if (text.trimmed().isEmpty()) {
        searchDebounceTimer->stop();
        executerRechercheInscGlobale();
    } else {
        searchDebounceTimer->start(250);
    }
}

void InscriptionsWidget::executerRechercheInscGlobale()
{
    QString searchStr = ui->txtSearchInscGlobal->text().trimmed();

    QList<QPair<QString, QString>> criteres;
    criteres.append({ui->comboTriInscCrit1->currentText(), ui->comboTriInscOrdre1->currentText()});
    criteres.append({ui->comboTriInscCrit2->currentText(), ui->comboTriInscOrdre2->currentText()});
    criteres.append({ui->comboTriInscCrit3->currentText(), ui->comboTriInscOrdre3->currentText()});

    QList<Inscription> resultats = Inscription::rechercherEtTrier(searchStr, criteres);
    chargerInscriptions(resultats);

    if (!searchStr.isEmpty()) {
        emit statusMessage(QString("🔍 Recherche inscription : %1 résultat(s) trouvé(s)").arg(resultats.size()), 4000);
    }
}

void InscriptionsWidget::on_btnInscAppliquerTri_clicked()
{
    executerRechercheInscGlobale();
    emit statusMessage("↕ Tri multicritère appliqué aux inscriptions", 3000);
}

void InscriptionsWidget::on_btnInscResetTri_clicked()
{
    ui->txtSearchInscGlobal->clear();
    ui->comboTriInscCrit1->setCurrentIndex(0); // Date inscription
    ui->comboTriInscCrit2->setCurrentIndex(0); // Note
    ui->comboTriInscCrit3->setCurrentIndex(0); // Statut

    actualiserOrdreCombo(ui->comboTriInscCrit1, ui->comboTriInscOrdre1);
    actualiserOrdreCombo(ui->comboTriInscCrit2, ui->comboTriInscOrdre2);
    actualiserOrdreCombo(ui->comboTriInscCrit3, ui->comboTriInscOrdre3);

    ui->comboTriInscOrdre1->setCurrentIndex(0);
    ui->comboTriInscOrdre2->setCurrentIndex(0);
    ui->comboTriInscOrdre3->setCurrentIndex(0);

    chargerInscriptions(Inscription::afficher());
    emit statusMessage("🔄 Recherche et tri inscriptions réinitialisés", 3000);
}

void InscriptionsWidget::onComboCrit1Changed(int index)
{
    Q_UNUSED(index);
    actualiserOrdreCombo(ui->comboTriInscCrit1, ui->comboTriInscOrdre1);
}

void InscriptionsWidget::onComboCrit2Changed(int index)
{
    Q_UNUSED(index);
    actualiserOrdreCombo(ui->comboTriInscCrit2, ui->comboTriInscOrdre2);
}

void InscriptionsWidget::onComboCrit3Changed(int index)
{
    Q_UNUSED(index);
    actualiserOrdreCombo(ui->comboTriInscCrit3, ui->comboTriInscOrdre3);
}

void InscriptionsWidget::actualiserOrdreCombo(QComboBox *comboCrit, QComboBox *comboOrdre)
{
    if (!comboCrit || !comboOrdre) return;

    QString critere = comboCrit->currentText().trimmed().toLower();
    int prevIndex = comboOrdre->currentIndex();
    if (prevIndex < 0) prevIndex = 0;

    comboOrdre->blockSignals(true);
    comboOrdre->clear();

    if (critere == "note") {
        comboOrdre->addItem("Décroissant (Note la plus haute → Plus basse)", "DESC");
        comboOrdre->addItem("Croissant (Note la plus basse → Plus haute)", "ASC");
    } else if (critere == "date inscription" || critere == "date_inscription" || critere == "date") {
        comboOrdre->addItem("Décroissant (Plus récente → Plus ancienne)", "DESC");
        comboOrdre->addItem("Croissant (Plus ancienne → Plus récente)", "ASC");
    } else if (critere == "statut") {
        comboOrdre->addItem("Croissant (Validé → En attente → Annulé)", "ASC");
        comboOrdre->addItem("Décroissant (Annulé → En attente → Validé)", "DESC");
    } else {
        comboOrdre->addItem("Croissant (A → Z)", "ASC");
        comboOrdre->addItem("Décroissant (Z → A)", "DESC");
    }

    if (prevIndex < comboOrdre->count()) {
        comboOrdre->setCurrentIndex(prevIndex);
    } else {
        comboOrdre->setCurrentIndex(0);
    }
    comboOrdre->blockSignals(false);
}
