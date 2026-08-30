#include "birdemailservice.h"
#include "birdconfig.h"
#include "connection.h"

#include <QProcessEnvironment>
#include <QFile>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QUrl>
#include <QDate>
#include <QDebug>

BirdEmailService::BirdEmailService(QObject *parent)
    : QObject(parent)
    , networkManager(new QNetworkAccessManager(this))
{
    connect(networkManager, &QNetworkAccessManager::finished, this, &BirdEmailService::onReplyFinished);
}

BirdEmailService::~BirdEmailService()
{
}

QString BirdEmailService::getApiKey()
{
    // 1. Configuration locale projet dans birdconfig.h
    QString configKey = QString::fromUtf8(BirdConfig::BIRD_API_KEY).trimmed();
    if (!configKey.isEmpty() && configKey != "YOUR_BIRD_API_KEY" && configKey != "YOUR_NEW_BIRD_API_KEY") {
        return configKey;
    }

    // 2. Variable d'environnement système BIRD_API_KEY
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (env.contains("BIRD_API_KEY") && !env.value("BIRD_API_KEY").trimmed().isEmpty()) {
        return env.value("BIRD_API_KEY").trimmed();
    }

    // 3. Fichier local sécurisé non versionné (api_config.ini ou .env)
    if (QFile::exists("api_config.ini")) {
        QSettings settings("api_config.ini", QSettings::IniFormat);
        QString key = settings.value("BIRD/api_key", "").toString().trimmed();
        if (!key.isEmpty()) return key;
    }

    if (QFile::exists(".env")) {
        QFile envFile(".env");
        if (envFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            while (!envFile.atEnd()) {
                QString line = QString::fromUtf8(envFile.readLine()).trimmed();
                if (line.startsWith("BIRD_API_KEY=")) {
                    int eqIdx = line.indexOf('=');
                    if (eqIdx != -1) {
                        QString val = line.mid(eqIdx + 1).trimmed().remove('"').remove('\'');
                        if (!val.isEmpty()) return val;
                    }
                }
            }
        }
    }

    return QString();
}

QString BirdEmailService::getRegion()
{
    QString apiKey = getApiKey();
    if (apiKey.startsWith("bk_us1_")) {
        return "us1";
    }
    // Par défaut pour les clés européennes bk_eu1_ ou standard
    return "eu1";
}

QString BirdEmailService::getApiEndpoint()
{
    // API Bird Email actuelle : https://{region}.platform.bird.com/v1/email/messages
    return QString("https://%1.platform.bird.com/v1/email/messages").arg(getRegion());
}

QString BirdEmailService::getSenderEmail()
{
    QString configSender = QString::fromUtf8(BirdConfig::SENDER_EMAIL).trimmed();
    if (!configSender.isEmpty()) {
        return configSender;
    }

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (env.contains("BIRD_SENDER_EMAIL") && !env.value("BIRD_SENDER_EMAIL").trimmed().isEmpty()) {
        return env.value("BIRD_SENDER_EMAIL").trimmed();
    }

    return "onboarding@messagebird.dev";
}

QString BirdEmailService::getSenderName()
{
    QString configName = QString::fromUtf8(BirdConfig::SENDER_NAME).trimmed();
    if (!configName.isEmpty()) {
        return configName;
    }

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (env.contains("BIRD_SENDER_NAME") && !env.value("BIRD_SENDER_NAME").trimmed().isEmpty()) {
        return env.value("BIRD_SENDER_NAME").trimmed();
    }

    return "Bird";
}

void BirdEmailService::testBirdEmail()
{
    // Reproduit exactement le code officiel TypeScript / SDK Bird :
    // from: "onboarding@messagebird.dev"
    // to: ["delivered@messagebird.dev"]
    // subject: "Hello World"
    // html: "<p>You made your <strong>first email fly</strong>. Congratulations!</p>"
    envoyerEmail(
        "delivered@messagebird.dev",
        "Hello World",
        "<p>You made your <strong>first email fly</strong>. Congratulations!</p>",
        0
    );
}

