#include "courswidget.h"
#include "ui_cours.h"

#include <QMessageBox>
#include <QDate>
#include <QHeaderView>
#include <QDebug>

CoursWidget::CoursWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CoursWidget)
    , coursModel(new QStandardItemModel(this))
    , searchDebounceTimer(new QTimer(this))
{
    ui->setupUi(this);

    // Initialisation du formulaire
    ui->dateCoursDebut->setDate(QDate::currentDate());
    ui->dateCoursFin->setDate(QDate::currentDate().addDays(30));
    ui->spinCoursDuree->setValue(30);
    ui->spinCoursCapacite->setValue(20);
    ui->doubleSpinCoursPrix->setValue(500.0);

    // Masquer les champs ID (gérés automatiquement par Oracle)
    ui->lblCoursId->hide();
    ui->txtCoursId->hide();

    // Configuration de la table
    setupTable();

    // Initialisation dynamique des options d'ordre de tri selon le critère
    connect(ui->comboTriCoursCrit1, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CoursWidget::onComboCrit1Changed);
    connect(ui->comboTriCoursCrit2, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CoursWidget::onComboCrit2Changed);
    connect(ui->comboTriCoursCrit3, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CoursWidget::onComboCrit3Changed);

    actualiserOrdreCombo(ui->comboTriCoursCrit1, ui->comboTriCoursOrdre1);
    actualiserOrdreCombo(ui->comboTriCoursCrit2, ui->comboTriCoursOrdre2);
    actualiserOrdreCombo(ui->comboTriCoursCrit3, ui->comboTriCoursOrdre3);

    // Debounce timer pour la recherche globale fluide (250 ms)
    searchDebounceTimer->setSingleShot(true);
    connect(searchDebounceTimer, &QTimer::timeout, this, &CoursWidget::executerRechercheGlobale);
    connect(ui->txtSearchCoursGlobal, &QLineEdit::textChanged, this, &CoursWidget::onSearchGlobalTextChanged);

    // Charger les données initiales depuis Oracle XE
    rafraichirDonnees();
}

CoursWidget::~CoursWidget()
{
    delete ui;
}

void CoursWidget::setupTable()
{
    coursModel->setHorizontalHeaderLabels({
        "ID", "Nom", "Description", "Date Début", "Date Fin",
        "Durée (h)", "Capacité", "Inscrits", "Prix (€)", "Statut"
    });

    ui->tableCours->setModel(coursModel);
    ui->tableCours->horizontalHeader()->setStretchLastSection(true);
    ui->tableCours->setColumnHidden(0, true); // Masquer la colonne ID
    ui->tableCours->setColumnWidth(1, 150);   // Nom
    ui->tableCours->setColumnWidth(2, 180);   // Description
    ui->tableCours->setColumnWidth(3, 100);   // Debut
    ui->tableCours->setColumnWidth(4, 100);   // Fin
    ui->tableCours->setColumnWidth(5, 80);    // Duree
    ui->tableCours->setColumnWidth(6, 80);    // Capacite
    ui->tableCours->setColumnWidth(7, 80);    // Inscrits
    ui->tableCours->setColumnWidth(8, 90);    // Prix
    ui->tableCours->setColumnWidth(9, 100);   // Statut
}

void CoursWidget::rafraichirDonnees()
{
    chargerCours(Cours::afficher());
}

void CoursWidget::chargerCours(const QList<Cours> &liste)
{
    coursModel->removeRows(0, coursModel->rowCount());

    for (const Cours &c : liste) {
        int inscrits = Cours::compterInscriptionsActuelles(c.getIdCours());

        QList<QStandardItem*> row;
        row.append(new QStandardItem(QString::number(c.getIdCours())));
        row.append(new QStandardItem(c.getNom()));
        row.append(new QStandardItem(c.getDescription()));
        row.append(new QStandardItem(c.getDateDebut().toString("dd/MM/yyyy")));
        row.append(new QStandardItem(c.getDateFin().toString("dd/MM/yyyy")));
        row.append(new QStandardItem(QString::number(c.getDuree())));
        row.append(new QStandardItem(QString::number(c.getCapacite())));
        row.append(new QStandardItem(QString::number(inscrits)));
        row.append(new QStandardItem(QString::number(c.getPrix(), 'f', 2)));
        row.append(new QStandardItem(c.getStatut()));

        coursModel->appendRow(row);
    }
}

void CoursWidget::viderFormulaire()
{
    ui->txtCoursId->clear();
    ui->txtCoursNom->clear();
    ui->txtCoursDescription->clear();
    ui->dateCoursDebut->setDate(QDate::currentDate());
    ui->dateCoursFin->setDate(QDate::currentDate().addDays(30));
    ui->spinCoursDuree->setValue(30);
    ui->spinCoursCapacite->setValue(20);
    ui->doubleSpinCoursPrix->setValue(500.0);
    ui->comboCoursStatut->setCurrentIndex(0);

    ui->tableCours->clearSelection();
}

