#ifndef BIRDEMAILSERVICE_H
#define BIRDEMAILSERVICE_H

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QMap>

#include "stagiaire.h"
#include "cours.h"
#include "inscription.h"

class BirdEmailService : public QObject
{
    Q_OBJECT

public:
    explicit BirdEmailService(QObject *parent = nullptr);
    virtual ~BirdEmailService();

    // Envoie l'email officiel de confirmation pour une inscription Oracle
    void envoyerConfirmationInscription(int idInscription);

    // Envoie générique d'email via Bird Platform API
    void envoyerEmail(const QString &destinataire, const QString &sujet, const QString &htmlContent, int idInscription = 0);

    // Test technique initial Sandbox (reproduisant exactement le code SDK TypeScript)
    void testBirdEmail();

    // Vérifie l'évolution du statut du message (GET /v1/email/messages/{id})
    void verifierStatutMessage(const QString &messageId);

    // Configuration & Détection de Région Automatique
    static QString getApiKey();
    static QString getRegion();
    static QString getApiEndpoint();
    static QString getSenderEmail();
    static QString getSenderName();

    // Générateur de template HTML professionnel et responsive
    static QString genererHtmlEmail(const Stagiaire &s, const Cours &c, const Inscription &ins);

signals:
    void emailEnvoyeSucces(int idInscription, const QString &destinataire, const QString &messageId, const QString &status);
    void emailEchec(int idInscription, const QString &destinataire, const QString &erreur);
    void statutMessageRecu(const QString &messageId, const QString &status);

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager *networkManager;
    QMap<QNetworkReply*, int> pendingInscRequests;
    QMap<QNetworkReply*, QString> pendingDestinataires;
    QMap<QNetworkReply*, QString> pendingStatusCheckIds;
};

#endif // BIRDEMAILSERVICE_H
