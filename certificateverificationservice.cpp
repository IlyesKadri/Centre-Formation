#include "certificateverificationservice.h"
#include "connection.h"
#include "qrcode.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QDebug>

QString CertificateVerificationService::formaterReference(int idInscription, int annee)
{
    if (annee <= 0) {
        annee = QDate::currentDate().year();
    }
    return QString("AT-%1-%2")
        .arg(annee)
        .arg(idInscription, 5, 10, QChar('0'));
}

QString CertificateVerificationService::formaterContenuQrCode(const QString &reference)
{
    return reference.trimmed().toUpper();
}

QString CertificateVerificationService::formaterContenuQrCodeCompact(
    const QString &reference,
    const QString &prenom,
    const QString &nom,
    const QString &formation,
    const QString &dateDebut,
    const QString &dateFin,
    int dureeHeures,
    const QString &statut)
{
    QString statutAffiche = statut.trimmed();
    if (statutAffiche.toUpper() == "CONFIRMEE" || statutAffiche.toUpper() == "CONFIRME" || statutAffiche.isEmpty()) {
        statutAffiche = "VALIDE";
    } else if (statutAffiche.toUpper() == "ANNULEE" || statutAffiche.toUpper() == "ANNULE") {
        statutAffiche = "ANNULÉ";
    }

    QString stagiaireNom = (prenom.trimmed() + " " + nom.trimmed()).trimmed();
    if (stagiaireNom.isEmpty()) stagiaireNom = "Stagiaire";

    // Formatage compact des dates : dd/MM/yy
    QString dDebut = dateDebut.trimmed();
    QString dFin = dateFin.trimmed();
    if (dDebut.length() == 10 && dDebut.count('/') == 2) {
        dDebut = dDebut.left(6) + dDebut.right(2);
    }
    if (dFin.length() == 10 && dFin.count('/') == 2) {
        dFin = dFin.left(6) + dFin.right(2);
    }

    QString datesCompact = QString("%1-%2").arg(dDebut).arg(dFin);
    QString dureeCompact = QString("%1h").arg(dureeHeures);

    // Structure compacte : CERTIFICAT|AT-2026-00010|Eya Ben Amor|HTML CSS JavaScript|01/12/26-20/12/26|45h|VALIDE
    QString qrData = QString("CERTIFICAT|%1|%2|%3|%4|%5|%6")
        .arg(reference.trimmed())
        .arg(stagiaireNom)
        .arg(formation.trimmed())
        .arg(datesCompact)
        .arg(dureeCompact)
        .arg(statutAffiche);

    return qrData;
}

