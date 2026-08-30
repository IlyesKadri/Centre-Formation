#ifndef INSCRIPTION_H
#define INSCRIPTION_H

#include <QString>
#include <QDate>
#include <QList>

class Inscription
{
private:
    int idInscription;
    int idStagiaire;
    int idCours;
    QDate dateInscription;
    QString statut;
    double note;

    // Attributs jointures pour affichage UI
    QString nomStagiaire;
    QString nomCours;

public:
    // Constructeurs
    Inscription();

    Inscription(int idInscription,
                int idStagiaire,
                int idCours,
                const QDate& dateInscription,
                const QString& statut,
                double note);

    // Getters
    int getIdInscription() const;
    int getIdStagiaire() const;
    int getIdCours() const;
    QDate getDateInscription() const;
    QString getStatut() const;
    double getNote() const;
    QString getNomStagiaire() const;
    QString getNomCours() const;

    // Setters
    void setIdInscription(int id);
    void setIdStagiaire(int idStag);
    void setIdCours(int idC);
    void setDateInscription(const QDate& date);
    void setStatut(const QString& statut);
    void setNote(double note);
    void setNomStagiaire(const QString& nom);
    void setNomCours(const QString& nom);

    // Validations & Règle Métier
    bool estValide(QString& erreur) const;
    static bool verifierRegleMetier(int idStagiaire, int idCours, int idInscriptionActuelle, QString& messageErreur);

    // CRUD SQL (Requêtes préparées)
    bool ajouter();
    bool modifier();
    bool supprimer();

    // Lecture & Recherche avec JOIN Oracle
    static QList<Inscription> afficher();
    static QList<Inscription> rechercher(const QString& searchStagiaire = "",
                                         const QString& searchCours = "",
                                         const QString& statut = "");
    static QList<Inscription> rechercherGlobale(const QString& texte);
    static QList<Inscription> rechercherEtTrier(const QString& search = "",
                                               const QList<QPair<QString, QString>>& criteresTri = {});
    static Inscription getById(int idInscription);
    static QList<Inscription> readByStagiaire(int idStagiaire);

    // Génération de Document PDF
    static bool genererAttestationPdf(int idInscription, const QString& filePath, QString& messageErreur);
};

#endif // INSCRIPTION_H
