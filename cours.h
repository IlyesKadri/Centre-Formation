#ifndef COURS_H
#define COURS_H

#include <QString>
#include <QDate>
#include <QList>

class Cours
{
private:
    int idCours;
    QString nom;
    QString description;
    QDate dateDebut;
    QDate dateFin;
    int duree;
    int capacite;
    double prix;
    QString statut;

public:
    // Constructeurs
    Cours();

    Cours(int idCours,
          const QString& nom,
          const QString& description,
          const QDate& dateDebut,
          const QDate& dateFin,
          int duree,
          int capacite,
          double prix,
          const QString& statut);

    // Getters
    int getIdCours() const;
    QString getNom() const;
    QString getDescription() const;
    QDate getDateDebut() const;
    QDate getDateFin() const;
    int getDuree() const;
    int getCapacite() const;
    double getPrix() const;
    QString getStatut() const;

    // Setters
    void setIdCours(int id);
    void setNom(const QString& nom);
    void setDescription(const QString& desc);
    void setDateDebut(const QDate& date);
    void setDateFin(const QDate& date);
    void setDuree(int duree);
    void setCapacite(int cap);
    void setPrix(double prix);
    void setStatut(const QString& statut);

    // Validation C++
    bool estValide(QString& erreur) const;

    // CRUD SQL (Requêtes préparées)
    bool ajouter();
    bool modifier();
    bool supprimer();

    // Lecture & Recherche
    static QList<Cours> afficher();
    static QList<Cours> rechercher(const QString& nom = "", const QString& statut = "");
    static QList<Cours> rechercherGlobale(const QString& texte);
    static QList<Cours> rechercherEtTrier(const QString& search = "",
                                         const QList<QPair<QString, QString>>& criteresTri = {});
    static int compterInscriptionsActuelles(int idCours);
    static Cours getById(int idCours);
};

#endif // COURS_H
