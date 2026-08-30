#ifndef STATISTIQUES_H
#define STATISTIQUES_H

#include <QString>
#include <QList>
#include <QPair>

class StatistiquesService
{
public:

    // =========================================================
    // KPI
    // =========================================================

    static int getTotalStagiaires();
    static int getTotalCours();
    static int getTotalInscriptions();
    static int getCoursDisponibles();
    static int getStagiairesActifs();
    static int getCoursComplets();

    static double getMoyenneNotes();
    static double getTauxOccupation();


    // =========================================================
    // REPARTITIONS
    // =========================================================

    static QList<QPair<QString, int>> getStagiairesParFormation();

    static QList<QPair<QString, int>> getStagiairesParNiveau();

    static QList<QPair<QString, int>> getInscriptionsParCours();

    static QList<QPair<QString, int>> getRepartitionStatutsStagiaire();

    static QList<QPair<QString, int>> getRepartitionStatutsInscription();


    // =========================================================
    // STATISTIQUES COURS
    // =========================================================

    static QList<QPair<QString, int>> getCapaciteParCours();

    static QList<QPair<QString, int>> getPlacesRestantesParCours();


    // =========================================================
    // STATISTIQUES NOTES
    // =========================================================

    static QList<QPair<QString, double>> getMoyenneNotesParCours();


    // =========================================================
    // TOP
    // =========================================================

    static QString getFormationLaPlusDemandee();

    static QString getCoursLePlusDemande();
};

#endif // STATISTIQUES_H
