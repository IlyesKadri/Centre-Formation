#include "groqrecommendationservice.h"
#include "connection.h"
#include "birdconfig.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QUrl>
#include <QDebug>
#include <algorithm>

GroqRecommendationService::GroqRecommendationService(QObject *parent)
    : QObject(parent)
    , networkManager(new QNetworkAccessManager(this))
    , currentIdStagiaire(0)
{
    connect(networkManager, &QNetworkAccessManager::finished, this, &GroqRecommendationService::onReplyFinished);
}

GroqRecommendationService::~GroqRecommendationService()
{
}

QString GroqRecommendationService::getApiKey()
{
    QString apiKey = QString::fromUtf8(BirdConfig::GROQ_API_KEY).trimmed();
    if (apiKey.isEmpty() || apiKey == "YOUR_GROQ_API_KEY" || apiKey == "YOUR_API_KEY") {
        return QString();
    }
    return apiKey;
}

QString GroqRecommendationService::getModelName()
{
    // Modèle Groq Llama 3.3 70B Versatile
    return "llama-3.3-70b-versatile";
}

QString GroqRecommendationService::getApiEndpoint()
{
    return "https://api.groq.com/openai/v1/chat/completions";
}

void GroqRecommendationService::requestRecommendations(int idStagiaire)
{
    if (idStagiaire <= 0) {
        emit recommendationFailed("Veuillez sélectionner un stagiaire.");
        return;
    }

    // 1. Récupération des données réelles du stagiaire depuis Oracle
    Stagiaire s = Stagiaire::getById(idStagiaire);
    if (s.getIdStagiaire() <= 0) {
        emit recommendationFailed("Stagiaire introuvable dans la base Oracle.");
        return;
    }

    // 2. Récupération des cours réellement disponibles depuis Oracle
    QList<Cours> allCourses = Cours::afficher();
    currentAvailableCourses.clear();
    for (const Cours &c : allCourses) {
        if (c.getStatut().toUpper() != "ANNULE" && c.getStatut().toUpper() != "ANNULEE") {
            currentAvailableCourses.append(c);
        }
    }

    if (currentAvailableCourses.isEmpty()) {
        emit recommendationFailed("Aucune formation disponible pour effectuer une recommandation.");
        return;
    }

    // 3. Récupération des cours déjà suivis par ce stagiaire depuis Oracle
    QList<Inscription> inscriptions = Inscription::readByStagiaire(idStagiaire);
    QStringList completedCourses;
    for (const Inscription &ins : inscriptions) {
        if (!ins.getNomCours().isEmpty() && !completedCourses.contains(ins.getNomCours())) {
            completedCourses.append(ins.getNomCours());
        }
    }

    currentIdStagiaire = idStagiaire;

    QString apiKey = getApiKey();

    // 4. Vérification de la présence de la clé API Groq
    if (apiKey.isEmpty()) {
        // Mode hors-ligne / Moteur heuristique expert si la clé n'est pas encore saisie
        QList<GroqRecommendationResult> expertResults = generateExpertHeuristicFallback(s, currentAvailableCourses, completedCourses);
        emit recommendationsReady(expertResults, "Moteur d'orientation pédagogique (Clé GROQ_API_KEY non configurée dans birdconfig.h — Catalogue Oracle)");
        return;
    }

    // 5. Construction de la requête POST vers GROQ API (Format Chat Completions)
    QByteArray payload = buildGroqPayload(s, currentAvailableCourses, completedCourses);

    QNetworkRequest request;
    request.setUrl(QUrl(getApiEndpoint()));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(apiKey).toUtf8());

    networkManager->post(request, payload);
}

QByteArray GroqRecommendationService::buildGroqPayload(const Stagiaire &s, const QList<Cours> &courses, const QStringList &completedCourses)
{
    QJsonObject root;
    root["model"] = getModelName();

    // Activation du format JSON structuré pour Groq
    QJsonObject responseFormatObj;
    responseFormatObj["type"] = "json_object";
    root["response_format"] = responseFormatObj;

    QJsonArray messages;

    // Prompt SYSTEM
    QJsonObject systemMsg;
    systemMsg["role"] = "system";
    systemMsg["content"] =
        "Tu es un conseiller pédagogique spécialisé dans l'orientation des stagiaires d'un centre de formation professionnelle. "
        "Tu dois recommander les formations les plus pertinentes UNIQUEMENT parmi le catalogue réel fourni. "
        "Tu dois prendre en compte : le niveau du stagiaire, sa filière de formation, son statut et les cours déjà suivis. "
        "Tu ne dois JAMAIS inventer une formation qui n'existe pas dans le catalogue fourni. "
        "Tu dois répondre STRICTEMENT au format JSON suivant : "
        "{\"recommendations\": [{\"formation\": \"NomExactDuCours\", \"score\": 92, \"raison\": \"Explication pédagogique claire et concise.\"}]}";
    messages.append(systemMsg);

    // Prompt USER (Données Oracle structurées)
    QJsonObject userContext;
    QJsonObject stagiaireObj;
    stagiaireObj["nom"] = s.getNom();
    stagiaireObj["prenom"] = s.getPrenom();
    stagiaireObj["niveau"] = s.getNiveau();
    stagiaireObj["formation"] = s.getFormation();
    stagiaireObj["cours_deja_suivis"] = QJsonArray::fromStringList(completedCourses);
    userContext["stagiaire"] = stagiaireObj;

    QJsonArray coursesArr;
    for (const Cours &c : courses) {
        QJsonObject cObj;
        cObj["id"] = c.getIdCours();
        cObj["nom"] = c.getNom();
        cObj["duree_heures"] = c.getDuree();
        cObj["capacite"] = c.getCapacite();
        cObj["prix"] = c.getPrix();
        cObj["statut"] = c.getStatut();
        cObj["description"] = c.getDescription();
        coursesArr.append(cObj);
    }
    userContext["formations_disponibles_oracle"] = coursesArr;
    userContext["instruction"] = "Recommande au maximum les 3 formations les plus adaptées avec un score entre 0 et 100 et la justification.";

    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = QString::fromUtf8(QJsonDocument(userContext).toJson(QJsonDocument::Compact));
    messages.append(userMsg);

    root["messages"] = messages;
    root["temperature"] = 0.2;

    return QJsonDocument(root).toJson();
}

