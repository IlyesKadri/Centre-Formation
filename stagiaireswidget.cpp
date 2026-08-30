#include "stagiaireswidget.h"
#include "ui_stagiaires.h"

#include <QMessageBox>
#include <QDate>
#include <QHeaderView>
#include <QDebug>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

StagiairesWidget::StagiairesWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::StagiairesWidget)
    , stagiairesModel(new QStandardItemModel(this))
    , searchDebounceTimer(new QTimer(this))
{
    ui->setupUi(this);

    // Initialisation du formulaire
    ui->dateStagiaireNaissance->setDate(QDate::currentDate().addYears(-20));

    // Contrôle de saisie du numéro de téléphone : uniquement des chiffres et exactement 8 chiffres max
    ui->txtStagiaireTelephone->setMaxLength(8);
    QRegularExpression rxTel("^[0-9]{0,8}$");
    ui->txtStagiaireTelephone->setValidator(new QRegularExpressionValidator(rxTel, this));
    ui->txtStagiaireTelephone->setPlaceholderText("8 chiffres (ex: 98123456)");

    // Masquer les champs ID (gérés automatiquement par Oracle)
    ui->lblStagId->hide();
    ui->txtStagiaireId->hide();

    // Configuration de la table
    setupTable();

    // Initialisation dynamique des options d'ordre de tri selon le critère
    connect(ui->comboTriStagiaireCrit1, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &StagiairesWidget::onComboCrit1Changed);
    connect(ui->comboTriStagiaireCrit2, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &StagiairesWidget::onComboCrit2Changed);
    connect(ui->comboTriStagiaireCrit3, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &StagiairesWidget::onComboCrit3Changed);

    actualiserOrdreCombo(ui->comboTriStagiaireCrit1, ui->comboTriStagiaireOrdre1);
    actualiserOrdreCombo(ui->comboTriStagiaireCrit2, ui->comboTriStagiaireOrdre2);
    actualiserOrdreCombo(ui->comboTriStagiaireCrit3, ui->comboTriStagiaireOrdre3);

    // Debounce timer pour la recherche globale (250 ms)
    searchDebounceTimer->setSingleShot(true);
    connect(searchDebounceTimer, &QTimer::timeout, this, &StagiairesWidget::executerRechercheGlobale);
    connect(ui->txtSearchStagiaireGlobal, &QLineEdit::textChanged, this, &StagiairesWidget::onSearchGlobalTextChanged);

    // Charger les données initiales depuis Oracle XE
    rafraichirDonnees();
}

StagiairesWidget::~StagiairesWidget()
{
    delete ui;
}

void StagiairesWidget::setupTable()
{
    stagiairesModel->setHorizontalHeaderLabels({
        "ID", "Nom", "Prénom", "Email", "Téléphone",
        "Date Naissance", "Niveau", "Formation"
    });

    ui->tableStagiaires->setModel(stagiairesModel);
    ui->tableStagiaires->horizontalHeader()->setStretchLastSection(true);
    ui->tableStagiaires->setColumnHidden(0, true); // Masquer ID
    ui->tableStagiaires->setColumnWidth(1, 140);   // Nom
    ui->tableStagiaires->setColumnWidth(2, 140);   // Prenom
    ui->tableStagiaires->setColumnWidth(3, 190);   // Email
    ui->tableStagiaires->setColumnWidth(4, 110);   // Tel
    ui->tableStagiaires->setColumnWidth(5, 110);   // Date
    ui->tableStagiaires->setColumnWidth(6, 120);   // Niveau
    ui->tableStagiaires->setColumnWidth(7, 150);   // Formation
}

void StagiairesWidget::rafraichirDonnees()
{
    chargerStagiaires(Stagiaire::afficher());
}

void StagiairesWidget::chargerStagiaires(const QList<Stagiaire> &liste)
{
    stagiairesModel->removeRows(0, stagiairesModel->rowCount());

    for (const Stagiaire &s : liste) {
        QList<QStandardItem*> row;
        row.append(new QStandardItem(QString::number(s.getIdStagiaire())));
        row.append(new QStandardItem(s.getNom()));
        row.append(new QStandardItem(s.getPrenom()));
        row.append(new QStandardItem(s.getEmail()));
        row.append(new QStandardItem(s.getTelephone()));
        row.append(new QStandardItem(s.getDateNaissance().toString("dd/MM/yyyy")));
        row.append(new QStandardItem(s.getNiveau()));
        row.append(new QStandardItem(s.getFormation()));

        stagiairesModel->appendRow(row);
    }
}

void StagiairesWidget::viderFormulaire()
{
    ui->txtStagiaireId->clear();
    ui->txtStagiaireNom->clear();
    ui->txtStagiairePrenom->clear();
    ui->txtStagiaireEmail->clear();
    ui->txtStagiaireTelephone->clear();
    ui->dateStagiaireNaissance->setDate(QDate::currentDate().addYears(-20));
    ui->comboStagiaireNiveau->setCurrentIndex(0);
    ui->txtStagiaireFormation->clear();

    ui->tableStagiaires->clearSelection();
}

