#include "cours.h"
#include "connection.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>

// ============================================================
// Constructeurs
// ============================================================

Cours::Cours()
    : idCours(0), duree(0), capacite(0), prix(0.0)
{
}

Cours::Cours(int idCours,
             const QString& nom,
             const QString& description,
             const QDate& dateDebut,
             const QDate& dateFin,
             int duree,
             int capacite,
             double prix,
             const QString& statut)
    : idCours(idCours),
      nom(nom),
      description(description),
      dateDebut(dateDebut),
      dateFin(dateFin),
      duree(duree),
      capacite(capacite),
      prix(prix),
      statut(statut)
{
}

// ============================================================
// GETTERS
// ============================================================

int Cours::getIdCours() const { return idCours; }
QString Cours::getNom() const { return nom; }
QString Cours::getDescription() const { return description; }
QDate Cours::getDateDebut() const { return dateDebut; }
QDate Cours::getDateFin() const { return dateFin; }
int Cours::getDuree() const { return duree; }
int Cours::getCapacite() const { return capacite; }
double Cours::getPrix() const { return prix; }
QString Cours::getStatut() const { return statut; }

// ============================================================
// SETTERS
// ============================================================

void Cours::setIdCours(int id) { idCours = id; }
void Cours::setNom(const QString& value) { nom = value; }
void Cours::setDescription(const QString& value) { description = value; }
void Cours::setDateDebut(const QDate& value) { dateDebut = value; }
void Cours::setDateFin(const QDate& value) { dateFin = value; }
void Cours::setDuree(int value) { duree = value; }
void Cours::setCapacite(int value) { capacite = value; }
void Cours::setPrix(double value) { prix = value; }
void Cours::setStatut(const QString& value) { statut = value; }

// ============================================================
// VALIDATION
// ============================================================

bool Cours::estValide(QString& erreur) const
{
    if (nom.trimmed().isEmpty()) {
        erreur = "Le nom du cours est obligatoire.";
        return false;
    }

    if (!dateDebut.isValid() || !dateFin.isValid()) {
        erreur = "Les dates de début et de fin doivent être valides.";
        return false;
    }

    if (dateDebut > dateFin) {
        erreur = "La date de début ne peut pas être postérieure à la date de fin.";
        return false;
    }

    if (duree <= 0) {
        erreur = "La durée doit être un nombre d'heures supérieur à 0.";
        return false;
    }

    if (capacite <= 0) {
        erreur = "La capacité doit être au moins de 1 personne.";
        return false;
    }

    if (prix < 0) {
        erreur = "Le prix ne peut pas être négatif.";
        return false;
    }

    if (statut != "PLANIFIE" &&
        statut != "EN_COURS" &&
        statut != "TERMINE" &&
        statut != "ANNULE") {
        erreur = "Le statut du cours est invalide.";
        return false;
    }

    return true;
}

// ============================================================
// AJOUTER
// ============================================================

bool Cours::ajouter()
{
    QString erreur;
    if (!estValide(erreur)) {
        qDebug() << "Validation Cours :" << erreur;
        return false;
    }

    QSqlQuery query(Connection::instance()->getDatabase());

    query.prepare(
        "INSERT INTO COURS "
        "(NOM, DESCRIPTION, DATE_DEBUT, DATE_FIN, DUREE, CAPACITE, PRIX, STATUT) "
        "VALUES "
        "(:nom, :description, :dateDebut, :dateFin, :duree, :capacite, :prix, :statut)"
    );

    query.bindValue(":nom", nom);
    query.bindValue(":description", description);
    query.bindValue(":dateDebut", dateDebut);
    query.bindValue(":dateFin", dateFin);
    query.bindValue(":duree", duree);
    query.bindValue(":capacite", capacite);
    query.bindValue(":prix", prix);
    query.bindValue(":statut", statut);

    if (!query.exec()) {
        qDebug() << "Erreur ajout cours Oracle :" << query.lastError().text();
        return false;
    }

    return true;
}

// ============================================================
// MODIFIER
// ============================================================