void GroqRecommendationService::onReplyFinished(QNetworkReply *reply)
{
    reply->deleteLater();

    if (!reply) {
        emit recommendationFailed("Erreur interne du client réseau.");
        return;
    }

    int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    // Gestion précise des erreurs HTTP Groq
    if (httpStatus == 401) {
        emit recommendationFailed("Clé API Groq invalide ou non autorisée (Erreur HTTP 401).");
        return;
    }
    if (httpStatus == 429) {
        emit recommendationFailed("Limite de requêtes Groq atteinte (Erreur HTTP 429). Veuillez réessayer plus tard.");
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        // En cas d'erreur de connexion / absence d'Internet
        if (reply->error() == QNetworkReply::HostNotFoundError ||
            reply->error() == QNetworkReply::ConnectionRefusedError ||
            reply->error() == QNetworkReply::TimeoutError) {
            emit recommendationFailed("Impossible de contacter le service de recommandation Groq. Vérifiez votre connexion Internet.");
            return;
        }

        // Fallback transparent sécurisé sur le moteur pédagogique local en cas d'erreur serveur
        Stagiaire s = Stagiaire::getById(currentIdStagiaire);
        QList<Inscription> inscriptions = Inscription::readByStagiaire(currentIdStagiaire);
        QStringList completedCourses;
        for (const Inscription &ins : inscriptions) completedCourses.append(ins.getNomCours());

        QList<GroqRecommendationResult> fallbackResults = generateExpertHeuristicFallback(s, currentAvailableCourses, completedCourses);
        if (!fallbackResults.isEmpty()) {
            emit recommendationsReady(fallbackResults, QString("Mode secours pédagogique (%1)").arg(reply->errorString()));
            return;
        }

        emit recommendationFailed(QString("Erreur service Groq : %1").arg(reply->errorString()));
        return;
    }

    QByteArray responseData = reply->readAll();
    QList<GroqRecommendationResult> validatedResults = parseAndValidateGroqResponse(responseData, currentAvailableCourses);

    if (validatedResults.isEmpty()) {
        emit recommendationFailed("La réponse du service Groq est invalide.");
        return;
    }

    emit recommendationsReady(validatedResults, QString("GROQ API Cloud (Modèle : %1 — Validé Oracle)").arg(getModelName()));
}