void BirdEmailService::envoyerConfirmationInscription(int idInscription)
{
    if (idInscription <= 0) {
        emit emailEchec(idInscription, "", "ID d'inscription invalide.");
        return;
    }

    // 1. Récupération des données réelles de l'inscription depuis Oracle
    Inscription ins = Inscription::getById(idInscription);
    if (ins.getIdInscription() <= 0) {
        emit emailEchec(idInscription, "", "Inscription introuvable dans la base Oracle.");
        return;
    }

    // 2. Récupération du stagiaire et de son adresse email depuis Oracle
    Stagiaire s = Stagiaire::getById(ins.getIdStagiaire());
    if (s.getIdStagiaire() <= 0) {
        emit emailEchec(idInscription, "", "Stagiaire associé introuvable dans Oracle.");
        return;
    }

    QString destinataireEmail = s.getEmail().trimmed();
    if (destinataireEmail.isEmpty() || !destinataireEmail.contains('@')) {
        emit emailEchec(idInscription, destinataireEmail, "L'adresse email du stagiaire est invalide ou non renseignée.");
        return;
    }

    // 3. Récupération du cours associé depuis Oracle
    Cours c = Cours::getById(ins.getIdCours());

    QString nomCours = ins.getNomCours().isEmpty() ? (c.getNom().isEmpty() ? "Formation Professionnelle" : c.getNom()) : ins.getNomCours();
    QString sujet = QString("Confirmation de votre inscription — %1").arg(nomCours);
    QString html = genererHtmlEmail(s, c, ins);

    envoyerEmail(destinataireEmail, sujet, html, idInscription);
}

void BirdEmailService::envoyerEmail(const QString &destinataire, const QString &sujet, const QString &htmlContent, int idInscription)
{
    QString apiKey = getApiKey();
    if (apiKey.isEmpty()) {
        emit emailEchec(idInscription, destinataire, "La clé API Bird n'est pas configurée (Veuillez renseigner API_KEY dans birdconfig.h).");
        return;
    }

    // Construction du payload JSON exact correspondant au SDK Bird Email API
    QJsonObject root;
    root["from"] = getSenderEmail();

    QJsonArray toArr;
    toArr.append(destinataire);
    root["to"] = toArr;

    root["subject"] = sujet;
    root["html"] = htmlContent;

    QByteArray payload = QJsonDocument(root).toJson(QJsonDocument::Compact);

    // Requête HTTP POST vers l'API Bird actuelle
    QNetworkRequest request;
    request.setUrl(QUrl(getApiEndpoint()));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    // Format officiel Bird Email API : "Authorization: Bearer <BIRD_API_KEY>"
    request.setRawHeader("Authorization", QString("Bearer %1").arg(apiKey).toUtf8());

    QNetworkReply *reply = networkManager->post(request, payload);
    pendingInscRequests.insert(reply, idInscription);
    pendingDestinataires.insert(reply, destinataire);
}

void BirdEmailService::verifierStatutMessage(const QString &messageId)
{
    if (messageId.isEmpty()) return;

    QString apiKey = getApiKey();
    if (apiKey.isEmpty()) return;

    QUrl url(QString("%1/%2").arg(getApiEndpoint()).arg(messageId));
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", QString("Bearer %1").arg(apiKey).toUtf8());

    QNetworkReply *reply = networkManager->get(request);
    pendingStatusCheckIds.insert(reply, messageId);
}