int CertificateVerificationService::extraireIdDepuisReference(const QString &referenceBrute, QString *messageErreur)
{
    QString ref = referenceBrute.trimmed().toUpper();
    if (ref.isEmpty()) {
        if (messageErreur) *messageErreur = "Veuillez saisir une référence de certificat.";
        return -1;
    }

    // Extraction tolérante de la référence AT-AAAA-XXXXX dans n'importe quel texte (CERTIFICATE_REF=..., URL, texte scanné)
    QRegularExpression rx(R"(AT-(\d{4})-(\d{1,8}))", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = rx.match(ref);

    if (match.hasMatch()) {
        int idInsc = match.captured(2).toInt();
        if (idInsc > 0) {
            return idInsc;
        }
    }

    // Cas tolérant si l'utilisateur saisit uniquement l'ID numérique
    bool isNumber = false;
    int directId = ref.toInt(&isNumber);
    if (isNumber && directId > 0) {
        return directId;
    }

    if (messageErreur) *messageErreur = "Format de référence invalide (Format attendu : AT-AAAA-XXXXX).";
    return -1;
}

CertificateVerificationResult CertificateVerificationService::verifierReference(const QString &referenceBrute)
{
    CertificateVerificationResult res;
    QString refClean = referenceBrute.trimmed().toUpper();
    res.reference = refClean;

    QString errSyntax;
    int idInsc = extraireIdDepuisReference(refClean, &errSyntax);
    if (idInsc <= 0) {
        res.existe = false;
        res.valide = false;
        res.statutCertificat = "❌ FORMAT INVALIDE";
        res.messageStatus = errSyntax.isEmpty() ? "Format de référence invalide." : errSyntax;
        return res;
    }

    res.idInscription = idInsc;

    // Requête préparée sécurisée dans Oracle XE
    QSqlQuery query(Connection::instance()->getDatabase());
    query.prepare(
        "SELECT i.ID_INSCRIPTION, i.ID_STAGIAIRE, i.ID_COURS, i.DATE_INSCRIPTION, i.STATUT, i.NOTE, "
        "s.NOM, s.PRENOM, s.EMAIL, s.TELEPHONE, s.NIVEAU, s.FORMATION, "
        "c.NOM AS NOM_COURS, c.DESCRIPTION, c.DUREE, c.PRIX, c.DATE_DEBUT, c.DATE_FIN "
        "FROM INSCRIPTION i "
        "JOIN STAGIAIRE s ON i.ID_STAGIAIRE = s.ID_STAGIAIRE "
        "JOIN COURS c ON i.ID_COURS = c.ID_COURS "
        "WHERE i.ID_INSCRIPTION = :idInsc"
    );
    query.bindValue(":idInsc", idInsc);

    if (!query.exec()) {
        qDebug() << "[CertificateVerificationService] SQL Error:" << query.lastError().text();
        res.existe = false;
        res.valide = false;
        res.statutCertificat = "❌ ERREUR SYSTÈME";
        res.messageStatus = "Impossible de vérifier le certificat auprès de la base Oracle.";
        return res;
    }

    if (!query.next()) {
        res.existe = false;
        res.valide = false;
        res.statutCertificat = "❌ CERTIFICAT NON VALIDE";
        res.messageStatus = "Cette référence de certificat n'existe pas dans notre base de données.";
        return res;
    }

    // Données trouvées dans Oracle
    res.existe = true;
    res.idInscription = query.value("ID_INSCRIPTION").toInt();
    res.idStagiaire = query.value("ID_STAGIAIRE").toInt();
    res.idCours = query.value("ID_COURS").toInt();
    res.dateInscription = query.value("DATE_INSCRIPTION").toDate();
    res.statutInscription = query.value("STATUT").toString();
    res.note = query.value("NOTE").toDouble();

    res.nomStagiaire = query.value("NOM").toString();
    res.prenomStagiaire = query.value("PRENOM").toString();
    res.emailStagiaire = query.value("EMAIL").toString();
    res.telephoneStagiaire = query.value("TELEPHONE").toString();
    res.niveau = query.value("NIVEAU").toString();
    res.filiere = query.value("FORMATION").toString();

    res.nomCours = query.value("NOM_COURS").toString();
    res.categorieCours = query.value("DESCRIPTION").toString();
    res.dureeCours = query.value("DUREE").toInt();
    res.dateDebut = query.value("DATE_DEBUT").toDate();
    res.dateFin = query.value("DATE_FIN").toDate();

    if (!res.dateDebut.isValid()) res.dateDebut = res.dateInscription;
    if (!res.dateFin.isValid()) res.dateFin = res.dateInscription;

    // Calcul de la mention
    if (res.note >= 16.0) res.mention = "Très Bien";
    else if (res.note >= 14.0) res.mention = "Bien";
    else if (res.note >= 12.0) res.mention = "Assez Bien";
    else if (res.note >= 10.0) res.mention = "Admis";
    else if (res.note > 0.0) res.mention = "Ajourné";
    else res.mention = "En Cours";

    // Formatage officiel de la référence retournée
    int anneeRef = res.dateInscription.isValid() ? res.dateInscription.year() : QDate::currentDate().year();
    res.reference = formaterReference(res.idInscription, anneeRef);

    // Analyse du statut métier
    QString statutUp = res.statutInscription.trimmed().toUpper();
    if (statutUp == "ANNULEE" || statutUp == "ANNULE") {
        res.valide = false;
        res.statutCertificat = "⚠ CERTIFICAT ANNULÉ";
        res.messageStatus = "Ce certificat a été révoqué car l'inscription correspondante a été annulée dans Oracle.";
    } else {
        res.valide = true;
        res.statutCertificat = "✓ CERTIFICAT AUTHENTIQUE";
        res.messageStatus = "Le certificat est authentique, valide et enregistré dans le registre officiel.";
    }

    return res;
}

QImage CertificateVerificationService::genererQrCodePourCertificat(const QString &referenceOuTexte, int taillePx)
{
    QString dataToEncode = referenceOuTexte.trimmed();

    // Si on passe une simple référence (ex: AT-2026-00001), on génère le format compact officiel complet
    if (!dataToEncode.contains('|')) {
        int idInsc = extraireIdDepuisReference(dataToEncode);
        if (idInsc > 0 && Connection::instance()->getDatabase().isOpen()) {
            CertificateVerificationResult res = verifierReference(dataToEncode);
            if (res.existe) {
                dataToEncode = formaterContenuQrCodeCompact(
                    res.reference,
                    res.prenomStagiaire,
                    res.nomStagiaire,
                    res.nomCours,
                    res.dateDebut.toString("dd/MM/yyyy"),
                    res.dateFin.toString("dd/MM/yyyy"),
                    res.dureeCours,
                    res.statutInscription
                );
            }
        }
    }

    QString refDoc = dataToEncode.contains('|') ? dataToEncode.split('|').value(1) : dataToEncode;
    qDebug() << "QR generation started for certificate:" << refDoc;
    qDebug() << "QR DATA:" << dataToEncode;

    // Génération ISO 18004 avec Quiet Zone = 4 modules et ECC Medium pour un QR compact et ultra-lisible
    QImage qrImg = qrcodegen::QrCode::generateQrImage(dataToEncode, taillePx, 4, qrcodegen::QrCode::Ecc::MEDIUM);

    // Export test image qr_debug.png pour validation directe
    qrImg.save("d:/downloads/CentreFormation/qr_debug.png");

    return qrImg;
}
