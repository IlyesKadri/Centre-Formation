#include "statistiques.h"
#include "connection.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>

// ============================================================
// INDICATEURS CLES (KPI)
// ============================================================

int StatistiquesService::getTotalStagiaires()
{
    QSqlQuery q("SELECT COUNT(*) FROM STAGIAIRE", Connection::instance()->getDatabase());
    if (q.exec() && q.next()) return q.value(0).toInt();
    return 0;
}

int StatistiquesService::getTotalCours()
{
    QSqlQuery q("SELECT COUNT(*) FROM COURS", Connection::instance()->getDatabase());
    if (q.exec() && q.next()) return q.value(0).toInt();
    return 0;
}

int StatistiquesService::getTotalInscriptions()
{
    QSqlQuery q("SELECT COUNT(*) FROM INSCRIPTION", Connection::instance()->getDatabase());
    if (q.exec() && q.next()) return q.value(0).toInt();
    return 0;
}

int StatistiquesService::getCoursDisponibles()
{
    QSqlQuery q("SELECT COUNT(*) FROM COURS WHERE STATUT IN ('PLANIFIE', 'EN_COURS')", Connection::instance()->getDatabase());
    if (q.exec() && q.next()) return q.value(0).toInt();
    return 0;
}

int StatistiquesService::getStagiairesActifs()
{
    QSqlQuery q("SELECT COUNT(*) FROM STAGIAIRE WHERE STATUT = 'ACTIF'", Connection::instance()->getDatabase());
    if (q.exec() && q.next()) return q.value(0).toInt();
    return 0;
}

int StatistiquesService::getCoursComplets()
{
    QSqlQuery q(
        "SELECT COUNT(*) FROM ("
        "  SELECT c.ID_COURS, c.CAPACITE, COUNT(i.ID_INSCRIPTION) AS NB "
        "  FROM COURS c "
        "  JOIN INSCRIPTION i ON c.ID_COURS = i.ID_COURS "
        "  WHERE i.STATUT != 'ABANDONNE' "
        "  GROUP BY c.ID_COURS, c.CAPACITE "
        "  HAVING COUNT(i.ID_INSCRIPTION) >= c.CAPACITE"
        ")",
        Connection::instance()->getDatabase()
    );
    if (q.exec() && q.next()) return q.value(0).toInt();
    return 0;
}

double StatistiquesService::getMoyenneNotes()
{
    QSqlQuery q("SELECT NVL(AVG(NOTE), 0) FROM INSCRIPTION WHERE NOTE IS NOT NULL", Connection::instance()->getDatabase());
    if (q.exec() && q.next()) return q.value(0).toDouble();
    qDebug() << "Erreur getMoyenneNotes Oracle :" << q.lastError().text();
    return 0.0;
}

double StatistiquesService::getTauxOccupation()
{
    QSqlQuery qCap("SELECT NVL(SUM(CAPACITE), 0) FROM COURS WHERE STATUT IN ('PLANIFIE', 'EN_COURS')", Connection::instance()->getDatabase());
    int totalCap = 0;
    if (qCap.exec() && qCap.next()) {
        totalCap = qCap.value(0).toInt();
    } else {
        qDebug() << "Erreur getTauxOccupation (capacite) Oracle :" << qCap.lastError().text();
    }

    if (totalCap <= 0) {
        qDebug() << "getTauxOccupation: capacite totale = 0 (aucun cours PLANIFIE/EN_COURS avec CAPACITE > 0 ?)";
        return 0.0;
    }

    QSqlQuery qInsc(
        "SELECT COUNT(*) FROM INSCRIPTION i "
        "JOIN COURS c ON i.ID_COURS = c.ID_COURS "
        "WHERE i.STATUT != 'ABANDONNE' AND c.STATUT IN ('PLANIFIE', 'EN_COURS')",
        Connection::instance()->getDatabase()
    );
    int totalInsc = 0;
    if (qInsc.exec() && qInsc.next()) {
        totalInsc = qInsc.value(0).toInt();
    } else {
        qDebug() << "Erreur getTauxOccupation (inscriptions) Oracle :" << qInsc.lastError().text();
    }

    return (static_cast<double>(totalInsc) / static_cast<double>(totalCap)) * 100.0;
}