void StagiairesWidget::remplirFormulaire(const Stagiaire &s)
{
    ui->txtStagiaireId->setText(QString::number(s.getIdStagiaire()));
    ui->txtStagiaireNom->setText(s.getNom());
    ui->txtStagiairePrenom->setText(s.getPrenom());
    ui->txtStagiaireEmail->setText(s.getEmail());
    ui->txtStagiaireTelephone->setText(s.getTelephone());
    if (s.getDateNaissance().isValid()) {
        ui->dateStagiaireNaissance->setDate(s.getDateNaissance());
    }
    ui->comboStagiaireNiveau->setCurrentText(s.getNiveau());
    ui->txtStagiaireFormation->setText(s.getFormation());
}

void StagiairesWidget::on_tableStagiaires_clicked(const QModelIndex &index)
{
    int row = index.row();
    if (row < 0 || row >= stagiairesModel->rowCount()) return;

    ui->txtStagiaireId->setText(stagiairesModel->item(row, 0)->text());
    ui->txtStagiaireNom->setText(stagiairesModel->item(row, 1)->text());
    ui->txtStagiairePrenom->setText(stagiairesModel->item(row, 2)->text());
    ui->txtStagiaireEmail->setText(stagiairesModel->item(row, 3)->text());
    ui->txtStagiaireTelephone->setText(stagiairesModel->item(row, 4)->text());

    QString dateStr = stagiairesModel->item(row, 5)->text();
    QDate d = QDate::fromString(dateStr, "dd/MM/yyyy");
    if (d.isValid()) {
        ui->dateStagiaireNaissance->setDate(d);
    }

    ui->comboStagiaireNiveau->setCurrentText(stagiairesModel->item(row, 6)->text());
    ui->txtStagiaireFormation->setText(stagiairesModel->item(row, 7)->text());
}

void StagiairesWidget::on_btnStagiaireAjouter_clicked()
{
    Stagiaire s(
        0,
        ui->txtStagiaireNom->text().trimmed(),
        ui->txtStagiairePrenom->text().trimmed(),
        ui->txtStagiaireEmail->text().trimmed(),
        ui->txtStagiaireTelephone->text().trimmed(),
        ui->dateStagiaireNaissance->date(),
        ui->comboStagiaireNiveau->currentText(),
        ui->txtStagiaireFormation->text().trimmed()
    );

    QString erreur;
    if (!s.estValide(erreur)) {
        QMessageBox::warning(this, "Validation impossible", erreur);
        return;
    }

    if (s.ajouter()) {
        QMessageBox::information(this, "Succès", "Stagiaire ajouté avec succès dans Oracle XE.");
        rafraichirDonnees();
        viderFormulaire();
        emit stagiairesModifies();
        emit statusMessage("Stagiaire ajouté avec succès", 3000);
    } else {
        QMessageBox::critical(this, "Erreur Oracle", "Échec de l'ajout du stagiaire. Vérifiez si l'email n'existe pas déjà.");
    }
}

void StagiairesWidget::on_btnStagiaireModifier_clicked()
{
    int id = ui->txtStagiaireId->text().toInt();
    if (id <= 0) {
        QMessageBox::warning(this, "Avertissement", "Veuillez sélectionner un stagiaire dans le tableau à modifier.");
        return;
    }

    Stagiaire s(
        id,
        ui->txtStagiaireNom->text().trimmed(),
        ui->txtStagiairePrenom->text().trimmed(),
        ui->txtStagiaireEmail->text().trimmed(),
        ui->txtStagiaireTelephone->text().trimmed(),
        ui->dateStagiaireNaissance->date(),
        ui->comboStagiaireNiveau->currentText(),
        ui->txtStagiaireFormation->text().trimmed()
    );

    QString erreur;
    if (!s.estValide(erreur)) {
        QMessageBox::warning(this, "Validation impossible", erreur);
        return;
    }

    if (s.modifier()) {
        QMessageBox::information(this, "Succès", "Stagiaire mis à jour avec succès.");
        rafraichirDonnees();
        viderFormulaire();
        emit stagiairesModifies();
        emit statusMessage("Stagiaire mis à jour avec succès", 3000);
    } else {
        QMessageBox::critical(this, "Erreur Oracle", "Échec de la modification du stagiaire.");
    }
}

