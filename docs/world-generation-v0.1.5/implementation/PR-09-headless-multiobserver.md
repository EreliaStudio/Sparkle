# PR09 — Validation headless avec plusieurs observateurs et requesters
**Couche :** Core/intégration. **Dépendances :** PR05, PR06, PR07. **État :** cahier des charges.

## Scénario
- Un EngineHeadless, plusieurs Entity3D portant WorldObserver, plus des entités non observatrices. ChunkCollection<TChunk> possédant une Entity3D et son ChunkFilter par chunk.
- Source locale et mock de source distante/fichier, utilisant la même interface ChunkRequester et les mêmes std::optional<TChunk> ; aucune implémentation de protocole réseau.
- Déplacement des joueurs, union et séparation des intérêts, chargement/déchargement, modifications de chunk et notification via la collection ; des composants headless attachés aux entités persistent lors d'un remplacement.

## Tests
- Priorité Manhattan avec dimensions anisotropes ; priorité au plus proche des observateurs intéressés et stabilité des égalités.
- Aucune recomposition des demandes lors des mouvements intra-chunk ; recomposition sur franchissement/rayon/ajout/retrait d'observateur.
- Réponse tardive après timeout ou sortie/réentrée rejetée, aucune annulation, budget physique sous jobs bloqués, politique de retry et rétablissement.
- Résultat nullopt mémorisé pour la zone d'intérêt courante uniquement ; demande renouvelée après sortie puis réentrée ; Task::Failed distinct.
- Décomposition headless des entités et des contrats sans composant graphique ; test de shutdown avec tâches encore pendantes et absence de mutation depuis worker.
- Déterminisme des chunks de démonstration indépendamment de l'ordre de génération.

## Exclusions
Aucun serveur réel, aucun protocole, aucun UUID multimonde, aucun nouveau cache de chunk.