void CoursWidget::remplirFormulaire(const Cours &c)
{
    ui->txtCoursId->setText(QString::number(c.getIdCours()));
    ui->txtCoursNom->setText(c.getNom());
    ui->txtCoursDescription->setText(c.getDescription());
    if (c.getDateDebut().isValid()) ui->dateCoursDebut->setDate(c.getDateDebut());
    if (c.getDateFin().isValid()) ui->dateCoursFin->setDate(c.getDateFin());
    ui->spinCoursDuree->setValue(c.getDuree());
    ui->spinCoursCapacite->setValue(c.getCapacite());
    ui->doubleSpinCoursPrix->setValue(c.getPrix());
    ui->comboCoursStatut->setCurrentText(c.getStatut());
}

void CoursWidget::on_tableCours_clicked(const QModelIndex &index)
{
    int row = index.row();
    if (row < 0 || row >= coursModel->rowCount()) return;

    ui->txtCoursId->setText(coursModel->item(row, 0)->text());
    ui->txtCoursNom->setText(coursModel->item(row, 1)->text());
    ui->txtCoursDescription->setText(coursModel->item(row, 2)->text());

    QDate dDeb = QDate::fromString(coursModel->item(row, 3)->text(), "dd/MM/yyyy");
    if (dDeb.isValid()) ui->dateCoursDebut->setDate(dDeb);

    QDate dFin = QDate::fromString(coursModel->item(row, 4)->text(), "dd/MM/yyyy");
    if (dFin.isValid()) ui->dateCoursFin->setDate(dFin);

    ui->spinCoursDuree->setValue(coursModel->item(row, 5)->text().toInt());
    ui->spinCoursCapacite->setValue(coursModel->item(row, 6)->text().toInt());
    ui->doubleSpinCoursPrix->setValue(coursModel->item(row, 8)->text().toDouble());
    ui->comboCoursStatut->setCurrentText(coursModel->item(row, 9)->text());
}

void CoursWidget::on_btnCoursAjouter_clicked()
{
    Cours c(
        0,
        ui->txtCoursNom->text().trimmed(),
        ui->txtCoursDescription->text().trimmed(),
        ui->dateCoursDebut->date(),
        ui->dateCoursFin->date(),
        ui->spinCoursDuree->value(),
        ui->spinCoursCapacite->value(),
        ui->doubleSpinCoursPrix->value(),
        ui->comboCoursStatut->currentText()
    );

    QString erreur;
    if (!c.estValide(erreur)) {
        QMessageBox::warning(this, "Validation impossible", erreur);
        return;
    }

    if (c.ajouter()) {
        QMessageBox::information(this, "Succès", "Cours créé avec succès dans la base Oracle.");
        rafraichirDonnees();
        viderFormulaire();
        emit coursModifies();
        emit statusMessage("Cours créé avec succès", 3000);
    } else {
        QMessageBox::critical(this, "Erreur Oracle", "Échec de l'ajout du cours.");
    }
}

void CoursWidget::on_btnCoursModifier_clicked()
{
    int id = ui->txtCoursId->text().toInt();
    if (id <= 0) {
        QMessageBox::warning(this, "Avertissement", "Veuillez sélectionner un cours dans la liste à modifier.");
        return;
    }

    Cours c(
        id,
        ui->txtCoursNom->text().trimmed(),
        ui->txtCoursDescription->text().trimmed(),
        ui->dateCoursDebut->date(),
        ui->dateCoursFin->date(),
        ui->spinCoursDuree->value(),
        ui->spinCoursCapacite->value(),
        ui->doubleSpinCoursPrix->value(),
        ui->comboCoursStatut->currentText()
    );

    QString erreur;
    if (!c.estValide(erreur)) {
        QMessageBox::warning(this, "Validation impossible", erreur);
        return;
    }

    if (c.modifier()) {
        QMessageBox::information(this, "Succès", "Cours mis à jour avec succès.");
        rafraichirDonnees();
        viderFormulaire();
        emit coursModifies();
        emit statusMessage("Cours mis à jour avec succès", 3000);
    } else {
        QMessageBox::critical(this, "Erreur Oracle", "Échec de la modification du cours.");
    }
}

void CoursWidget::on_btnCoursSupprimer_clicked()
{
    int id = ui->txtCoursId->text().toInt();
    if (id <= 0) {
        QMessageBox::warning(this, "Avertissement", "Veuillez sélectionner un cours à supprimer.");
        return;
    }

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(
        this,
        "Confirmation de suppression",
        QString("Êtes-vous sûr de vouloir supprimer le cours ID %1 (%2) ?")
            .arg(id)
            .arg(ui->txtCoursNom->text()),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        Cours c;
        c.setIdCours(id);

        if (c.supprimer()) {
            QMessageBox::information(this, "Succès", "Cours supprimé avec succès.");
            rafraichirDonnees();
            viderFormulaire();
            emit coursModifies();
            emit statusMessage("Cours supprimé avec succès", 3000);
        } else {
            QMessageBox::critical(this, "Erreur Oracle", "Échec de la suppression. Des inscriptions existent peut-être pour ce cours.");
        }
    }
}

