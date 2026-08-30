#include "inscription.h"
#include "connection.h"
#include "cours.h"
#include "stagiaire.h"
#include "qrcode.h"
#include "certificateverificationservice.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QPrinter>
#include <QPainter>
#include <QPageSize>
#include <QPageLayout>
#include <QDebug>

// ============================================================
// Constructeurs
// ============================================================

Inscription::Inscription()
    : idInscription(0), idStagiaire(0), idCours(0), note(0.0)
{
}

Inscription::Inscription(int idInscription,
                         int idStagiaire,
                         int idCours,
                         const QDate& dateInscription,
                         const QString& statut,
                         double note)
    : idInscription(idInscription),
      idStagiaire(idStagiaire),
      idCours(idCours),
      dateInscription(dateInscription),
      statut(statut),
      note(note)
{
}

// ============================================================
// GETTERS & SETTERS
// ============================================================

int Inscription::getIdInscription() const { return idInscription; }
int Inscription::getIdStagiaire() const { return idStagiaire; }
int Inscription::getIdCours() const { return idCours; }
QDate Inscription::getDateInscription() const { return dateInscription; }
QString Inscription::getStatut() const { return statut; }
double Inscription::getNote() const { return note; }
QString Inscription::getNomStagiaire() const { return nomStagiaire; }
QString Inscription::getNomCours() const { return nomCours; }

void Inscription::setIdInscription(int id) { idInscription = id; }
void Inscription::setIdStagiaire(int idStag) { idStagiaire = idStag; }
void Inscription::setIdCours(int idC) { idCours = idC; }
void Inscription::setDateInscription(const QDate& date) { dateInscription = date; }
void Inscription::setStatut(const QString& value) { statut = value; }
void Inscription::setNote(double n) { note = n; }
void Inscription::setNomStagiaire(const QString& nom) { nomStagiaire = nom; }
void Inscription::setNomCours(const QString& nom) { nomCours = nom; }

// ============================================================
// VALIDATIONS C++ & REGLE METIER
// ============================================================

bool Inscription::estValide(QString& erreur) const
{
    if (idStagiaire <= 0) {
        erreur = "Veuillez sélectionner un stagiaire valide.";
        return false;
    }

    if (idCours <= 0) {
        erreur = "Veuillez sélectionner un cours valide.";
        return false;
    }

    if (!dateInscription.isValid()) {
        erreur = "La date d'inscription est invalide.";
        return false;
    }

    // Contrôle de saisie : la date d'inscription doit être antérieure à la date de début du cours
    Cours c = Cours::getById(idCours);
    if (c.getIdCours() > 0 && c.getDateDebut().isValid()) {
        if (dateInscription >= c.getDateDebut()) {
            erreur = QString("Contrôle de saisie : La date d'inscription (%1) doit être antérieure à la date de début du cours (%2 prévue le %3).")
                        .arg(dateInscription.toString("dd/MM/yyyy"))
                        .arg(c.getNom())
                        .arg(c.getDateDebut().toString("dd/MM/yyyy"));
            return false;
        }
    }

    if (statut != "INSCRIT" && statut != "TERMINE" && statut != "ABANDONNE") {
        erreur = "Le statut d'inscription est invalide.";
        return false;
    }

    if (note < 0.0 || note > 20.0) {
        erreur = "La note doit être comprise strictement entre 0 et 20.";
        return false;
    }

    return true;
}

bool Inscription::verifierRegleMetier(int idStagiaire, int idCours, int idInscriptionActuelle, QString& messageErreur)
{
    QSqlDatabase db = Connection::instance()->getDatabase();

    // 1. Vérification doublon : Le même stagiaire ne peut pas s'inscrire deux fois au même cours
    QSqlQuery queryDoublon(db);
    queryDoublon.prepare(
        "SELECT COUNT(*) FROM INSCRIPTION "
        "WHERE ID_STAGIAIRE = :idStag AND ID_COURS = :idCours "
        "AND ID_INSCRIPTION != :idInsc AND STATUT != 'ABANDONNE'"
    );
    queryDoublon.bindValue(":idStag", idStagiaire);
    queryDoublon.bindValue(":idCours", idCours);
    queryDoublon.bindValue(":idInsc", idInscriptionActuelle);

    if (queryDoublon.exec() && queryDoublon.next()) {
        if (queryDoublon.value(0).toInt() > 0) {
            messageErreur = "Inscripton refusée !\nCe stagiaire est déjà inscrit à ce cours.";
            return false;
        }
    }

    // 2. Vérification Capacité du cours
    Cours c = Cours::getById(idCours);
    if (c.getIdCours() <= 0) {
        messageErreur = "Le cours sélectionné n'existe pas dans Oracle.";
        return false;
    }

    QSqlQuery queryInscrits(db);
    queryInscrits.prepare(
        "SELECT COUNT(*) FROM INSCRIPTION "
        "WHERE ID_COURS = :idCours AND STATUT != 'ABANDONNE' "
        "AND ID_INSCRIPTION != :idInsc"
    );
    queryInscrits.bindValue(":idCours", idCours);
    queryInscrits.bindValue(":idInsc", idInscriptionActuelle);

    int nbActuel = 0;
    if (queryInscrits.exec() && queryInscrits.next()) {
        nbActuel = queryInscrits.value(0).toInt();
    }

    if (nbActuel >= c.getCapacite()) {
        messageErreur = QString("Cours complet !\nCapacité maximale : %1\nNombre actuel d'inscrits : %2")
                            .arg(c.getCapacite())
                            .arg(nbActuel);
        return false;
    }

    return true;
}