void StagiairesWidget::on_btnStagiaireSupprimer_clicked()
{
    int id = ui->txtStagiaireId->text().toInt();
    if (id <= 0) {
        QMessageBox::warning(this, "Avertissement", "Veuillez sélectionner un stagiaire dans le tableau à supprimer.");
        return;
    }

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(
        this,
        "Confirmation de suppression",
        QString("Êtes-vous sûr de vouloir supprimer le stagiaire ID %1 (%2 %3) ?")
            .arg(id)
            .arg(ui->txtStagiaireNom->text())
            .arg(ui->txtStagiairePrenom->text()),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        Stagiaire s;
        s.setIdStagiaire(id);

        if (s.supprimer()) {
            QMessageBox::information(this, "Succès", "Stagiaire supprimé avec succès.");
            rafraichirDonnees();
            viderFormulaire();
            emit stagiairesModifies();
            emit statusMessage("Stagiaire supprimé avec succès", 3000);
        } else {
            QMessageBox::critical(this, "Erreur Oracle", "Échec de la suppression. Le stagiaire possède peut-être des inscriptions associées.");
        }
    }
}

void StagiairesWidget::on_btnStagiaireVider_clicked()
{
    viderFormulaire();
}

void StagiairesWidget::onSearchGlobalTextChanged(const QString &text)
{
    if (text.trimmed().isEmpty()) {
        searchDebounceTimer->stop();
        executerRechercheGlobale();
    } else {
        searchDebounceTimer->start(250);
    }
}

void StagiairesWidget::executerRechercheGlobale()
{
    QString searchStr = ui->txtSearchStagiaireGlobal->text().trimmed();

    QList<QPair<QString, QString>> criteres;
    criteres.append({ui->comboTriStagiaireCrit1->currentText(), ui->comboTriStagiaireOrdre1->currentText()});
    criteres.append({ui->comboTriStagiaireCrit2->currentText(), ui->comboTriStagiaireOrdre2->currentText()});
    criteres.append({ui->comboTriStagiaireCrit3->currentText(), ui->comboTriStagiaireOrdre3->currentText()});

    QList<Stagiaire> resultats = Stagiaire::rechercherEtTrier(searchStr, criteres);
    chargerStagiaires(resultats);

    if (!searchStr.isEmpty()) {
        emit statusMessage(QString("🔍 Recherche stagiaires : %1 résultat(s) trouvé(s)").arg(resultats.size()), 4000);
    }
}

void StagiairesWidget::on_btnStagiaireAppliquerTri_clicked()
{
    executerRechercheGlobale();
    emit statusMessage("↕ Tri multicritère appliqué aux stagiaires", 3000);
}

void StagiairesWidget::on_btnStagiaireResetTri_clicked()
{
    ui->txtSearchStagiaireGlobal->clear();
    ui->comboTriStagiaireCrit1->setCurrentIndex(0); // Nom
    ui->comboTriStagiaireCrit2->setCurrentIndex(0); // Date de naissance
    ui->comboTriStagiaireCrit3->setCurrentIndex(0); // Niveau

    actualiserOrdreCombo(ui->comboTriStagiaireCrit1, ui->comboTriStagiaireOrdre1);
    actualiserOrdreCombo(ui->comboTriStagiaireCrit2, ui->comboTriStagiaireOrdre2);
    actualiserOrdreCombo(ui->comboTriStagiaireCrit3, ui->comboTriStagiaireOrdre3);

    ui->comboTriStagiaireOrdre1->setCurrentIndex(0);
    ui->comboTriStagiaireOrdre2->setCurrentIndex(0);
    ui->comboTriStagiaireOrdre3->setCurrentIndex(0);

    chargerStagiaires(Stagiaire::afficher());
    emit statusMessage("🔄 Recherche et tri stagiaires réinitialisés", 3000);
}

void StagiairesWidget::onComboCrit1Changed(int index)
{
    Q_UNUSED(index);
    actualiserOrdreCombo(ui->comboTriStagiaireCrit1, ui->comboTriStagiaireOrdre1);
}

void StagiairesWidget::onComboCrit2Changed(int index)
{
    Q_UNUSED(index);
    actualiserOrdreCombo(ui->comboTriStagiaireCrit2, ui->comboTriStagiaireOrdre2);
}

void StagiairesWidget::onComboCrit3Changed(int index)
{
    Q_UNUSED(index);
    actualiserOrdreCombo(ui->comboTriStagiaireCrit3, ui->comboTriStagiaireOrdre3);
}

void StagiairesWidget::actualiserOrdreCombo(QComboBox *comboCrit, QComboBox *comboOrdre)
{
    if (!comboCrit || !comboOrdre) return;

    QString critere = comboCrit->currentText().trimmed().toLower();
    int prevIndex = comboOrdre->currentIndex();
    if (prevIndex < 0) prevIndex = 0;

    comboOrdre->blockSignals(true);
    comboOrdre->clear();

    if (critere == "niveau") {
        comboOrdre->addItem("Croissant (Débutant → Intermédiaire → Avancé)", "ASC");
        comboOrdre->addItem("Décroissant (Avancé → Intermédiaire → Débutant)", "DESC");
    } else if (critere == "date de naissance" || critere == "date_naissance" || critere == "date naissance") {
        comboOrdre->addItem("Croissant (Plus ancien → Plus récent)", "ASC");
        comboOrdre->addItem("Décroissant (Plus récent → Plus ancien)", "DESC");
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
