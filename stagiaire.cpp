#include "stagiaire.h"
#include "connection.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>
#include <QRegularExpression>

// ============================================================
// Constructeur par défaut
// ============================================================

Stagiaire::Stagiaire()
    : idStagiaire(0)
{
}

// ============================================================
// Constructeur paramétré
// ============================================================

Stagiaire::Stagiaire(
    int idStagiaire,
    const QString& nom,
    const QString& prenom,
    const QString& email,
    const QString& telephone,
    const QDate& dateNaissance,
    const QString& niveau,
    const QString& formation)
    : idStagiaire(idStagiaire),
    nom(nom),
    prenom(prenom),
    email(email),
    telephone(telephone),
    dateNaissance(dateNaissance),
    niveau(niveau),
    formation(formation)
{
}

// ============================================================
// GETTERS
// ============================================================

int Stagiaire::getIdStagiaire() const
{
    return idStagiaire;
}

QString Stagiaire::getNom() const
{
    return nom;
}

QString Stagiaire::getPrenom() const
{
    return prenom;
}

QString Stagiaire::getEmail() const
{
    return email;
}

QString Stagiaire::getTelephone() const
{
    return telephone;
}

QDate Stagiaire::getDateNaissance() const
{
    return dateNaissance;
}

QString Stagiaire::getNiveau() const
{
    return niveau;
}

QString Stagiaire::getFormation() const
{
    return formation;
}

// ============================================================
// SETTERS
// ============================================================

void Stagiaire::setIdStagiaire(int id)
{
    idStagiaire = id;
}

void Stagiaire::setNom(const QString& value)
{
    nom = value;
}

void Stagiaire::setPrenom(const QString& value)
{
    prenom = value;
}

void Stagiaire::setEmail(const QString& value)
{
    email = value;
}

void Stagiaire::setTelephone(const QString& value)
{
    telephone = value;
}

void Stagiaire::setDateNaissance(const QDate& value)
{
    dateNaissance = value;
}

void Stagiaire::setNiveau(const QString& value)
{
    niveau = value;
}

void Stagiaire::setFormation(const QString& value)
{
    formation = value;
}

// ============================================================
// VALIDATION
// ============================================================

bool Stagiaire::estValide(QString& erreur) const
{
    if (nom.trimmed().isEmpty()) {
        erreur = "Le nom est obligatoire.";
        return false;
    }

    if (prenom.trimmed().isEmpty()) {
        erreur = "Le prénom est obligatoire.";
        return false;
    }

    if (email.trimmed().isEmpty()) {
        erreur = "L'email est obligatoire.";
        return false;
    }

    if (!email.contains('@') || !email.contains('.')) {
        erreur = "L'email est invalide.";
        return false;
    }

    if (telephone.trimmed().isEmpty()) {
        erreur = "Le numéro de téléphone est obligatoire.";
        return false;
    }

    static const QRegularExpression telRegex("^[0-9]{8}$");
    if (!telRegex.match(telephone.trimmed()).hasMatch()) {
        erreur = "Le numéro de téléphone est invalide : il doit contenir exactement 8 chiffres (uniquement des nombres).";
        return false;
    }

    if (niveau != "DEBUTANT" &&
        niveau != "INTERMEDIAIRE" &&
        niveau != "AVANCE") {
        erreur = "Le niveau est invalide.";
        return false;
    }

    if (formation.trimmed().isEmpty()) {
        erreur = "La formation est obligatoire.";
        return false;
    }

    if (dateNaissance.isValid() &&
        dateNaissance > QDate::currentDate()) {
        erreur = "La date de naissance est invalide.";
        return false;
    }

    return true;
}

// ============================================================
// AJOUTER
// ============================================================