void BirdEmailService::onReplyFinished(QNetworkReply *reply)
{
    if (!reply) return;
    reply->deleteLater();

    // Cas de la vérification de statut (GET /v1/email/messages/{id})
    if (pendingStatusCheckIds.contains(reply)) {
        QString msgId = pendingStatusCheckIds.take(reply);
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isNull() && doc.isObject()) {
            QString status = doc.object().value("status").toString();
            emit statutMessageRecu(msgId, status);
        }
        return;
    }

    int idInscription = pendingInscRequests.take(reply);
    QString destinataire = pendingDestinataires.take(reply);

    int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray responseData = reply->readAll();

    // Debug technique sécurisé (jamais de clé API affichée)
    qDebug() << "[BirdEmailService] Code HTTP reçu :" << httpStatus;
    qDebug() << "[BirdEmailService] Réponse brute :" << responseData;

    // 1. SUCCÈS : Tout code HTTP 2xx (notamment 202 Accepted, 200 OK, 201 Created)
    if (httpStatus >= 200 && httpStatus < 300) {
        QString messageId = "em_accepted";
        QString status = "accepted";
        QJsonDocument doc = QJsonDocument::fromJson(responseData);
        if (!doc.isNull() && doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj.contains("id")) {
                messageId = obj.value("id").toString();
            }
            if (obj.contains("status")) {
                status = obj.value("status").toString();
            }
        }
        emit emailEnvoyeSucces(idInscription, destinataire, messageId, status);
        return;
    }

    // 2. CODES D'ERREUR HTTP EXPLICITES
    if (httpStatus == 400) {
        emit emailEchec(idInscription, destinataire, "Les données envoyées à Bird sont invalides (Erreur HTTP 400).");
        return;
    }
    if (httpStatus == 401) {
        emit emailEchec(idInscription, destinataire, "Clé API Bird invalide ou non autorisée (Erreur HTTP 401).");
        return;
    }
    if (httpStatus == 403) {
        emit emailEchec(idInscription, destinataire, "Accès au service Bird refusé (Erreur HTTP 403).");
        return;
    }
    if (httpStatus == 422) {
        emit emailEchec(idInscription, destinataire, "Destinataire ou expéditeur non autorisé par Bird (Erreur HTTP 422).");
        return;
    }
    if (httpStatus == 429) {
        emit emailEchec(idInscription, destinataire, "Limite d'envoi Bird atteinte (Erreur HTTP 429). Veuillez réessayer plus tard.");
        return;
    }
    if (httpStatus >= 500) {
        emit emailEchec(idInscription, destinataire, QString("Le service Bird est temporairement indisponible (Erreur serveur %1).").arg(httpStatus));
        return;
    }

    // 3. ERREUR DE CONNEXION RÉSEAU / TIMEOUT
    if (reply->error() != QNetworkReply::NoError) {
        if (reply->error() == QNetworkReply::HostNotFoundError ||
            reply->error() == QNetworkReply::ConnectionRefusedError ||
            reply->error() == QNetworkReply::TimeoutError) {
            emit emailEchec(idInscription, destinataire, "Impossible de contacter le service email Bird. Vérifiez votre connexion Internet.");
            return;
        }

        emit emailEchec(idInscription, destinataire, QString("Erreur lors de l'envoi de l'email : %1").arg(reply->errorString()));
        return;
    }

    emit emailEchec(idInscription, destinataire, "Réponse inattendue du service Bird.");
}

