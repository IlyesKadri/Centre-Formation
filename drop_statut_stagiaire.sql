-- ========================================================
-- Script SQL pour Oracle XE : Suppression de la colonne STATUT
-- Table : STAGIAIRE
-- Projet : CentreFormation
-- ========================================================

-- 1. Suppression de la colonne STATUT de la table STAGIAIRE
ALTER TABLE STAGIAIRE DROP COLUMN STATUT;

-- 2. Validation de la modification
COMMIT;

-- 3. Vérification de la structure de la table STAGIAIRE
DESC STAGIAIRE;
