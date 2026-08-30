#include "recommandationwidget.h"
#include "ui_recommandations.h"
#include "stagiaire.h"
#include "cours.h"
#include "inscription.h"

#include <QMessageBox>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QHBoxLayout>
#include <QVBoxLayout>

RecommandationWidget::RecommandationWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::RecommandationWidget)
    , groqService(new GroqRecommendationService(this))
{
    ui->setupUi(this);

    // Initialisation placeholder
    QLabel *lblPlaceholder = new QLabel(
        "💡 Cliquez sur « 🤖 Analyser et Recommander » pour générer les 3 cours les plus adaptés au profil sélectionné."
    );
    lblPlaceholder->setStyleSheet("color: #64748b; font-size: 13px; padding: 40px; border: 1px dashed #334155; border-radius: 8px;");
    lblPlaceholder->setAlignment(Qt::AlignCenter);
    ui->recomResultsContainerLayout->addWidget(lblPlaceholder);
    ui->recomResultsContainerLayout->addStretch();

    // Connexions du service Groq
    connect(groqService, &GroqRecommendationService::recommendationsReady, this, &RecommandationWidget::onRecommendationsReceived);
    connect(groqService, &GroqRecommendationService::recommendationFailed, this, &RecommandationWidget::onRecommendationError);

    // Chargement initial des stagiaires
    chargerStagiaires();
}

RecommandationWidget::~RecommandationWidget()
{
    delete ui;
}

void RecommandationWidget::chargerStagiaires()
{
    ui->comboRecomStagiaire->blockSignals(true);
    ui->comboRecomStagiaire->clear();

    QList<Stagiaire> liste = Stagiaire::afficher();
    for (const Stagiaire &s : liste) {
        QString text = QString("%1 - %2 %3 (%4)")
                           .arg(s.getIdStagiaire())
                           .arg(s.getNom())
                           .arg(s.getPrenom())
                           .arg(s.getFormation());
        ui->comboRecomStagiaire->addItem(text, s.getIdStagiaire());
    }

    ui->comboRecomStagiaire->blockSignals(false);

    if (ui->comboRecomStagiaire->count() > 0) {
        ui->comboRecomStagiaire->setCurrentIndex(0);
        afficherProfilStagiaire(ui->comboRecomStagiaire->currentData().toInt());
    } else {
        ui->lblRecomNomVal->setText("Aucun stagiaire dans Oracle");
    }
}

void RecommandationWidget::afficherProfilStagiaire(int idStagiaire)
{
    if (idStagiaire <= 0) return;

    Stagiaire s = Stagiaire::getById(idStagiaire);
    if (s.getIdStagiaire() <= 0) return;

    ui->lblRecomNomVal->setText(QString("%1 %2").arg(s.getNom()).arg(s.getPrenom()));
    ui->lblRecomFiliereVal->setText(s.getFormation().isEmpty() ? "Non renseignée" : s.getFormation());
    ui->lblRecomNiveauVal->setText(s.getNiveau().isEmpty() ? "Standard" : s.getNiveau());

    QList<Inscription> inscriptions = Inscription::readByStagiaire(idStagiaire);
    QStringList coursNoms;
    for (const Inscription &ins : inscriptions) {
        if (!ins.getNomCours().isEmpty() && !coursNoms.contains(ins.getNomCours())) {
            coursNoms.append(ins.getNomCours());
        }
    }

    if (coursNoms.isEmpty()) {
        ui->lblRecomCoursSuivisVal->setText("Aucun cours enregistré");
        ui->lblRecomCoursSuivisVal->setStyleSheet("color: #64748b; font-style: italic;");
    } else {
        ui->lblRecomCoursSuivisVal->setText(coursNoms.join(", "));
        ui->lblRecomCoursSuivisVal->setStyleSheet("color: #38bdf8; font-weight: 500;");
    }
}