QString BirdEmailService::genererHtmlEmail(const Stagiaire &s, const Cours &c, const Inscription &ins)
{
    QString nomStagiaire = QString("%1 %2").arg(s.getPrenom()).arg(s.getNom());
    QString nomCours = ins.getNomCours().isEmpty() ? (c.getNom().isEmpty() ? "Formation Professionnelle" : c.getNom()) : ins.getNomCours();
    QString dateDebutStr = c.getDateDebut().isValid() ? c.getDateDebut().toString("dd/MM/yyyy") : ins.getDateInscription().toString("dd/MM/yyyy");
    QString dateFinStr = c.getDateFin().isValid() ? c.getDateFin().toString("dd/MM/yyyy") : ins.getDateInscription().toString("dd/MM/yyyy");
    QString statutStr = ins.getStatut().isEmpty() ? "Confirmée" : ins.getStatut();
    QString refStr = QString("INS-%1-%2").arg(QDate::currentDate().year()).arg(ins.getIdInscription(), 5, 10, QChar('0'));
    QString dureeStr = QString("%1").arg(c.getDuree() > 0 ? c.getDuree() : 30);
    QString prixStr = c.getPrix() > 0 ? QString("%1 TND").arg(QString::number(c.getPrix(), 'f', 2)) : "Pris en charge";

    QString html = QString(
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<meta charset='utf-8'>"
        "<style>"
        "  body { font-family: Arial, sans-serif; background-color: #f8fafc; margin: 0; padding: 20px; color: #1e293b; }"
        "  .container { max-width: 600px; margin: 0 auto; background-color: #ffffff; border-radius: 8px; border: 1px solid #e2e8f0; overflow: hidden; }"
        "  .header { background-color: #0f172a; padding: 25px 20px; text-align: center; color: #ffffff; }"
        "  .header h1 { margin: 0; font-size: 18px; letter-spacing: 1px; color: #ffffff; }"
        "  .badge { display: inline-block; background-color: #059669; color: #ffffff; padding: 4px 12px; border-radius: 12px; font-size: 12px; font-weight: bold; margin-top: 10px; }"
        "  .body-content { padding: 25px; line-height: 1.6; font-size: 14px; }"
        "  .card { background-color: #f1f5f9; border-left: 4px solid #0284c7; padding: 15px; margin: 15px 0; border-radius: 4px; }"
        "  .course-title { font-size: 16px; font-weight: bold; color: #0f172a; margin-bottom: 10px; }"
        "  .info-row { margin: 6px 0; font-size: 13px; }"
        "  .info-lbl { font-weight: bold; color: #475569; display: inline-block; width: 140px; }"
        "  .info-val { font-weight: bold; color: #0f172a; }"
        "  .footer { background-color: #f8fafc; padding: 15px; text-align: center; font-size: 11px; color: #94a3b8; border-top: 1px solid #e2e8f0; }"
        "</style>"
        "</head>"
        "<body>"
        "  <div class='container'>"
        "    <div class='header'>"
        "      <h1>CENTRE DE FORMATION PROFESSIONNELLE</h1>"
        "      <div class='badge'>✓ INSCRIPTION CONFIRMÉE</div>"
        "    </div>"
        "    <div class='body-content'>"
        "      <p>Bonjour <strong>%1</strong>,</p>"
        "      <p>Votre inscription a été confirmée avec succès dans notre système de gestion.</p>"
        "      <div class='card'>"
        "        <div class='course-title'>FORMATION : %2</div>"
        "        <div class='info-row'><span class='info-lbl'>Date de début :</span><span class='info-val'>%3</span></div>"
        "        <div class='info-row'><span class='info-lbl'>Date de fin :</span><span class='info-val'>%4</span></div>"
        "        <div class='info-row'><span class='info-lbl'>Volume horaire :</span><span class='info-val'>%5 heures</span></div>"
        "        <div class='info-row'><span class='info-lbl'>Frais pédagogiques :</span><span class='info-val'>%6</span></div>"
        "        <div class='info-row'><span class='info-lbl'>Référence :</span><span class='info-val' style='color: #0284c7;'>%7</span></div>"
        "        <div class='info-row'><span class='info-lbl'>Statut :</span><span class='info-val' style='color: #059669;'>%8</span></div>"
        "      </div>"
        "      <p>Merci pour votre confiance.</p>"
        "      <p style='margin-top: 20px;'>Cordialement,<br><strong>Direction des Études</strong><br>Centre de Formation Professionnelle</p>"
        "    </div>"
        "    <div class='footer'>"
        "      Notification officielle Bird Email — Référence : %7<br>"
        "      © %9 Centre de Formation Professionnelle."
        "    </div>"
        "  </div>"
        "</body>"
        "</html>"
    )
    .arg(nomStagiaire)
    .arg(nomCours)
    .arg(dateDebutStr)
    .arg(dateFinStr)
    .arg(dureeStr)
    .arg(prixStr)
    .arg(refStr)
    .arg(statutStr)
    .arg(QDate::currentDate().year());

    return html;
}
