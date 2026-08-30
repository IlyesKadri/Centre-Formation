#include "dashboard.h"
#include "connection.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>

DashboardMetrics DashboardService::getMetrics()
{
    DashboardMetrics m = {0, 0, 0, 0};
    QSqlDatabase db = Connection::instance()->getDatabase();

    // 1. Total Stagiaires
    QSqlQuery qStag(db);
    qStag.prepare("SELECT COUNT(*) FROM STAGIAIRE");
    if (qStag.exec() && qStag.next()) {
        m.totalStagiaires = qStag.value(0).toInt();
    }

    // 2. Total Cours
    QSqlQuery qCours(db);
    qCours.prepare("SELECT COUNT(*) FROM COURS");
    if (qCours.exec() && qCours.next()) {
        m.totalCours = qCours.value(0).toInt();
    }

    // 3. Total Inscriptions
    QSqlQuery qInsc(db);
    qInsc.prepare("SELECT COUNT(*) FROM INSCRIPTION");
    if (qInsc.exec() && qInsc.next()) {
        m.totalInscriptions = qInsc.value(0).toInt();
    }

    // 4. Cours Disponibles (PLANIFIE ou EN_COURS)
    QSqlQuery qDisp(db);
    qDisp.prepare("SELECT COUNT(*) FROM COURS WHERE STATUT IN ('PLANIFIE', 'EN_COURS')");
    if (qDisp.exec() && qDisp.next()) {
        m.coursDisponibles = qDisp.value(0).toInt();
    }

    return m;
}

QList<Inscription> DashboardService::getDernieresInscriptions(int limit)
{
    QList<Inscription> liste;
    QSqlQuery query(Connection::instance()->getDatabase());

    // Requête préparée pour Oracle avec ROWNUM pour imiter LIMIT
    query.prepare(
        "SELECT * FROM ("
        "  SELECT i.ID_INSCRIPTION, i.ID_STAGIAIRE, i.ID_COURS, i.DATE_INSCRIPTION, i.STATUT, i.NOTE, "
        "  s.NOM || ' ' || s.PRENOM AS STAGIAIRE_NOM, c.NOM AS COURS_NOM "
        "  FROM INSCRIPTION i "
        "  JOIN STAGIAIRE s ON i.ID_STAGIAIRE = s.ID_STAGIAIRE "
        "  JOIN COURS c ON i.ID_COURS = c.ID_COURS "
        "  ORDER BY i.ID_INSCRIPTION DESC"
        ") WHERE ROWNUM <= :lim"
    );
    query.bindValue(":lim", limit);

    if (!query.exec()) {
        qDebug() << "Erreur dernières inscriptions Oracle :" << query.lastError().text();
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

QList<QPair<QString, int>> DashboardService::getCoursPopulaires(int limit)
{
    QList<QPair<QString, int>> liste;
    QSqlQuery query(Connection::instance()->getDatabase());

    query.prepare(
        "SELECT * FROM ("
        "  SELECT c.NOM, COUNT(i.ID_INSCRIPTION) AS NB_INSCRITS "
        "  FROM COURS c "
        "  LEFT JOIN INSCRIPTION i ON c.ID_COURS = i.ID_COURS "
        "  GROUP BY c.ID_COURS, c.NOM "
        "  ORDER BY NB_INSCRITS DESC"
        ") WHERE ROWNUM <= :lim"
    );
    query.bindValue(":lim", limit);

    if (!query.exec()) {
        qDebug() << "Erreur cours populaires Oracle :" << query.lastError().text();
        return liste;
    }

    while (query.next()) {
        QString nomCours = query.value(0).toString();
        int nbInscrits = query.value(1).toInt();
        liste.append(qMakePair(nomCours, nbInscrits));
    }

    return liste;
}