void RecommandationWidget::on_comboRecomStagiaire_currentIndexChanged(int index)
{
    if (index < 0) return;
    int idStagiaire = ui->comboRecomStagiaire->itemData(index).toInt();
    afficherProfilStagiaire(idStagiaire);
}

void RecommandationWidget::on_btnLancerRecommandation_clicked()
{
    if (ui->comboRecomStagiaire->count() == 0) {
        QMessageBox::warning(this, "Avertissement", "Veuillez d'abord sélectionner un stagiaire.");
        return;
    }

    int idStagiaire = ui->comboRecomStagiaire->currentData().toInt();
    if (idStagiaire <= 0) {
        QMessageBox::warning(this, "Avertissement", "Identifiant du stagiaire invalide.");
        return;
    }

    ui->btnLancerRecommandation->setEnabled(false);
    ui->btnLancerRecommandation->setText("⏳ Analyse Groq en cours...");
    ui->lblRecomStatusInfo->setText("📡 Récupération du catalogue Oracle et interrogation de GROQ API...");
    ui->lblRecomStatusInfo->setStyleSheet("color: #38bdf8; font-size: 11px; font-weight: bold;");

    emit statusMessage("🤖 Analyse prédictive Groq IA en cours...", 4000);

    // Appel asynchrone non-bloquant vers Groq
    groqService->requestRecommendations(idStagiaire);
}