// ============================================================
// AJOUTER
// ============================================================

bool Inscription::ajouter()
{
    QString erreur;
    if (!estValide(erreur)) {
        qDebug() << "Validation Inscription :" << erreur;
        return false;
    }

    QSqlQuery query(Connection::instance()->getDatabase());
    query.prepare(
        "INSERT INTO INSCRIPTION "
        "(ID_STAGIAIRE, ID_COURS, DATE_INSCRIPTION, STATUT, NOTE) "
        "VALUES "
        "(:idStag, :idCours, :dateInsc, :statut, :note)"
    );

    query.bindValue(":idStag", idStagiaire);
    query.bindValue(":idCours", idCours);
    query.bindValue(":dateInsc", dateInscription);
    query.bindValue(":statut", statut);
    query.bindValue(":note", note);

    if (!query.exec()) {
        qDebug() << "Erreur ajout inscription Oracle :" << query.lastError().text();
        return false;
    }

    return true;
}

// ============================================================
// MODIFIER
// ============================================================

bool Inscription::modifier()
{
    if (idInscription <= 0) return false;

    QString erreur;
    if (!estValide(erreur)) return false;

    QSqlQuery query(Connection::instance()->getDatabase());
    query.prepare(
        "UPDATE INSCRIPTION SET "
        "ID_STAGIAIRE = :idStag, "
        "ID_COURS = :idCours, "
        "DATE_INSCRIPTION = :dateInsc, "
        "STATUT = :statut, "
        "NOTE = :note "
        "WHERE ID_INSCRIPTION = :id"
    );

    query.bindValue(":idStag", idStagiaire);
    query.bindValue(":idCours", idCours);
    query.bindValue(":dateInsc", dateInscription);
    query.bindValue(":statut", statut);
    query.bindValue(":note", note);
    query.bindValue(":id", idInscription);

    if (!query.exec()) {
        qDebug() << "Erreur modification inscription Oracle :" << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

// ============================================================
// SUPPRIMER
// ============================================================

bool Inscription::supprimer()
{
    if (idInscription <= 0) return false;

    QSqlQuery query(Connection::instance()->getDatabase());
    query.prepare("DELETE FROM INSCRIPTION WHERE ID_INSCRIPTION = :id");
    query.bindValue(":id", idInscription);

    if (!query.exec()) {
        qDebug() << "Erreur suppression inscription Oracle :" << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

// ============================================================
// AFFICHER AVEC JOIN
// ============================================================

QList<Inscription> Inscription::afficher()
{
    QList<Inscription> liste;
    QSqlQuery query(Connection::instance()->getDatabase());

    query.prepare(
        "SELECT i.ID_INSCRIPTION, i.ID_STAGIAIRE, i.ID_COURS, i.DATE_INSCRIPTION, i.STATUT, i.NOTE, "
        "s.NOM || ' ' || s.PRENOM AS STAGIAIRE_NOM, c.NOM AS COURS_NOM "
        "FROM INSCRIPTION i "
        "JOIN STAGIAIRE s ON i.ID_STAGIAIRE = s.ID_STAGIAIRE "
        "JOIN COURS c ON i.ID_COURS = c.ID_COURS "
        "ORDER BY i.ID_INSCRIPTION DESC"
    );

    if (!query.exec()) {
        qDebug() << "Erreur affichage inscriptions Oracle :" << query.lastError().text();
        return liste;
    }

    while (query.next()) {
        Inscription ins;
        ins.setIdInscription(query.value("ID_INSCRIPTION").toInt());
        ins.setIdStagiaire(query.value("ID_STAGIAIRE").toInt());
        ins.setIdCours(query.value("ID_COURS").toInt());
        ins.setDateInscription(query.value("DATE_INSCRIPTION").toDate());
        ins.setStatut(query.value("STATUT").toString());
        ins.setNote(query.value("NOTE").toDouble());
        ins.setNomStagiaire(query.value("STAGIAIRE_NOM").toString());
        ins.setNomCours(query.value("COURS_NOM").toString());

        liste.append(ins);
    }

    return liste;
}

// ============================================================
// RECHERCHER
// ============================================================

QList<Inscription> Inscription::rechercher(const QString& searchStagiaire,
                                           const QString& searchCours,
                                           const QString& statut)
{
    QList<Inscription> liste;
    QString sql =
        "SELECT i.ID_INSCRIPTION, i.ID_STAGIAIRE, i.ID_COURS, i.DATE_INSCRIPTION, i.STATUT, i.NOTE, "
        "s.NOM || ' ' || s.PRENOM AS STAGIAIRE_NOM, c.NOM AS COURS_NOM "
        "FROM INSCRIPTION i "
        "JOIN STAGIAIRE s ON i.ID_STAGIAIRE = s.ID_STAGIAIRE "
        "JOIN COURS c ON i.ID_COURS = c.ID_COURS "
        "WHERE 1=1";

    if (!searchStagiaire.trimmed().isEmpty())
        sql += " AND (UPPER(s.NOM) LIKE UPPER(:stag) OR UPPER(s.PRENOM) LIKE UPPER(:stag))";

    if (!searchCours.trimmed().isEmpty())
        sql += " AND UPPER(c.NOM) LIKE UPPER(:cours)";

    if (!statut.trimmed().isEmpty())
        sql += " AND i.STATUT = :statut";

    sql += " ORDER BY i.ID_INSCRIPTION DESC";

    QSqlQuery query(Connection::instance()->getDatabase());
    query.prepare(sql);

    if (!searchStagiaire.trimmed().isEmpty())
        query.bindValue(":stag", "%" + searchStagiaire.trimmed() + "%");

    if (!searchCours.trimmed().isEmpty())
        query.bindValue(":cours", "%" + searchCours.trimmed() + "%");

    if (!statut.trimmed().isEmpty())
        query.bindValue(":statut", statut);

    if (!query.exec()) {
        qDebug() << "Erreur recherche inscription Oracle :" << query.lastError().text();
        return liste;
    }

    while (query.next()) {
        Inscription ins;
        ins.setIdInscription(query.value("ID_INSCRIPTION").toInt());
        ins.setIdStagiaire(query.value("ID_STAGIAIRE").toInt());
        ins.setIdCours(query.value("ID_COURS").toInt());
        ins.setDateInscription(query.value("DATE_INSCRIPTION").toDate());
        ins.setStatut(query.value("STATUT").toString());
        ins.setNote(query.value("NOTE").toDouble());
        ins.setNomStagiaire(query.value("STAGIAIRE_NOM").toString());
        ins.setNomCours(query.value("COURS_NOM").toString());

        liste.append(ins);
    }

    return liste;
}

// ============================================================
// RECHERCHE GLOBALE & TRI MULTICRITÈRE (DYNAMIQUE ORACLE)
// ============================================================

static QString mapCritereInscriptionSql(const QString& champ)
{
    QString c = champ.trimmed().toLower();
    if (c == "stagiaire" || c == "nom stagiaire" || c == "nom") return "s.NOM";
    if (c == "cours" || c == "nom cours") return "c.NOM";
    if (c == "date inscription" || c == "date_inscription" || c == "date") return "i.DATE_INSCRIPTION";
    if (c == "statut") return "i.STATUT";
    if (c == "note") return "i.NOTE";
    if (c == "id" || c == "id_inscription") return "i.ID_INSCRIPTION";
    return "";
}

QList<Inscription> Inscription::rechercherGlobale(const QString& texte)
{
    return rechercherEtTrier(texte, {});
}

QList<Inscription> Inscription::rechercherEtTrier(const QString& search, const QList<QPair<QString, QString>>& criteresTri)
{
    QList<Inscription> liste;
    QString searchStr = search.trimmed();

    QString sql =
        "SELECT i.ID_INSCRIPTION, i.ID_STAGIAIRE, i.ID_COURS, i.DATE_INSCRIPTION, i.STATUT, i.NOTE, "
        "s.NOM || ' ' || s.PRENOM AS STAGIAIRE_NOM, c.NOM AS COURS_NOM "
        "FROM INSCRIPTION i "
        "JOIN STAGIAIRE s ON i.ID_STAGIAIRE = s.ID_STAGIAIRE "
        "JOIN COURS c ON i.ID_COURS = c.ID_COURS ";

    if (!searchStr.isEmpty()) {
        sql +=
            "WHERE (LOWER(s.NOM) LIKE LOWER(:search) "
            "   OR LOWER(s.PRENOM) LIKE LOWER(:search) "
            "   OR LOWER(s.EMAIL) LIKE LOWER(:search) "
            "   OR LOWER(s.TELEPHONE) LIKE LOWER(:search) "
            "   OR LOWER(c.NOM) LIKE LOWER(:search) "
            "   OR LOWER(i.STATUT) LIKE LOWER(:search) "
            "   OR TO_CHAR(i.ID_INSCRIPTION) LIKE :search "
            "   OR LOWER('AT-' || TO_CHAR(i.DATE_INSCRIPTION, 'YYYY') || '-' || LPAD(i.ID_INSCRIPTION, 5, '0')) LIKE LOWER(:search)) ";
    }

    // Construction sécurisée du ORDER BY multicritère avec tri logique pour Statut
    QStringList orderClauses;
    QSet<QString> colonnesUtilisees;
    for (const auto& pair : criteresTri) {
        QString champRaw = pair.first.trimmed();
        QString champLower = champRaw.toLower();
        QString ordreRaw = pair.second.trimmed();

        bool isDesc = (ordreRaw.toUpper() == "DESC" ||
                       ordreRaw.contains("Décroissant", Qt::CaseInsensitive) ||
                       ordreRaw.contains("Decroissant", Qt::CaseInsensitive) ||
                       ordreRaw.contains("Plus haute", Qt::CaseInsensitive) ||
                       ordreRaw.contains("Z → A", Qt::CaseInsensitive) ||
                       ordreRaw.contains("Annulé →", Qt::CaseInsensitive) ||
                       ordreRaw.contains("Plus récente", Qt::CaseInsensitive) ||
                       ordreRaw.contains("Plus récent", Qt::CaseInsensitive));
        QString dir = isDesc ? "DESC" : "ASC";

        if (champLower == "statut") {
            if (!colonnesUtilisees.contains("STATUT")) {
                colonnesUtilisees.insert("STATUT");
                // Tri logique : VALIDE (1) -> EN ATTENTE (2) -> ANNULE (3)
                orderClauses << QString("CASE UPPER(i.STATUT) WHEN 'VALIDE' THEN 1 WHEN 'VALIDEE' THEN 1 WHEN 'EN ATTENTE' THEN 2 WHEN 'ANNULE' THEN 3 WHEN 'ANNULEE' THEN 3 ELSE 4 END %1").arg(dir);
            }
        } else {
            QString colSql = mapCritereInscriptionSql(champRaw);
            if (!colSql.isEmpty() && !colonnesUtilisees.contains(colSql)) {
                colonnesUtilisees.insert(colSql);
                orderClauses << QString("%1 %2").arg(colSql, dir);
            }
        }
    }

    if (!orderClauses.isEmpty()) {
        sql += " ORDER BY " + orderClauses.join(", ");
    } else {
        sql += " ORDER BY i.ID_INSCRIPTION DESC";
    }

    QSqlQuery query(Connection::instance()->getDatabase());
    query.prepare(sql);
    if (!searchStr.isEmpty()) {
        query.bindValue(":search", "%" + searchStr + "%");
    }

    if (!query.exec()) {
        qDebug() << "Erreur recherche et tri inscription Oracle :" << query.lastError().text();
        return liste;
    }

    while (query.next()) {
        Inscription ins;
        ins.setIdInscription(query.value("ID_INSCRIPTION").toInt());
        ins.setIdStagiaire(query.value("ID_STAGIAIRE").toInt());
        ins.setIdCours(query.value("ID_COURS").toInt());
        ins.setDateInscription(query.value("DATE_INSCRIPTION").toDate());
        ins.setStatut(query.value("STATUT").toString());
        ins.setNote(query.value("NOTE").toDouble());
        ins.setNomStagiaire(query.value("STAGIAIRE_NOM").toString());
        ins.setNomCours(query.value("COURS_NOM").toString());

        liste.append(ins);
    }

    return liste;
}

// ============================================================
// OBTENIR INSCRIPTION PAR ID
// ============================================================

Inscription Inscription::getById(int idInscription)
{
    Inscription ins;
    if (idInscription <= 0) return ins;

    QSqlQuery query(Connection::instance()->getDatabase());
    query.prepare(
        "SELECT i.ID_INSCRIPTION, i.ID_STAGIAIRE, i.ID_COURS, i.DATE_INSCRIPTION, i.STATUT, i.NOTE, "
        "s.NOM || ' ' || s.PRENOM AS STAGIAIRE_NOM, c.NOM AS COURS_NOM "
        "FROM INSCRIPTION i "
        "JOIN STAGIAIRE s ON i.ID_STAGIAIRE = s.ID_STAGIAIRE "
        "JOIN COURS c ON i.ID_COURS = c.ID_COURS "
        "WHERE i.ID_INSCRIPTION = :id"
    );
    query.bindValue(":id", idInscription);

    if (query.exec() && query.next()) {
        ins.setIdInscription(query.value("ID_INSCRIPTION").toInt());
        ins.setIdStagiaire(query.value("ID_STAGIAIRE").toInt());
        ins.setIdCours(query.value("ID_COURS").toInt());
        ins.setDateInscription(query.value("DATE_INSCRIPTION").toDate());
        ins.setStatut(query.value("STATUT").toString());
        ins.setNote(query.value("NOTE").toDouble());
        ins.setNomStagiaire(query.value("STAGIAIRE_NOM").toString());
        ins.setNomCours(query.value("COURS_NOM").toString());
    }

    return ins;
}

// ============================================================
// OBTENIR TOUTES LES INSCRIPTIONS D'UN STAGIAIRE
// ============================================================

QList<Inscription> Inscription::readByStagiaire(int idStagiaire)
{
    QList<Inscription> liste;
    if (idStagiaire <= 0) return liste;

    QSqlQuery query(Connection::instance()->getDatabase());
    query.prepare(
        "SELECT i.ID_INSCRIPTION, i.ID_STAGIAIRE, i.ID_COURS, i.DATE_INSCRIPTION, i.STATUT, i.NOTE, "
        "s.NOM || ' ' || s.PRENOM AS STAGIAIRE_NOM, c.NOM AS COURS_NOM "
        "FROM INSCRIPTION i "
        "JOIN STAGIAIRE s ON i.ID_STAGIAIRE = s.ID_STAGIAIRE "
        "JOIN COURS c ON i.ID_COURS = c.ID_COURS "
        "WHERE i.ID_STAGIAIRE = :idStag "
        "ORDER BY i.DATE_INSCRIPTION DESC"
    );
    query.bindValue(":idStag", idStagiaire);

    if (query.exec()) {
        while (query.next()) {
            Inscription ins;
            ins.setIdInscription(query.value("ID_INSCRIPTION").toInt());
            ins.setIdStagiaire(query.value("ID_STAGIAIRE").toInt());
            ins.setIdCours(query.value("ID_COURS").toInt());
            ins.setDateInscription(query.value("DATE_INSCRIPTION").toDate());
            ins.setStatut(query.value("STATUT").toString());
            ins.setNote(query.value("NOTE").toDouble());
            ins.setNomStagiaire(query.value("STAGIAIRE_NOM").toString());
            ins.setNomCours(query.value("COURS_NOM").toString());
            liste.append(ins);
        }
    }

    return liste;
}

// ============================================================
// GENERATION DE DOCUMENT PDF (ATTESTATION DE FORMATION OFFICIELLE)
// ============================================================

bool Inscription::genererAttestationPdf(int idInscription, const QString& filePath, QString& messageErreur)
{
    if (idInscription <= 0) {
        messageErreur = "ID d'inscription invalide.";
        return false;
    }

    if (filePath.isEmpty()) {
        messageErreur = "Chemin de destination du PDF non spécifié.";
        return false;
    }

    Inscription ins = Inscription::getById(idInscription);
    if (ins.getIdInscription() <= 0) {
        messageErreur = "Inscription non trouvée dans Oracle.";
        return false;
    }

    if (ins.getStatut().toUpper() != "TERMINE") {
        messageErreur = QString("L'attestation de réussite ne peut être générée que si le statut de l'inscription est « TERMINE ».\nStatut actuel : %1")
                            .arg(ins.getStatut());
        return false;
    }

    Cours c = Cours::getById(ins.getIdCours());

    // Calcul de la mention d'évaluation et du statut officiel
    QString mention = "Admis";
    QString statutCertificat = "Formation Validée avec Succès";

    if (ins.getStatut().toUpper() == "ANNULEE" || ins.getStatut().toUpper() == "ANNULE") {
        statutCertificat = "Inscription Annulée";
        mention = "Non Validé";
    } else if (ins.getNote() >= 16.0) {
        mention = "Mention Très Bien";
        statutCertificat = "Formation Validée avec Succès";
    } else if (ins.getNote() >= 14.0) {
        mention = "Mention Bien";
        statutCertificat = "Formation Validée avec Succès";
    } else if (ins.getNote() >= 12.0) {
        mention = "Mention Assez Bien";
        statutCertificat = "Formation Validée avec Succès";
    } else if (ins.getNote() >= 10.0) {
        mention = "Admis";
        statutCertificat = "Formation Validée avec Succès";
    } else if (ins.getNote() > 0.0) {
        mention = "Ajourné";
        statutCertificat = "Attestation de Présence";
    } else {
        mention = "En Cours";
        statutCertificat = (ins.getStatut().isEmpty()) ? "Formation Validée" : ins.getStatut();
    }

    // Référence unique générée dynamiquement
    QString referenceDoc = QString("AT-%1-%2")
                               .arg(QDate::currentDate().year())
                               .arg(idInscription, 5, 10, QChar('0'));

    QString dateDebutStr = c.getDateDebut().isValid() ? c.getDateDebut().toString("dd/MM/yyyy") : ins.getDateInscription().toString("dd/MM/yyyy");
    QString dateFinStr = c.getDateFin().isValid() ? c.getDateFin().toString("dd/MM/yyyy") : ins.getDateInscription().toString("dd/MM/yyyy");
    QString dateJourStr = QDate::currentDate().toString("dd/MM/yyyy");
    QString nomStagiaireStr = ins.getNomStagiaire().isEmpty() ? "STAGIAIRE" : ins.getNomStagiaire().toUpper();
    QString nomCoursStr = ins.getNomCours().isEmpty() ? (c.getNom().isEmpty() ? "Formation Professionnelle" : c.getNom()) : ins.getNomCours();

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(filePath);
    printer.setPageSize(QPageSize::A4);
    printer.setPageOrientation(QPageLayout::Portrait);
    printer.setFullPage(true);

    QPainter painter;
    if (!painter.begin(&printer)) {
        messageErreur = "Impossible d'initialiser le moteur de rendu PDF vectoriel.";
        return false;
    }

    // Système de coordonnées normalisé 1000 x 1414 (Ratio A4 = 1 : 1.414)
    // Garantit une taille de police et des proportions parfaites quelle que soit la résolution
    painter.setWindow(0, 0, 1000, 1414);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    // Fond blanc
    painter.fillRect(0, 0, 1000, 1414, Qt::white);

    // 1. Cadre double élégant et fin
    painter.setPen(QPen(QColor("#0f172a"), 3));
    painter.drawRect(40, 40, 920, 1334);

    painter.setPen(QPen(QColor("#0284c7"), 1.5));
    painter.drawRect(50, 50, 900, 1314);

    // 2. En-tête Institutionnel
    QFont fHeader("Arial", 18, QFont::Bold);
    fHeader.setPixelSize(22);
    painter.setFont(fHeader);
    painter.setPen(QColor("#0f172a"));
    painter.drawText(QRect(70, 90, 860, 35), Qt::AlignCenter, "CENTRE DE FORMATION PROFESSIONNELLE");

    QFont fSubHeader("Arial", 11, QFont::DemiBold);
    fSubHeader.setPixelSize(13);
    painter.setFont(fSubHeader);
    painter.setPen(QColor("#0284c7"));
    painter.drawText(QRect(70, 130, 860, 22), Qt::AlignCenter, "Établissement Agréé d'Enseignement et de Formation Continue");

    painter.setPen(QPen(QColor("#cbd5e1"), 1.5));
    painter.drawLine(250, 170, 750, 170);

    // 3. Titre Principal
    QRect titleBox(100, 205, 800, 105);
    painter.setPen(QPen(QColor("#0284c7"), 2));
    painter.setBrush(QColor("#f8fafc"));
    painter.drawRoundedRect(titleBox, 8, 8);

    QFont fTitle("Arial", 22, QFont::Bold);
    fTitle.setPixelSize(28);
    fTitle.setLetterSpacing(QFont::AbsoluteSpacing, 2);
    painter.setFont(fTitle);
    painter.setPen(QColor("#0f172a"));
    painter.drawText(QRect(titleBox.left(), titleBox.top() + 16, titleBox.width(), 42), Qt::AlignCenter, "ATTESTATION DE FORMATION");

    QFont fCertSub("Arial", 10, QFont::Bold);
    fCertSub.setPixelSize(12);
    fCertSub.setLetterSpacing(QFont::AbsoluteSpacing, 1);
    painter.setFont(fCertSub);
    painter.setPen(QColor("#64748b"));
    painter.drawText(QRect(titleBox.left(), titleBox.top() + 64, titleBox.width(), 24), Qt::AlignCenter, "CERTIFICAT OFFICIEL DE RÉUSSITE & D'APTITUDE PROFESSIONNELLE");

    // 4. Texte Protocolaire
    QFont fBody("Arial", 12, QFont::Normal);
    fBody.setPixelSize(15);
    painter.setFont(fBody);
    painter.setPen(QColor("#475569"));
    painter.drawText(QRect(70, 345, 860, 30), Qt::AlignCenter, "La Direction Générale et le Comité Pédagogique certifient par la présente que :");

    // 5. Nom du Stagiaire (Mis en valeur)
    QFont fName("Arial", 24, QFont::Bold);
    fName.setPixelSize(30);
    fName.setLetterSpacing(QFont::AbsoluteSpacing, 2);
    painter.setFont(fName);
    painter.setPen(QColor("#0284c7"));
    painter.drawText(QRect(70, 395, 860, 48), Qt::AlignCenter, nomStagiaireStr);

    painter.setPen(QPen(QColor("#38bdf8"), 2));
    painter.drawLine(350, 455, 650, 455);

    // 6. Transition
    painter.setFont(fBody);
    painter.setPen(QColor("#475569"));
    painter.drawText(QRect(70, 485, 860, 30), Qt::AlignCenter, "a suivi avec assiduité et validé les épreuves du programme de formation intitulé :");

    // 7. Nom du Cours (Cartouche stylisé)
    QRect courseBox(100, 535, 800, 75);
    painter.setPen(QPen(QColor("#cbd5e1"), 1.5));
    painter.setBrush(QColor("#f1f5f9"));
    painter.drawRoundedRect(courseBox, 6, 6);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#0284c7"));
    painter.drawRoundedRect(QRect(courseBox.left(), courseBox.top(), 8, courseBox.height()), 3, 3);

    QFont fCourse("Arial", 16, QFont::Bold);
    fCourse.setPixelSize(22);
    painter.setFont(fCourse);
    painter.setPen(QColor("#0f172a"));
    painter.drawText(courseBox, Qt::AlignCenter, QString("« %1 »").arg(nomCoursStr));

    // 8. Tableau des Détails et Modalités (2x2)
    QFont fSec("Arial", 11, QFont::Bold);
    fSec.setPixelSize(14);
    painter.setFont(fSec);
    painter.setPen(QColor("#334155"));
    painter.drawText(QRect(80, 645, 840, 25), Qt::AlignLeft | Qt::AlignVCenter, "DÉTAILS ET MODALITÉS DE LA FORMATION");

    QRect gridRect(80, 680, 840, 190);
    painter.setPen(QPen(QColor("#cbd5e1"), 1.5));
    painter.setBrush(QColor("#ffffff"));
    painter.drawRect(gridRect);

    painter.drawLine(gridRect.left(), gridRect.top() + 95, gridRect.right(), gridRect.top() + 95);
    painter.drawLine(gridRect.left() + 420, gridRect.top(), gridRect.left() + 420, gridRect.bottom());

    painter.fillRect(gridRect.left() + 1, gridRect.top() + 1, 838, 93, QColor("#f8fafc"));

    QFont fLbl("Arial", 9, QFont::Bold);
    fLbl.setPixelSize(12);
    QFont fVal("Arial", 13, QFont::Bold);
    fVal.setPixelSize(17);

    // Cell (0,0) : Volume horaire
    painter.setFont(fLbl);
    painter.setPen(QColor("#64748b"));
    painter.drawText(QRect(100, 695, 380, 22), Qt::AlignLeft, "VOLUME HORAIRE :");
    painter.setFont(fVal);
    painter.setPen(QColor("#0f172a"));
    painter.drawText(QRect(100, 725, 380, 32), Qt::AlignLeft, QString("%1 heures d'enseignement").arg(c.getDuree()));

    // Cell (0,1) : Période
    painter.setFont(fLbl);
    painter.setPen(QColor("#64748b"));
    painter.drawText(QRect(520, 695, 380, 22), Qt::AlignLeft, "PÉRIODE DE FORMATION :");
    painter.setFont(fVal);
    painter.setPen(QColor("#0f172a"));
    painter.drawText(QRect(520, 725, 380, 32), Qt::AlignLeft, QString("Du %1 au %2").arg(dateDebutStr).arg(dateFinStr));

    // Cell (1,0) : Évaluation
    painter.setFont(fLbl);
    painter.setPen(QColor("#64748b"));
    painter.drawText(QRect(100, 790, 380, 22), Qt::AlignLeft, "ÉVALUATION FINALE :");
    painter.setFont(fVal);
    painter.setPen(QColor("#0284c7"));
    painter.drawText(QRect(100, 820, 380, 32), Qt::AlignLeft, QString("%1 / 20  (%2)").arg(QString::number(ins.getNote(), 'f', 1)).arg(mention));

    // Cell (1,1) : Statut
    painter.setFont(fLbl);
    painter.setPen(QColor("#64748b"));
    painter.drawText(QRect(520, 790, 380, 22), Qt::AlignLeft, "STATUT DU CERTIFICAT :");
    painter.setFont(fVal);
    painter.setPen(QColor("#059669"));
    painter.drawText(QRect(520, 820, 380, 32), Qt::AlignLeft, statutCertificat);

    // 9. Zone Basse : Signature, QR Code d'Authenticité & Cachet
    QFont fDate("Arial", 11, QFont::DemiBold);
    fDate.setPixelSize(15);
    painter.setFont(fDate);
    painter.setPen(QColor("#334155"));
    painter.drawText(QRect(80, 930, 270, 25), Qt::AlignLeft, QString("Fait le %1").arg(dateJourStr));

    QFont fRef("Arial", 10, QFont::Bold);
    fRef.setPixelSize(13);
    painter.setFont(fRef);
    painter.setPen(QColor("#64748b"));
    painter.drawText(QRect(80, 960, 270, 25), Qt::AlignLeft, QString("Réf : %1").arg(referenceDoc));

    painter.setPen(QPen(QColor("#cbd5e1"), 1.5));
    painter.drawLine(80, 1070, 310, 1070);
    QFont fSignP("Arial", 10, QFont::Bold);
    fSignP.setPixelSize(13);
    painter.setFont(fSignP);
    painter.setPen(QColor("#334155"));
    painter.drawText(QRect(80, 1080, 270, 25), Qt::AlignLeft, "Le Responsable Pédagogique");

    // Module Central : QR Code d'Authenticité Officiel (40-45 mm, Haute Résolution, Quiet Zone = 4)
    QRect qrBox(325, 895, 350, 235);
    painter.setPen(QPen(QColor("#0284c7"), 1.5));
    painter.setBrush(QColor("#f8fafc"));
    painter.drawRoundedRect(qrBox, 8, 8);

    QFont fQrTitle("Arial", 9, QFont::Bold);
    fQrTitle.setPixelSize(12);
    painter.setFont(fQrTitle);
    painter.setPen(QColor("#0284c7"));
    painter.drawText(QRect(qrBox.left(), qrBox.top() + 8, qrBox.width(), 18), Qt::AlignCenter, "VÉRIFICATION D'AUTHENTICITÉ");

    // 1. Récupération des informations dynamiques du stagiaire et de la formation
    Stagiaire stagiaireObj = Stagiaire::getById(ins.getIdStagiaire());
    QString nomStg = stagiaireObj.getNom().isEmpty() ? ins.getNomStagiaire() : stagiaireObj.getNom();
    QString prenomStg = stagiaireObj.getPrenom();
    QString statutDocAffiche = (ins.getStatut().toUpper() == "ANNULEE" || ins.getStatut().toUpper() == "ANNULE") ? "ANNULÉ" : "VALIDE";
    int dureeHeures = c.getDuree() > 0 ? c.getDuree() : 0;

    // 2. Construction du format compact officiel en une seule ligne pipe-délimitée
    QString qrData = CertificateVerificationService::formaterContenuQrCodeCompact(
        referenceDoc,
        prenomStg,
        nomStg,
        nomCoursStr,
        dateDebutStr,
        dateFinStr,
        dureeHeures,
        statutDocAffiche
    );

    // 3. Génération vectorielle 1000x1000 haute résolution du QR Code (appel unique)
    QImage qrImg = CertificateVerificationService::genererQrCodePourCertificat(qrData, 1000);
    if (!qrImg.isNull()) {
        painter.drawImage(QRect(qrBox.left() + (qrBox.width() - 134) / 2, qrBox.top() + 28, 134, 134), qrImg);
    }

    QFont fQrRef("Arial", 9, QFont::Bold);
    fQrRef.setPixelSize(12);
    painter.setFont(fQrRef);
    painter.setPen(QColor("#0f172a"));
    painter.drawText(QRect(qrBox.left(), qrBox.top() + 166, qrBox.width(), 20), Qt::AlignCenter, referenceDoc);

    QFont fQrScan("Arial", 8, QFont::Normal);
    fQrScan.setPixelSize(10);
    painter.setFont(fQrScan);
    painter.setPen(QColor("#64748b"));
    painter.drawText(QRect(qrBox.left() + 10, qrBox.top() + 188, qrBox.width() - 20, 26), Qt::AlignCenter | Qt::TextWordWrap, "Scannez avec un smartphone pour vérifier");

    // Direction & Cachet à droite
    QFont fDir("Arial", 11, QFont::Bold);
    fDir.setPixelSize(15);
    painter.setFont(fDir);
    painter.setPen(QColor("#0f172a"));
    painter.drawText(QRect(685, 930, 235, 25), Qt::AlignCenter, "LA DIRECTION");

    QRect stampBox(685, 965, 235, 125);
    painter.setPen(QPen(QColor("#0284c7"), 1.5));
    painter.setBrush(QColor("#ffffff"));
    painter.drawRoundedRect(stampBox, 8, 8);

    QFont fStmpT("Arial", 10, QFont::Bold);
    fStmpT.setPixelSize(13);
    painter.setFont(fStmpT);
    painter.setPen(QColor("#0284c7"));
    painter.drawText(QRect(stampBox.left(), stampBox.top() + 28, stampBox.width(), 22), Qt::AlignCenter, "CACHET & SIGNATURE");

    QFont fStmpS("Arial", 8, QFont::Normal);
    fStmpS.setPixelSize(11);
    painter.setFont(fStmpS);
    painter.setPen(QColor("#64748b"));
    painter.drawText(QRect(stampBox.left(), stampBox.top() + 65, stampBox.width(), 20), Qt::AlignCenter, "Direction des Études");

    // 10. Pied de Page (Footer)
    painter.setPen(QPen(QColor("#e2e8f0"), 1.5));
    painter.drawLine(80, 1290, 920, 1290);

    QFont fFoot("Arial", 8, QFont::Normal);
    fFoot.setPixelSize(11);
    painter.setFont(fFoot);
    painter.setPen(QColor("#94a3b8"));
    painter.drawText(QRect(80, 1305, 840, 25), Qt::AlignCenter,
                     QString("CENTRE DE FORMATION PROFESSIONNELLE — Document officiel — %1").arg(referenceDoc));

    painter.end();

    return true;
}