void CoursWidget::on_btnCoursVider_clicked()
{
    viderFormulaire();
}

void CoursWidget::onSearchGlobalTextChanged(const QString &text)
{
    if (text.trimmed().isEmpty()) {
        searchDebounceTimer->stop();
        executerRechercheGlobale();
    } else {
        searchDebounceTimer->start(250);
    }
}

void CoursWidget::executerRechercheGlobale()
{
    QString searchStr = ui->txtSearchCoursGlobal->text().trimmed();

    QList<QPair<QString, QString>> criteres;
    criteres.append(QPair<QString, QString>(ui->comboTriCoursCrit1->currentText(), ui->comboTriCoursOrdre1->currentText()));
    criteres.append(QPair<QString, QString>(ui->comboTriCoursCrit2->currentText(), ui->comboTriCoursOrdre2->currentText()));
    criteres.append(QPair<QString, QString>(ui->comboTriCoursCrit3->currentText(), ui->comboTriCoursOrdre3->currentText()));

    QList<Cours> resultats = Cours::rechercherEtTrier(searchStr, criteres);
    chargerCours(resultats);

    if (!searchStr.isEmpty()) {
        emit statusMessage(QString("🔍 Recherche cours : %1 résultat(s) trouvé(s)").arg(resultats.size()), 4000);
    }
}

void CoursWidget::on_btnCoursAppliquerTri_clicked()
{
    executerRechercheGlobale();
    emit statusMessage("↕ Tri multicritère appliqué aux cours", 3000);
}

void CoursWidget::on_btnCoursResetTri_clicked()
{
    ui->txtSearchCoursGlobal->clear();
    ui->comboTriCoursCrit1->setCurrentIndex(0); // Date début
    ui->comboTriCoursCrit2->setCurrentIndex(0); // Prix
    ui->comboTriCoursCrit3->setCurrentIndex(0); // Capacité

    actualiserOrdreCombo(ui->comboTriCoursCrit1, ui->comboTriCoursOrdre1);
    actualiserOrdreCombo(ui->comboTriCoursCrit2, ui->comboTriCoursOrdre2);
    actualiserOrdreCombo(ui->comboTriCoursCrit3, ui->comboTriCoursOrdre3);

    ui->comboTriCoursOrdre1->setCurrentIndex(0);
    ui->comboTriCoursOrdre2->setCurrentIndex(0);
    ui->comboTriCoursOrdre3->setCurrentIndex(0);

    chargerCours(Cours::afficher());
    emit statusMessage("🔄 Recherche et tri cours réinitialisés", 3000);
}

void CoursWidget::onComboCrit1Changed(int index)
{
    Q_UNUSED(index);
    actualiserOrdreCombo(ui->comboTriCoursCrit1, ui->comboTriCoursOrdre1);
}

void CoursWidget::onComboCrit2Changed(int index)
{
    Q_UNUSED(index);
    actualiserOrdreCombo(ui->comboTriCoursCrit2, ui->comboTriCoursOrdre2);
}

void CoursWidget::onComboCrit3Changed(int index)
{
    Q_UNUSED(index);
    actualiserOrdreCombo(ui->comboTriCoursCrit3, ui->comboTriCoursOrdre3);
}

void CoursWidget::actualiserOrdreCombo(QComboBox *comboCrit, QComboBox *comboOrdre)
{
    if (!comboCrit || !comboOrdre) return;

    QString critere = comboCrit->currentText().trimmed().toLower();
    int prevIndex = comboOrdre->currentIndex();
    if (prevIndex < 0) prevIndex = 0;

    comboOrdre->blockSignals(true);
    comboOrdre->clear();

    if (critere == "prix") {
        comboOrdre->addItem("Croissant (Moins cher → Plus cher)", "ASC");
        comboOrdre->addItem("Décroissant (Plus cher → Moins cher)", "DESC");
    } else if (critere == "durée" || critere == "duree") {
        comboOrdre->addItem("Croissant (Plus court → Plus long)", "ASC");
        comboOrdre->addItem("Décroissant (Plus long → Plus court)", "DESC");
    } else if (critere == "capacité" || critere == "capacite") {
        comboOrdre->addItem("Croissant (Petite → Grande capacité)", "ASC");
        comboOrdre->addItem("Décroissant (Grande → Petite capacité)", "DESC");
    } else if (critere == "date début" || critere == "date debut" || critere == "date_debut" ||
               critere == "date fin" || critere == "date_fin") {
        comboOrdre->addItem("Croissant (Plus ancien → Plus récent)", "ASC");
        comboOrdre->addItem("Décroissant (Plus récent → Plus ancien)", "DESC");
    } else if (critere == "statut") {
        comboOrdre->addItem("Croissant (Planifié → En cours → Terminé → Annulé)", "ASC");
        comboOrdre->addItem("Décroissant (Annulé → Terminé → En cours → Planifié)", "DESC");
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
