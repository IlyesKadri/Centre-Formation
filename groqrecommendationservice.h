#ifndef GROQRECOMMENDATIONSERVICE_H
#define GROQRECOMMENDATIONSERVICE_H

#include <QObject>
#include <QString>
#include <QList>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "stagiaire.h"
#include "cours.h"
#include "inscription.h"

struct GroqRecommendationResult {
    int idCours;
    QString nomCours;
    int score; // 0 à 100 %
    QString raison;
    int duree;
    int capacite;
    QString statut;
};

class GroqRecommendationService : public QObject
{
    Q_OBJECT

public:
    explicit GroqRecommendationService(QObject *parent = nullptr);
    virtual ~GroqRecommendationService();

    // Lance l'analyse intelligente et la recommandation de formations via Groq API
    void requestRecommendations(int idStagiaire);

    // Récupère la clé API Groq de manière sécurisée (sans hardcodage)
    static QString getApiKey();
    static QString getModelName();
    static QString getApiEndpoint();

signals:
    void recommendationsReady(const QList<GroqRecommendationResult> &recommendations, const QString &sourceInfo);
    void recommendationFailed(const QString &errorMessage);

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager *networkManager;

    // Contexte de la requête en cours
    int currentIdStagiaire;
    QList<Cours> currentAvailableCourses;

    // Construction du payload, parsing et validation
    QByteArray buildGroqPayload(const Stagiaire &s, const QList<Cours> &courses, const QStringList &completedCourses);
    QList<GroqRecommendationResult> parseAndValidateGroqResponse(const QByteArray &jsonData, const QList<Cours> &availableCourses);
    QList<GroqRecommendationResult> generateExpertHeuristicFallback(const Stagiaire &s, const QList<Cours> &availableCourses, const QStringList &completedCourses);
};

#endif // GROQRECOMMENDATIONSERVICE_H