bool Stagiaire::ajouter()
{
    QString erreur;

    if (!estValide(erreur)) {
        qDebug() << "Validation :" << erreur;
        return false;
    }

    QSqlQuery query(Connection::instance()->getDatabase());

    query.prepare(
        "INSERT INTO STAGIAIRE "
        "(NOM, PRENOM, EMAIL, TELEPHONE, DATE_NAISSANCE, "
        "NIVEAU, FORMATION) "
        "VALUES "
        "(:nom, :prenom, :email, :telephone, :dateNaissance, "
        ":niveau, :formation)"
        );

    query.bindValue(":nom", nom);
    query.bindValue(":prenom", prenom);
    query.bindValue(":email", email);
    query.bindValue(":telephone", telephone);

    if (dateNaissance.isValid())
        query.bindValue(":dateNaissance", dateNaissance);
    else
        query.bindValue(":dateNaissance", QVariant(QMetaType(QMetaType::QDate)));

    query.bindValue(":niveau", niveau);
    query.bindValue(":formation", formation);

    if (!query.exec()) {
        qDebug() << "Erreur ajout stagiaire :"
                 << query.lastError().text();
        return false;
    }

    return true;
}

// ============================================================
// MODIFIER
// ============================================================

bool Stagiaire::modifier()
{
    if (idStagiaire <= 0) {
        qDebug() << "ID stagiaire invalide.";
        return false;
    }

    QString erreur;

    if (!estValide(erreur)) {
        qDebug() << "Validation :" << erreur;
        return false;
    }

    QSqlQuery query(Connection::instance()->getDatabase());

    query.prepare(
        "UPDATE STAGIAIRE SET "
        "NOM = :nom, "
        "PRENOM = :prenom, "
        "EMAIL = :email, "
        "TELEPHONE = :telephone, "
        "DATE_NAISSANCE = :dateNaissance, "
        "NIVEAU = :niveau, "
        "FORMATION = :formation "
        "WHERE ID_STAGIAIRE = :id"
        );

    query.bindValue(":nom", nom);
    query.bindValue(":prenom", prenom);
    query.bindValue(":email", email);
    query.bindValue(":telephone", telephone);
    query.bindValue(":dateNaissance", dateNaissance);
    query.bindValue(":niveau", niveau);
    query.bindValue(":formation", formation);
    query.bindValue(":id", idStagiaire);

    if (!query.exec()) {
        qDebug() << "Erreur modification stagiaire :"
                 << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

// ============================================================
// SUPPRIMER
// ============================================================

bool Stagiaire::supprimer()
{
    if (idStagiaire <= 0) {
        qDebug() << "ID stagiaire invalide.";
        return false;
    }

    QSqlQuery query(Connection::instance()->getDatabase());

    query.prepare(
        "DELETE FROM STAGIAIRE "
        "WHERE ID_STAGIAIRE = :id"
        );

    query.bindValue(":id", idStagiaire);

    if (!query.exec()) {
        qDebug() << "Erreur suppression stagiaire :"
                 << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

// ============================================================
// AFFICHER
// ============================================================

QList<Stagiaire> Stagiaire::afficher()
{
    QList<Stagiaire> liste;

    QSqlQuery query(Connection::instance()->getDatabase());

    query.prepare(
        "SELECT ID_STAGIAIRE, NOM, PRENOM, EMAIL, TELEPHONE, "
        "DATE_NAISSANCE, NIVEAU, FORMATION "
        "FROM STAGIAIRE "
        "ORDER BY ID_STAGIAIRE"
        );

    if (!query.exec()) {
        qDebug() << "Erreur affichage stagiaires :"
                 << query.lastError().text();
        return liste;
    }

    while (query.next()) {

        Stagiaire s;

        s.setIdStagiaire(
            query.value("ID_STAGIAIRE").toInt());

        s.setNom(
            query.value("NOM").toString());

        s.setPrenom(
            query.value("PRENOM").toString());

        s.setEmail(
            query.value("EMAIL").toString());

        s.setTelephone(
            query.value("TELEPHONE").toString());

        s.setDateNaissance(
            query.value("DATE_NAISSANCE").toDate());

        s.setNiveau(
            query.value("NIVEAU").toString());

        s.setFormation(
            query.value("FORMATION").toString());

        liste.append(s);
    }

    return liste;
}

// ============================================================
// RECHERCHE MULTICRITÈRE
// Critères : nom + formation + niveau
// ============================================================

QList<Stagiaire> Stagiaire::rechercher(
    const QString& nom,
    const QString& formation,
    const QString& niveau)
{
    QList<Stagiaire> liste;

    QString sql =
        "SELECT ID_STAGIAIRE, NOM, PRENOM, EMAIL, TELEPHONE, "
        "DATE_NAISSANCE, NIVEAU, FORMATION "
        "FROM STAGIAIRE "
        "WHERE 1 = 1";

    if (!nom.trimmed().isEmpty())
        sql += " AND UPPER(NOM) LIKE UPPER(:nom)";

    if (!formation.trimmed().isEmpty())
        sql += " AND FORMATION = :formation";

    if (!niveau.trimmed().isEmpty())
        sql += " AND NIVEAU = :niveau";

    sql += " ORDER BY NOM, PRENOM";

    QSqlQuery query(Connection::instance()->getDatabase());

    query.prepare(sql);

    if (!nom.trimmed().isEmpty())
        query.bindValue(":nom", "%" + nom.trimmed() + "%");

    if (!formation.trimmed().isEmpty())
        query.bindValue(":formation", formation);

    if (!niveau.trimmed().isEmpty())
        query.bindValue(":niveau", niveau);

    if (!query.exec()) {
        qDebug() << "Erreur recherche stagiaires :"
                 << query.lastError().text();
        return liste;
    }

    while (query.next()) {

        Stagiaire s;

        s.setIdStagiaire(
            query.value("ID_STAGIAIRE").toInt());

        s.setNom(
            query.value("NOM").toString());

        s.setPrenom(
            query.value("PRENOM").toString());

        s.setEmail(
            query.value("EMAIL").toString());

        s.setTelephone(
            query.value("TELEPHONE").toString());

        s.setDateNaissance(
            query.value("DATE_NAISSANCE").toDate());

        s.setNiveau(
            query.value("NIVEAU").toString());

        s.setFormation(
            query.value("FORMATION").toString());

        liste.append(s);
    }

    return liste;
}

// ============================================================
// GET BY ID
// ============================================================

Stagiaire Stagiaire::getById(int idStagiaire)
{
    Stagiaire s;
    if (idStagiaire <= 0) return s;

    QSqlQuery query(Connection::instance()->getDatabase());
    query.prepare("SELECT * FROM STAGIAIRE WHERE ID_STAGIAIRE = :id");
    query.bindValue(":id", idStagiaire);

    if (query.exec() && query.next()) {
        s.setIdStagiaire(query.value("ID_STAGIAIRE").toInt());
        s.setNom(query.value("NOM").toString());
        s.setPrenom(query.value("PRENOM").toString());
        s.setEmail(query.value("EMAIL").toString());
        s.setTelephone(query.value("TELEPHONE").toString());
        s.setDateNaissance(query.value("DATE_NAISSANCE").toDate());
        s.setNiveau(query.value("NIVEAU").toString());
        s.setFormation(query.value("FORMATION").toString());
    }

    return s;
}

// ============================================================
// RECHERCHE GLOBALE & TRI MULTICRITÈRE (DYNAMIQUE ORACLE)
// ============================================================

static QString mapCritereStagiaireSql(const QString& champ)
{
    QString c = champ.trimmed().toLower();
    if (c == "nom" || c == "nom stagiaire") return "NOM";
    if (c == "prénom" || c == "prenom") return "PRENOM";
    if (c == "email") return "EMAIL";
    if (c == "date de naissance" || c == "date_naissance" || c == "date naissance") return "DATE_NAISSANCE";
    if (c == "niveau") return "NIVEAU";
    if (c == "formation") return "FORMATION";
    if (c == "id" || c == "id_stagiaire") return "ID_STAGIAIRE";
    return "";
}

QList<Stagiaire> Stagiaire::rechercherGlobale(const QString& texte)
{
    return rechercherEtTrier(texte, {});
}

QList<Stagiaire> Stagiaire::rechercherEtTrier(const QString& search, const QList<QPair<QString, QString>>& criteresTri)
{
    QList<Stagiaire> liste;
    QString searchStr = search.trimmed();

    QString sql =
        "SELECT ID_STAGIAIRE, NOM, PRENOM, EMAIL, TELEPHONE, "
        "DATE_NAISSANCE, NIVEAU, FORMATION "
        "FROM STAGIAIRE ";

    if (!searchStr.isEmpty()) {
        sql +=
            "WHERE (LOWER(NOM) LIKE LOWER(:search) "
            "   OR LOWER(PRENOM) LIKE LOWER(:search) "
            "   OR LOWER(EMAIL) LIKE LOWER(:search) "
            "   OR LOWER(TELEPHONE) LIKE LOWER(:search) "
            "   OR LOWER(FORMATION) LIKE LOWER(:search) "
            "   OR LOWER(NIVEAU) LIKE LOWER(:search) "
            "   OR TO_CHAR(DATE_NAISSANCE, 'DD/MM/YYYY') LIKE :search "
            "   OR TO_CHAR(DATE_NAISSANCE, 'YYYY-MM-DD') LIKE :search "
            "   OR TO_CHAR(ID_STAGIAIRE) LIKE :search) ";
    }

    // Construction sécurisée du ORDER BY multicritère avec tri logique pour Niveau
    QStringList orderClauses;
    QSet<QString> colonnesUtilisees;
    for (const auto& pair : criteresTri) {
        QString champRaw = pair.first.trimmed();
        QString champLower = champRaw.toLower();
        QString ordreRaw = pair.second.trimmed();

        bool isDesc = (ordreRaw.toUpper() == "DESC" ||
                       ordreRaw.contains("Décroissant", Qt::CaseInsensitive) ||
                       ordreRaw.contains("Decroissant", Qt::CaseInsensitive) ||
                       ordreRaw.contains("Avancé →", Qt::CaseInsensitive) ||
                       ordreRaw.contains("Z → A", Qt::CaseInsensitive) ||
                       ordreRaw.contains("Plus récent", Qt::CaseInsensitive));
        QString dir = isDesc ? "DESC" : "ASC";

        if (champLower == "niveau") {
            if (!colonnesUtilisees.contains("NIVEAU")) {
                colonnesUtilisees.insert("NIVEAU");
                // Tri logique : DEBUTANT (1) -> INTERMEDIAIRE (2) -> AVANCE (3)
                orderClauses << QString("CASE UPPER(NIVEAU) WHEN 'DEBUTANT' THEN 1 WHEN 'INTERMEDIAIRE' THEN 2 WHEN 'AVANCE' THEN 3 ELSE 4 END %1").arg(dir);
            }
        } else {
            QString colSql = mapCritereStagiaireSql(champRaw);
            if (!colSql.isEmpty() && !colonnesUtilisees.contains(colSql)) {
                colonnesUtilisees.insert(colSql);
                orderClauses << QString("%1 %2").arg(colSql, dir);
            }
        }
    }

    if (!orderClauses.isEmpty()) {
        sql += " ORDER BY " + orderClauses.join(", ");
    } else {
        sql += " ORDER BY NOM, PRENOM";
    }

    QSqlQuery query(Connection::instance()->getDatabase());
    query.prepare(sql);
    if (!searchStr.isEmpty()) {
        query.bindValue(":search", "%" + searchStr + "%");
    }

    if (!query.exec()) {
        qDebug() << "Erreur recherche et tri stagiaires Oracle :" << query.lastError().text();
        return liste;
    }

    while (query.next()) {
        Stagiaire s;
        s.setIdStagiaire(query.value("ID_STAGIAIRE").toInt());
        s.setNom(query.value("NOM").toString());
        s.setPrenom(query.value("PRENOM").toString());
        s.setEmail(query.value("EMAIL").toString());
        s.setTelephone(query.value("TELEPHONE").toString());
        s.setDateNaissance(query.value("DATE_NAISSANCE").toDate());
        s.setNiveau(query.value("NIVEAU").toString());
        s.setFormation(query.value("FORMATION").toString());
        liste.append(s);
    }

    return liste;
}