void RecommandationWidget::onRecommendationsReceived(const QList<GroqRecommendationResult> &recommendations, const QString &sourceInfo)
{
    ui->btnLancerRecommandation->setEnabled(true);
    ui->btnLancerRecommandation->setText("🤖 Analyser et Recommander");

    ui->lblRecomStatusInfo->setText("✅ Recommandations Groq actualisées.");
    ui->lblRecomStatusInfo->setStyleSheet("color: #34d399; font-size: 11px; font-weight: bold;");
    emit statusMessage("✓ Recommandations IA générées avec succès", 4000);

    // Nettoyage de la zone d'affichage
    QLayoutItem *child;
    while ((child = ui->recomResultsContainerLayout->takeAt(0)) != nullptr) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }

    // Badge Source
    QLabel *lblSource = new QLabel(QString("ℹ️ %1").arg(sourceInfo));
    lblSource->setStyleSheet("color: #64748b; font-size: 11px; font-style: italic; padding: 4px 8px; background-color: #0f172a; border-radius: 4px;");
    ui->recomResultsContainerLayout->addWidget(lblSource);

    if (recommendations.isEmpty()) {
        QLabel *lblEmpty = new QLabel("⚠️ Aucune formation adaptée n'a pu être identifiée pour ce profil dans le catalogue Oracle actuel.");
        lblEmpty->setStyleSheet("color: #fbbf24; font-size: 13px; padding: 25px; background-color: #0f172a; border-radius: 8px;");
        lblEmpty->setAlignment(Qt::AlignCenter);
        ui->recomResultsContainerLayout->addWidget(lblEmpty);
        ui->recomResultsContainerLayout->addStretch();
        return;
    }

    const QStringList ranks = {"🥇 1ÈRE RECOMMANDATION (TOP MATCH)", "🥈 2ÈME RECOMMANDATION", "🥉 3ÈME RECOMMANDATION"};
    const QStringList badgeColors = {"#059669", "#0284c7", "#d97706"};
    const QStringList borderColors = {"#34d399", "#38bdf8", "#fbbf24"};

    for (int i = 0; i < recommendations.size(); ++i) {
        const GroqRecommendationResult &rec = recommendations[i];

        QFrame *card = new QFrame();
        QString bColor = (i < borderColors.size()) ? borderColors[i] : "#475569";
        card->setStyleSheet(QString("QFrame { background-color: #0f172a; border: 1px solid %1; border-radius: 8px; }").arg(bColor));
        QVBoxLayout *cardLay = new QVBoxLayout(card);
        cardLay->setContentsMargins(15, 12, 15, 14);
        cardLay->setSpacing(8);

        // Ligne 1 : Titre Rang & Badge Score de pertinence
        QHBoxLayout *topRow = new QHBoxLayout();
        QString rankText = (i < ranks.size()) ? ranks[i] : QString("FORMATION RECOMMANDÉE #%1").arg(i + 1);
        QLabel *lblRank = new QLabel(rankText);
        lblRank->setStyleSheet("color: #94a3b8; font-size: 11px; font-weight: bold; letter-spacing: 1px;");

        QString badgeCol = (i < badgeColors.size()) ? badgeColors[i] : "#0284c7";
        QLabel *lblScore = new QLabel(QString("Pertinence : %1 %").arg(rec.score));
        lblScore->setStyleSheet(QString("background-color: %1; color: #ffffff; font-size: 11px; font-weight: bold; padding: 3px 8px; border-radius: 4px;").arg(badgeCol));

        topRow->addWidget(lblRank);
        topRow->addStretch();
        topRow->addWidget(lblScore);
        cardLay->addLayout(topRow);

        // Ligne 2 : Nom du cours
        QLabel *lblNom = new QLabel(rec.nomCours);
        lblNom->setStyleSheet("color: #f8fafc; font-size: 16px; font-weight: bold;");
        cardLay->addWidget(lblNom);

        // Ligne 3 : Métadonnées (Durée, Capacité, Statut)
        QHBoxLayout *metaRow = new QHBoxLayout();
        QLabel *lblMeta = new QLabel(QString("⏱️ Volume : %1 heures   |   👥 Capacité : %2 places   |   🏷️ Statut : %3")
                                         .arg(rec.duree)
                                         .arg(rec.capacite)
                                         .arg(rec.statut));
        lblMeta->setStyleSheet("color: #38bdf8; font-size: 12px; font-weight: 500;");
        metaRow->addWidget(lblMeta);
        metaRow->addStretch();
        cardLay->addLayout(metaRow);

        // Ligne 4 : Justification pédagogique de l'IA
        QLabel *lblRaison = new QLabel(QString("💬 %1").arg(rec.raison));
        lblRaison->setStyleSheet("color: #cbd5e1; font-size: 12px; line-height: 1.4; padding: 6px 8px; background-color: #1e293b; border-radius: 6px;");
        lblRaison->setWordWrap(true);
        cardLay->addWidget(lblRaison);

        // Ligne 5 : Bouton Inscription Directe
        QHBoxLayout *actRow = new QHBoxLayout();
        actRow->addStretch();
        QPushButton *btnInscrire = new QPushButton("📝 Inscrire ce stagiaire à cette formation");
        btnInscrire->setCursor(Qt::PointingHandCursor);
        btnInscrire->setStyleSheet(
            "QPushButton { background-color: #0284c7; color: #ffffff; border: none; border-radius: 5px; padding: 6px 14px; font-size: 12px; font-weight: bold; }"
            "QPushButton:hover { background-color: #0369a1; }"
        );

        int targetIdCours = rec.idCours;
        int targetIdStagiaire = ui->comboRecomStagiaire->currentData().toInt();

        connect(btnInscrire, &QPushButton::clicked, this, [this, targetIdStagiaire, targetIdCours]() {
            emit inscriptionDemandee(targetIdStagiaire, targetIdCours);
        });

        actRow->addWidget(btnInscrire);
        cardLay->addLayout(actRow);

        ui->recomResultsContainerLayout->addWidget(card);
    }

    ui->recomResultsContainerLayout->addStretch();
}

void RecommandationWidget::onRecommendationError(const QString &errorMessage)
{
    ui->btnLancerRecommandation->setEnabled(true);
    ui->btnLancerRecommandation->setText("🤖 Analyser et Recommander");

    ui->lblRecomStatusInfo->setText(QString("❌ %1").arg(errorMessage));
    ui->lblRecomStatusInfo->setStyleSheet("color: #f87171; font-size: 11px; font-weight: bold;");

    emit statusMessage(QString("❌ Échec de la recommandation IA : %1").arg(errorMessage), 6000);
}
