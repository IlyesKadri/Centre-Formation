#ifndef STAGIAIRE_H
#define STAGIAIRE_H

#include <QString>
#include <QDate>
#include <QList>

class Stagiaire
{
private:
    int idStagiaire;
    QString nom;
    QString prenom;
    QString email;
    QString telephone;
    QDate dateNaissance;
    QString niveau;
    QString formation;

public:
    // Constructeurs
    Stagiaire();

    Stagiaire(int idStagiaire,
              const QString& nom,
              const QString& prenom,
              const QString& email,
              const QString& telephone,
              const QDate& dateNaissance,
              const QString& niveau,
              const QString& formation);

    // Getters
    int getIdStagiaire() const;
    QString getNom() const;
    QString getPrenom() const;
    QString getEmail() const;
    QString getTelephone() const;
    QDate getDateNaissance() const;
    QString getNiveau() const;
    QString getFormation() const;

    // Setters
    void setIdStagiaire(int id);
    void setNom(const QString& nom);
    void setPrenom(const QString& prenom);
    void setEmail(const QString& email);
    void setTelephone(const QString& telephone);
    void setDateNaissance(const QDate& date);
    void setNiveau(const QString& niveau);
    void setFormation(const QString& formation);

    // Validation
    bool estValide(QString& erreur) const;

    // CRUD
    bool ajouter();
    bool modifier();
    bool supprimer();

    // Lecture
    static QList<Stagiaire> afficher();
    static Stagiaire getById(int idStagiaire);

    // Recherche multicritère & globale
    static QList<Stagiaire> rechercher(
        const QString& nom = "",
        const QString& formation = "",
        const QString& niveau = ""
        );
    static QList<Stagiaire> rechercherGlobale(const QString& texte);
    static QList<Stagiaire> rechercherEtTrier(const QString& search = "",
                                             const QList<QPair<QString, QString>>& criteresTri = {});
};

#endif // STAGIAIRE_H