QString StatistiquesService::getFormationLaPlusDemandee()
{
    QSqlQuery q(
        "SELECT FORM FROM ("
        "  SELECT NVL(FORMATION, 'Non définie') AS FORM, COUNT(*) AS NB "
        "  FROM STAGIAIRE "
        "  GROUP BY FORMATION "
        "  ORDER BY NB DESC"
        ") WHERE ROWNUM = 1",
        Connection::instance()->getDatabase()
    );
    if (q.exec()) {
        if (q.next()) return q.value(0).toString();
        qDebug() << "getFormationLaPlusDemandee: aucune ligne retournee (table STAGIAIRE vide ?)";
    } else {
        qDebug() << "Erreur getFormationLaPlusDemandee Oracle :" << q.lastError().text();
    }
    return "N/A";
}

QString StatistiquesService::getCoursLePlusDemande()
{
    QSqlQuery q(
        "SELECT NOM FROM ("
        "  SELECT c.NOM, COUNT(i.ID_INSCRIPTION) AS NB "
        "  FROM COURS c "
        "  LEFT JOIN INSCRIPTION i ON c.ID_COURS = i.ID_COURS "
        "  GROUP BY c.ID_COURS, c.NOM "
        "  ORDER BY NB DESC"
        ") WHERE ROWNUM = 1",
        Connection::instance()->getDatabase()
    );
    if (q.exec()) {
        if (q.next()) return q.value(0).toString();
        qDebug() << "getCoursLePlusDemande: aucune ligne retournee (table COURS vide ?)";
    } else {
        qDebug() << "Erreur getCoursLePlusDemande Oracle :" << q.lastError().text();
    }
    return "N/A";
}

// ============================================================
// REPARTITIONS ET STATISTIQUES AGREGÉES
// ============================================================

QList<QPair<QString, int>> StatistiquesService::getStagiairesParFormation()
{
    QList<QPair<QString, int>> liste;
    QSqlQuery query(Connection::instance()->getDatabase());

    query.prepare(
        "SELECT NVL(FORMATION, 'Non définie') AS FORM, COUNT(*) AS NB "
        "FROM STAGIAIRE "
        "GROUP BY FORMATION "
        "ORDER BY NB DESC"
    );

    if (query.exec()) {
        while (query.next()) {
            liste.append(qMakePair(query.value(0).toString(), query.value(1).toInt()));
        }
    } else {
        qDebug() << "Erreur stats formation Oracle :" << query.lastError().text();
    }

    return liste;
}

QList<QPair<QString, int>> StatistiquesService::getStagiairesParNiveau()
{
    QList<QPair<QString, int>> liste;
    QSqlQuery query(Connection::instance()->getDatabase());

    query.prepare(
        "SELECT NVL(NIVEAU, 'Non défini') AS NIV, COUNT(*) AS NB "
        "FROM STAGIAIRE "
        "GROUP BY NIVEAU "
        "ORDER BY NB DESC"
    );

    if (query.exec()) {
        while (query.next()) {
            liste.append(qMakePair(query.value(0).toString(), query.value(1).toInt()));
        }
    } else {
        qDebug() << "Erreur stats niveau Oracle :" << query.lastError().text();
    }

    return liste;
}

