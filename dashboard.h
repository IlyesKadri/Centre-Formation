#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <QString>
#include <QList>
#include <QPair>
#include "inscription.h"

struct DashboardMetrics {
    int totalStagiaires;
    int totalCours;
    int totalInscriptions;
    int coursDisponibles;
};

class DashboardService
{
public:
    static DashboardMetrics getMetrics();
    static QList<Inscription> getDernieresInscriptions(int limit = 5);
    static QList<QPair<QString, int>> getCoursPopulaires(int limit = 5);
};

#endif // DASHBOARD_H