bool Cours::modifier()
{
    if (idCours <= 0) {
        qDebug() << "ID cours invalide.";
        return false;
    }

    QString erreur;
    if (!estValide(erreur)) {
        qDebug() << "Validation Cours :" << erreur;
        return false;
    }

    QSqlQuery query(Connection::instance()->getDatabase());

    query.prepare(
        "UPDATE COURS SET "
        "NOM = :nom, "
        "DESCRIPTION = :description, "
        "DATE_DEBUT = :dateDebut, "
        "DATE_FIN = :dateFin, "
        "DUREE = :duree, "
        "CAPACITE = :capacite, "
        "PRIX = :prix, "
        "STATUT = :statut "
        "WHERE ID_COURS = :id"
    );

    query.bindValue(":nom", nom);
    query.bindValue(":description", description);
    query.bindValue(":dateDebut", dateDebut);
    query.bindValue(":dateFin", dateFin);
    query.bindValue(":duree", duree);
    query.bindValue(":capacite", capacite);
    query.bindValue(":prix", prix);
    query.bindValue(":statut", statut);
    query.bindValue(":id", idCours);

    if (!query.exec()) {
        qDebug() << "Erreur modification cours Oracle :" << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

// ============================================================
// SUPPRIMER
// ============================================================

bool Cours::supprimer()
{
    if (idCours <= 0) {
        qDebug() << "ID cours invalide.";
        return false;
    }

    QSqlQuery query(Connection::instance()->getDatabase());

    query.prepare("DELETE FROM COURS WHERE ID_COURS = :id");
    query.bindValue(":id", idCours);

    if (!query.exec()) {
        qDebug() << "Erreur suppression cours Oracle :" << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

// ============================================================
// AFFICHER
// ============================================================

QList<Cours> Cours::afficher()
{
    QList<Cours> liste;
    QSqlQuery query(Connection::instance()->getDatabase());

    query.prepare(
        "SELECT ID_COURS, NOM, DESCRIPTION, DATE_DEBUT, DATE_FIN, "
        "DUREE, CAPACITE, PRIX, STATUT "
        "FROM COURS "
        "ORDER BY ID_COURS"
    );

    if (!query.exec()) {
        qDebug() << "Erreur affichage cours Oracle :" << query.lastError().text();
        return liste;
    }

    while (query.next()) {
        Cours c;
        c.setIdCours(query.value("ID_COURS").toInt());
        c.setNom(query.value("NOM").toString());
        c.setDescription(query.value("DESCRIPTION").toString());
        c.setDateDebut(query.value("DATE_DEBUT").toDate());
        c.setDateFin(query.value("DATE_FIN").toDate());
        c.setDuree(query.value("DUREE").toInt());
        c.setCapacite(query.value("CAPACITE").toInt());
        c.setPrix(query.value("PRIX").toDouble());
        c.setStatut(query.value("STATUT").toString());

        liste.append(c);
    }

    return liste;
}

// ============================================================
// RECHERCHER
// ============================================================

QList<Cours> Cours::rechercher(const QString& nom, const QString& statut)
{
    QList<Cours> liste;
    QString sql = "SELECT ID_COURS, NOM, DESCRIPTION, DATE_DEBUT, DATE_FIN, "
                  "DUREE, CAPACITE, PRIX, STATUT FROM COURS WHERE 1=1";

    if (!nom.trimmed().isEmpty())
        sql += " AND UPPER(NOM) LIKE UPPER(:nom)";

    if (!statut.trimmed().isEmpty())
        sql += " AND STATUT = :statut";

    sql += " ORDER BY DATE_DEBUT DESC, NOM";

    QSqlQuery query(Connection::instance()->getDatabase());
    query.prepare(sql);

    if (!nom.trimmed().isEmpty())
        query.bindValue(":nom", "%" + nom.trimmed() + "%");

    if (!statut.trimmed().isEmpty())
        query.bindValue(":statut", statut);

    if (!query.exec()) {
        qDebug() << "Erreur recherche cours Oracle :" << query.lastError().text();
        return liste;
    }

    while (query.next()) {
        Cours c;
        c.setIdCours(query.value("ID_COURS").toInt());
        c.setNom(query.value("NOM").toString());
        c.setDescription(query.value("DESCRIPTION").toString());
        c.setDateDebut(query.value("DATE_DEBUT").toDate());
        c.setDateFin(query.value("DATE_FIN").toDate());
        c.setDuree(query.value("DUREE").toInt());
        c.setCapacite(query.value("CAPACITE").toInt());
        c.setPrix(query.value("PRIX").toDouble());
        c.setStatut(query.value("STATUT").toString());

        liste.append(c);
    }

    return liste;
}

// ============================================================
// METIER - COMPTER INSCRIPTIONS ACTUELLES
// ============================================================

int Cours::compterInscriptionsActuelles(int idCours)
{
    if (idCours <= 0) return 0;

    QSqlQuery query(Connection::instance()->getDatabase());
    query.prepare("SELECT COUNT(*) FROM INSCRIPTION WHERE ID_COURS = :id AND STATUT != 'ABANDONNE'");
    query.bindValue(":id", idCours);

    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }

    return 0;
}

// ============================================================
// OBTENIR UN COURS PAR ID
// ============================================================

Cours Cours::getById(int idCours)
{
    Cours c;
    if (idCours <= 0) return c;

    QSqlQuery query(Connection::instance()->getDatabase());
    query.prepare("SELECT ID_COURS, NOM, DESCRIPTION, DATE_DEBUT, DATE_FIN, DUREE, CAPACITE, PRIX, STATUT FROM COURS WHERE ID_COURS = :id");
    query.bindValue(":id", idCours);

    if (query.exec() && query.next()) {
        c.setIdCours(query.value("ID_COURS").toInt());
        c.setNom(query.value("NOM").toString());
        c.setDescription(query.value("DESCRIPTION").toString());
        c.setDateDebut(query.value("DATE_DEBUT").toDate());
        c.setDateFin(query.value("DATE_FIN").toDate());
        c.setDuree(query.value("DUREE").toInt());
        c.setCapacite(query.value("CAPACITE").toInt());
        c.setPrix(query.value("PRIX").toDouble());
        c.setStatut(query.value("STATUT").toString());
    }

    return c;
}

// ============================================================
// RECHERCHE GLOBALE & TRI MULTICRITÈRE (DYNAMIQUE ORACLE)
// ============================================================

static QString mapCritereCoursSql(const QString& champ)
{
    QString c = champ.trimmed().toLower();
    if (c == "nom" || c == "nom cours") return "NOM";
    if (c == "description") return "DESCRIPTION";
    if (c == "date début" || c == "date debut" || c == "date_debut") return "DATE_DEBUT";
    if (c == "date fin" || c == "date_fin") return "DATE_FIN";
    if (c == "durée" || c == "duree") return "DUREE";
    if (c == "capacité" || c == "capacite") return "CAPACITE";
    if (c == "prix") return "PRIX";
    if (c == "statut") return "STATUT";
    if (c == "id" || c == "id_cours") return "ID_COURS";
    return "";
}

QList<Cours> Cours::rechercherGlobale(const QString& texte)
{
    return rechercherEtTrier(texte, {});
}

QList<Cours> Cours::rechercherEtTrier(const QString& search, const QList<QPair<QString, QString>>& criteresTri)
{
    QList<Cours> liste;
    QString searchStr = search.trimmed();

    QString sql =
        "SELECT ID_COURS, NOM, DESCRIPTION, DATE_DEBUT, DATE_FIN, "
        "DUREE, CAPACITE, PRIX, STATUT "
        "FROM COURS ";

    if (!searchStr.isEmpty()) {
        sql +=
            "WHERE (LOWER(NOM) LIKE LOWER(:search) "
            "   OR LOWER(DESCRIPTION) LIKE LOWER(:search) "
            "   OR LOWER(STATUT) LIKE LOWER(:search) "
            "   OR TO_CHAR(DATE_DEBUT, 'DD/MM/YYYY') LIKE :search "
            "   OR TO_CHAR(DATE_FIN, 'DD/MM/YYYY') LIKE :search "
            "   OR TO_CHAR(DATE_DEBUT, 'YYYY-MM-DD') LIKE :search "
            "   OR TO_CHAR(DATE_FIN, 'YYYY-MM-DD') LIKE :search "
            "   OR TO_CHAR(DUREE) LIKE :search "
            "   OR TO_CHAR(CAPACITE) LIKE :search "
            "   OR TO_CHAR(PRIX) LIKE :search "
            "   OR TO_CHAR(ID_COURS) LIKE :search) ";
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
                       ordreRaw.contains("Plus cher", Qt::CaseInsensitive) ||
                       ordreRaw.contains("Plus long", Qt::CaseInsensitive) ||
                       ordreRaw.contains("Grande →", Qt::CaseInsensitive) ||
                       ordreRaw.contains("Z → A", Qt::CaseInsensitive) ||
                       ordreRaw.contains("Annulé →", Qt::CaseInsensitive) ||
                       ordreRaw.contains("Plus récent", Qt::CaseInsensitive));
        QString dir = isDesc ? "DESC" : "ASC";

        if (champLower == "statut") {
            if (!colonnesUtilisees.contains("STATUT")) {
                colonnesUtilisees.insert("STATUT");
                // Tri logique : PLANIFIE (1) -> EN COURS (2) -> TERMINE (3) -> ANNULE (4)
                orderClauses << QString("CASE UPPER(STATUT) WHEN 'PLANIFIE' THEN 1 WHEN 'EN COURS' THEN 2 WHEN 'TERMINE' THEN 3 WHEN 'ANNULE' THEN 4 WHEN 'ANNULEE' THEN 4 ELSE 5 END %1").arg(dir);
            }
        } else {
            QString colSql = mapCritereCoursSql(champRaw);
            if (!colSql.isEmpty() && !colonnesUtilisees.contains(colSql)) {
                colonnesUtilisees.insert(colSql);
                orderClauses << QString("%1 %2").arg(colSql, dir);
            }
        }
    }

    if (!orderClauses.isEmpty()) {
        sql += " ORDER BY " + orderClauses.join(", ");
    } else {
        sql += " ORDER BY ID_COURS";
    }

    QSqlQuery query(Connection::instance()->getDatabase());
    query.prepare(sql);
    if (!searchStr.isEmpty()) {
        query.bindValue(":search", "%" + searchStr + "%");
    }

    if (!query.exec()) {
        qDebug() << "Erreur recherche et tri cours Oracle :" << query.lastError().text();
        return liste;
    }

    while (query.next()) {
        Cours c;
        c.setIdCours(query.value("ID_COURS").toInt());
        c.setNom(query.value("NOM").toString());
        c.setDescription(query.value("DESCRIPTION").toString());
        c.setDateDebut(query.value("DATE_DEBUT").toDate());
        c.setDateFin(query.value("DATE_FIN").toDate());
        c.setDuree(query.value("DUREE").toInt());
        c.setCapacite(query.value("CAPACITE").toInt());
        c.setPrix(query.value("PRIX").toDouble());
        c.setStatut(query.value("STATUT").toString());
        liste.append(c);
    }

    return liste;
}