QList<QPair<QString, int>> StatistiquesService::getInscriptionsParCours()
{
    QList<QPair<QString, int>> liste;
    QSqlQuery query(Connection::instance()->getDatabase());

    query.prepare(
        "SELECT c.NOM, COUNT(i.ID_INSCRIPTION) AS NB "
        "FROM COURS c "
        "LEFT JOIN INSCRIPTION i ON c.ID_COURS = i.ID_COURS "
        "GROUP BY c.ID_COURS, c.NOM "
        "ORDER BY NB DESC"
    );

    if (query.exec()) {
        while (query.next()) {
            liste.append(qMakePair(query.value(0).toString(), query.value(1).toInt()));
        }
    } else {
        qDebug() << "Erreur stats inscriptions cours Oracle :" << query.lastError().text();
    }

    return liste;
}

QList<QPair<QString, int>> StatistiquesService::getRepartitionStatutsStagiaire()
{
    QList<QPair<QString, int>> liste;
    QSqlQuery query(Connection::instance()->getDatabase());

    query.prepare(
        "SELECT STATUT, COUNT(*) AS NB "
        "FROM STAGIAIRE "
        "GROUP BY STATUT "
        "ORDER BY NB DESC"
    );

    if (query.exec()) {
        while (query.next()) {
            liste.append(qMakePair(query.value(0).toString(), query.value(1).toInt()));
        }
    } else {
        qDebug() << "Erreur stats statuts stagiaire Oracle :" << query.lastError().text();
    }

    return liste;
}

QList<QPair<QString, int>> StatistiquesService::getRepartitionStatutsInscription()
{
    QList<QPair<QString, int>> liste;
    QSqlQuery query(Connection::instance()->getDatabase());

    query.prepare(
        "SELECT STATUT, COUNT(*) AS NB "
        "FROM INSCRIPTION "
        "GROUP BY STATUT "
        "ORDER BY NB DESC"
    );

    if (query.exec()) {
        while (query.next()) {
            liste.append(qMakePair(query.value(0).toString(), query.value(1).toInt()));
        }
    } else {
        qDebug() << "Erreur stats statuts inscription Oracle :" << query.lastError().text();
    }

    return liste;
}

QList<QPair<QString, int>> StatistiquesService::getCapaciteParCours()
{
    QList<QPair<QString, int>> liste;
    QSqlQuery query(Connection::instance()->getDatabase());

    query.prepare("SELECT NOM, CAPACITE FROM COURS ORDER BY CAPACITE DESC");
    if (query.exec()) {
        while (query.next()) {
            liste.append(qMakePair(query.value(0).toString(), query.value(1).toInt()));
        }
    }
    return liste;
}

QList<QPair<QString, int>> StatistiquesService::getPlacesRestantesParCours()
{
    QList<QPair<QString, int>> liste;
    QSqlQuery query(Connection::instance()->getDatabase());

    query.prepare(
        "SELECT c.NOM, (c.CAPACITE - COUNT(i.ID_INSCRIPTION)) AS RESTANTES "
        "FROM COURS c "
        "LEFT JOIN INSCRIPTION i ON c.ID_COURS = i.ID_COURS AND i.STATUT != 'ABANDONNE' "
        "GROUP BY c.ID_COURS, c.NOM, c.CAPACITE "
        "ORDER BY RESTANTES ASC"
    );
    if (query.exec()) {
        while (query.next()) {
            liste.append(qMakePair(query.value(0).toString(), query.value(1).toInt()));
        }
    }
    return liste;
}

QList<QPair<QString, double>> StatistiquesService::getMoyenneNotesParCours()
{
    QList<QPair<QString, double>> liste;
    QSqlQuery query(Connection::instance()->getDatabase());

    query.prepare(
        "SELECT c.NOM, NVL(AVG(i.NOTE), 0) AS MOY "
        "FROM COURS c "
        "JOIN INSCRIPTION i ON c.ID_COURS = i.ID_COURS "
        "WHERE i.NOTE IS NOT NULL "
        "GROUP BY c.ID_COURS, c.NOM "
        "ORDER BY MOY DESC"
    );
    if (query.exec()) {
        while (query.next()) {
            liste.append(qMakePair(query.value(0).toString(), query.value(1).toDouble()));
        }
    }
    return liste;
}
