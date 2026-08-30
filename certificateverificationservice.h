#ifndef CERTIFICATEVERIFICATIONSERVICE_H
#define CERTIFICATEVERIFICATIONSERVICE_H

#include <QString>
#include <QDate>
#include <QImage>

struct CertificateVerificationResult
{
    bool existe = false;
    bool valide = false;
    QString reference;
    int idInscription = 0;
    int idStagiaire = 0;
    int idCours = 0;
    QString nomStagiaire;
    QString prenomStagiaire;
    QString emailStagiaire;
    QString telephoneStagiaire;
    QString filiere;
    QString niveau;
    QString nomCours;
    QString categorieCours;
    int dureeCours = 0;
    double note = 0.0;
    QString mention;
    QDate dateInscription;
    QDate dateDebut;
    QDate dateFin;
    QString statutInscription;
    QString statutCertificat;
    QString messageStatus;
};

class CertificateVerificationService
{
public:
    // Formatage officiel de la référence : AT-2026-00010
    static QString formaterReference(int idInscription, int annee = -1);

    // Contenu réel encodé dans le QR Code pour lecture smartphone
    static QString formaterContenuQrCode(const QString &reference);

    // Contenu compact structuré pour QR Code officiel (Haute lisibilité smartphone)
    static QString formaterContenuQrCodeCompact(
        const QString &reference,
        const QString &prenom,
        const QString &nom,
        const QString &formation,
        const QString &dateDebut,
        const QString &dateFin,
        int dureeHeures,
        const QString &statut);

    // Extraction et validation syntaxique de l'ID depuis la référence ou le texte QR scanné
    static int extraireIdDepuisReference(const QString &referenceBrute, QString *messageErreur = nullptr);

    // Vérification dynamique complète de l'authenticité auprès d'Oracle (Requête préparée)
    static CertificateVerificationResult verifierReference(const QString &referenceBrute);

    // Génération du QR Code officiel prêt pour intégration PDF ou affichage UI
    static QImage genererQrCodePourCertificat(const QString &reference, int taillePx = 250);
};

#endif // CERTIFICATEVERIFICATIONSERVICE_H
