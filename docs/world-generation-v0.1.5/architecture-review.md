# Sparkle v0.1.5 — Audit de l'architecture et des PR
État après le réexamen des 11 fiches [PR00–PR10](implementation/README.md). La conception de haut niveau est **suffisante pour découper les implémentations**, mais les points ci-dessous sont des contrôles techniques obligatoires durant leurs PR, et non de nouvelles fonctionnalités.

## Décisions consolidées
- Chunk<T dimensions> héritable, données libres et mutables, coordonnée stockée uniquement comme clé de map. Copie/move sans transfert d'abonnements ; chaque setter appelle notifyChunkEdition().
- ChunkCollection possède TChunk par valeur et Entity3D headless, plus ChunkFilter non propriétaire ; édition relayée par la collection, remplacement conserve entité, suppression détruit entité/composants.
- Generator avec addPass et apply(coord,data), données persistantes synchronisées par leur propriétaire, instanciation et nettoyage par coordonnée ; aucun builder, colonne ou compteur d'intérêt imposé.
- Task renvoie std::optional<TChunk>, nullopt succès et absence au seul sein de l'intérêt ; échec/timeout permettent retry contrôlé. Pas d'annulation, résultat ancien toujours rejeté.
- WorldObserver et multi-observateur ; recalcul de l'union et file seulement au franchissement de coordonnée de chunk, rayon/observateurs modifiés. Manhattan à dimensions pondérées.
- Volume voxel exactement SX×SY×SZ, setter sur chunk mutable, Voxel::ChunkMesher par héritage de Mesher avec outsideCell() et consultation des six voisins à chaque bake. À chaque changement, rebake le chunk et six voisins.

## Risques d'implémentation identifiés et affectation explicite
| Risque / vérification | PR chargée |
| --- | --- |
| ContractProvider non copiable : fournir sémantique de copie/move sur Chunk sans répliquer les abonnés | 01 |
| Conservation d'adresse lors du remplacement map et durée de vie du ChunkFilter ; reentrance des événements | 02 |
| Destruction Entity et ordre des callbacks LifeCycle, composants attachés, Engine attach/detach | 00, 02 |
| Données optionnelles, signature virtuelle apply(coordinate,data), instanciation finale du chunk | 03 |
| Task::Answer::result() renvoie const TResult& : copie nécessaire depuis optional<TChunk> ; vérifier les contraintes C++ | 04 |
| WorkerPool::~WorkerPool utilise jthread, peut attendre un job bloqué ; garder ses dépendances en vie sans bloquer ~WorldStreamingSystem | 04, 06 |
| Timeout n'interrompt pas le job réel ; compter toutes les demandes soumises non terminées dans le cap | 06 |
| Nullopt ne doit pas être un cache LRU : conserver seulement tant que coordonnée reste intéressée | 06 |
| Événement de disparition par LifeCycle::Destruction, sans deref de l'entité pendant callback | 00, 02, 08 |
| Voxel::Mesher::outsideCell, indexing des faces, lookup voisin temporaire, pas de double maillage entre chunks | 08 |
| Tests négatifs, dimensions non cubiques, multiobs, déterminisme utilisateur et goldens | 07, 09, 10 |

## Points de signature à finaliser lors du code
- Méthodes exactes de souscription Chunk et collection, noms des setters du filtre et forme de la référence non propriétaire.
- Modes copy/move de TChunk nécessaires au contrat du conteneur et du Task ; comment autoriser un placement stable sans perdre le fournisseur d'abonnements propre à la destination.
- Comportement des exceptions dans les callbacks, contrat de réentrance, ordre précis d'invalidation du filtre et destruction de l'entité.
- Durée de vie réelle des objets capturés par les workers et rythme des nouvelles tentatives ; ne pas annoncer un shutdown non bloquant de WorkerPool lui-même.
- Décision d'égalité de priorités (ordre stable), valeurs par défaut du budget et des délais de retry.
Ces points doivent recevoir des tests dans leur PR respective ; aucun ne justifie l'ajout d'une infrastructure hors scope.

## Contraintes non négociables
Ne pas réintroduire des unique_ptr<TChunk> dans l'API utilisateur ; ne pas forcer shared_ptr<const Data> dans Chunk ; ne pas notifier la collection à chaque update Engine ; pas de cache voisin permanent ; pas de monde géographique prédéfini, réseau ou moteur physique.

**Statut :** fiches de conception uniquement, aucune modification C++ ni autorisation de merge.