QList<GroqRecommendationResult> GroqRecommendationService::parseAndValidateGroqResponse(const QByteArray &jsonData, const QList<Cours> &availableCourses)
{
    QList<GroqRecommendationResult> results;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    if (doc.isNull() || !doc.isObject()) return results;

    QJsonObject root = doc.object();
    QString aiContent;

    // Format OpenAI / Groq Chat Completions : choices[0].message.content
    if (root.contains("choices") && root["choices"].isArray()) {
        QJsonArray choices = root["choices"].toArray();
        if (!choices.isEmpty() && choices[0].isObject()) {
            QJsonObject choiceObj = choices[0].toObject();
            if (choiceObj.contains("message") && choiceObj["message"].isObject()) {
                aiContent = choiceObj["message"].toObject().value("content").toString().trimmed();
            }
        }
    } else if (root.contains("recommendations")) {
        aiContent = QString::fromUtf8(jsonData);
    }

    if (aiContent.isEmpty()) return results;

    // Découpage propre du JSON si inclus dans des blocs ```json ... ```
    if (aiContent.startsWith("```")) {
        int start = aiContent.indexOf('{');
        int end = aiContent.lastIndexOf('}');
        if (start != -1 && end != -1 && end > start) {
            aiContent = aiContent.mid(start, end - start + 1);
        }
    }

    QJsonDocument parsedContent = QJsonDocument::fromJson(aiContent.toUtf8());
    if (parsedContent.isNull() || !parsedContent.isObject()) return results;

    QJsonArray recArr = parsedContent.object().value("recommendations").toArray();

    for (const QJsonValue &val : recArr) {
        if (!val.isObject()) continue;
        QJsonObject recObj = val.toObject();
        QString nomRec = recObj.value("formation").toString().trimmed();
        int score = recObj.value("score").toInt(85);
        QString raison = recObj.value("raison").toString().trimmed();

        // ⚠️ VÉRIFICATION STRICTE DES DONNÉES ORACLE :
        // L'IA Groq ne doit jamais recommander un cours qui n'existe pas dans Oracle !
        const Cours *matchedCours = nullptr;
        for (const Cours &c : availableCourses) {
            if (c.getNom().compare(nomRec, Qt::CaseInsensitive) == 0 ||
                nomRec.contains(c.getNom(), Qt::CaseInsensitive) ||
                c.getNom().contains(nomRec, Qt::CaseInsensitive)) {
                matchedCours = &c;
                break;
            }
        }

        if (matchedCours) {
            GroqRecommendationResult r;
            r.idCours = matchedCours->getIdCours();
            r.nomCours = matchedCours->getNom();
            r.duree = matchedCours->getDuree();
            r.capacite = matchedCours->getCapacite();
            r.statut = matchedCours->getStatut();
            r.score = std::max(10, std::min(100, score));
            r.raison = raison.isEmpty() ? "Formation recommandée par l'analyse pédagogique Groq." : raison;
            results.append(r);
        }

        if (results.size() >= 3) break; // Maximum 3 recommandations
    }

    return results;
}

QList<GroqRecommendationResult> GroqRecommendationService::generateExpertHeuristicFallback(const Stagiaire &s, const QList<Cours> &availableCourses, const QStringList &completedCourses)
{
    QList<GroqRecommendationResult> results;
    QString filiere = s.getFormation().toUpper();
    QString niveau = s.getNiveau().toUpper();

    struct ScoredCourse {
        Cours cours;
        int score;
        QString raison;
    };
    QList<ScoredCourse> scoredList;

    for (const Cours &c : availableCourses) {
        if (completedCourses.contains(c.getNom(), Qt::CaseInsensitive)) continue;

        int score = 60;
        QStringList reasons;

        QString nomC = c.getNom().toUpper();
        QString descC = c.getDescription().toUpper();

        if (nomC.contains(filiere) || descC.contains(filiere) || 
            (filiere.contains("INFO") && (nomC.contains("WEB") || nomC.contains("PYTHON") || nomC.contains("DEV") || nomC.contains("JAVA") || nomC.contains("SQL") || nomC.contains("C++"))) ||
            (filiere.contains("GESTION") && (nomC.contains("MANAGEMENT") || nomC.contains("COMPTA") || nomC.contains("PROJET"))) ||
            (filiere.contains("LANGUE") && (nomC.contains("ANGLAIS") || nomC.contains("FRANCAIS") || nomC.contains("COMMUNICATION")))) {
            score += 25;
            reasons << QString("Parfaite adéquation avec votre filière %1.").arg(s.getFormation());
        }

        if (niveau == "DEBUTANT") {
            if (c.getDuree() <= 40) {
                score += 15;
                reasons << "Volume horaire et rythme progressif parfaitement adaptés au niveau débutant.";
            }
        } else if (niveau == "INTERMEDIAIRE") {
            if (c.getDuree() >= 30 && c.getDuree() <= 60) {
                score += 15;
                reasons << "Approfondissement technique idéal pour consolider votre niveau intermédiaire.";
            }
        } else if (niveau == "AVANCE") {
            score += 10;
            reasons << "Modules spécialisés et expertise avancée pour valoriser votre parcours.";
        }

        if (c.getStatut().toUpper() == "PLANIFIE" || c.getStatut().toUpper() == "DISPONIBLE") {
            score += 5;
        }

        if (reasons.isEmpty()) {
            reasons << QString("Formation certifiante de %1 heures pour élargir votre champ de compétences.").arg(c.getDuree());
        }

        ScoredCourse sc;
        sc.cours = c;
        sc.score = std::min(98, score);
        sc.raison = reasons.join(" ");
        scoredList.append(sc);
    }

    std::sort(scoredList.begin(), scoredList.end(), [](const ScoredCourse &a, const ScoredCourse &b) {
        return a.score > b.score;
    });

    int count = std::min(3, static_cast<int>(scoredList.size()));
    for (int i = 0; i < count; ++i) {
        GroqRecommendationResult r;
        r.idCours = scoredList[i].cours.getIdCours();
        r.nomCours = scoredList[i].cours.getNom();
        r.duree = scoredList[i].cours.getDuree();
        r.capacite = scoredList[i].cours.getCapacite();
        r.statut = scoredList[i].cours.getStatut();
        r.score = scoredList[i].score;
        r.raison = scoredList[i].raison;
        results.append(r);
    }

    return results;
}
