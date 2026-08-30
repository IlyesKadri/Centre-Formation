#ifndef RECOMMANDATIONWIDGET_H
#define RECOMMANDATIONWIDGET_H

#include <QWidget>
#include "groqrecommendationservice.h"

namespace Ui {
class RecommandationWidget;
}

class RecommandationWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RecommandationWidget(QWidget *parent = nullptr);
    ~RecommandationWidget();

    void chargerStagiaires();
    void afficherProfilStagiaire(int idStagiaire);

signals:
    void inscriptionDemandee(int idStagiaire, int idCours);
    void statusMessage(const QString &message, int timeout = 0);

private slots:
    void on_comboRecomStagiaire_currentIndexChanged(int index);
    void on_btnLancerRecommandation_clicked();
    void onRecommendationsReceived(const QList<GroqRecommendationResult> &recommendations, const QString &sourceInfo);
    void onRecommendationError(const QString &errorMessage);

private:
    Ui::RecommandationWidget *ui;
    GroqRecommendationService *groqService;
};

#endif // RECOMMANDATIONWIDGET_H
