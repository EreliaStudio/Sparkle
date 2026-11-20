# PR06 — WorldStreamingSystem : file, priorités, timeout et publication
**Couche :** Core/EngineHeadless. **Dépendances :** PR02, PR04, PR05. **État :** cahier des charges.

## Contrat
- Un WorldStreamingSystem<TChunk> se lie à une ChunkCollection et un ChunkRequester ; inspecte les WorldObserver du même EngineHeadless.
- À la première liaison, et seulement quand un centre **en coordonnées chunk** change, qu'un rayon change ou que la liste des observateurs change : reconstruire l'union des coordonnées souhaitées, enlever les requêtes en attente obsolètes, ajouter les nouvelles et recalculer les priorités. Aucune recomposition due à un simple mouvement à l'intérieur d'un chunk.
- Priorité : distance de Manhattan en unités du monde, à partir de la différence entre coordonnées de chunks multipliée par les dimensions SX/SY/SZ. Pour chaque coordonnée prendre la plus petite distance parmi les observateurs qui la demandent. Une égalité garde un ordre stable. Pas de préemption des Tasks déjà soumises.
- Budget configurable de tâches effectivement soumises/non terminées ; renouvellement du budget au fur et à mesure des fins de travaux, non par paquets. Reprioriser la file en attente, sans lancer plusieurs fois le même chunk actif.
- Les demandes portent un identifiant monotone ; abandon d'intérêt et timeout rendent l'ancien résultat obsolète. Aucune annulation : une Task expirée continue réellement et son résultat ne sera jamais publié, même après réentrée dans la zone.
- Point impératif : une tâche expirée mais encore exécutée **compte dans le budget physique**, pour éviter d'empiler sans fin des requêtes sur WorkerPool. Réessayer selon un délai configurable et seulement lorsqu'une place physique se libère ; un worker bloqué peut donc retarder le streaming, sans violation du budget.
- Sur Task::Completed avec std::optional<TChunk> vide, enregistrer la coordonnée comme absente pour la durée de son intérêt actuel, sans retry. Lorsque la coordonnée quitte l'union, oublier ce statut : au retour on redemande. Task::Failed et timeout sont des erreurs temporaires avec politique de retry, indépendantes des absences valides.
- Le callback de fin de tâche peut tourner sur worker ; il ne touche jamais l'Engine. Les réponses sont transférées/consommées sur update Engine et validées par l'intérêt actuel + la version avant l'ajout du chunk.
- Décharger immédiatement les chunks hors intérêt ; jamais de cache LRU. Couper les abonnements à l'arrêt, marquer les versions invalides et éviter tout accès à un système détruit. Ne pas bloquer son destructeur sur les Tasks externes.
- Un changement d'Engine délie le système ; une reconnexion ultérieure est explicite.

## Tests
Priorité Manhattan anisotrope et multiobs, pas de rebuild intra-chunk, frontières négatives, recalcul après rayon/observateur, Task périmée ignorée, Task non annulée, budget physique sous timeouts répétés, retries temporisés, nullopt dans l'intérêt seulement, réentrée, shutdown pendant callback, thread Engine unique pour collection.

## Exclusions
Aucune Graphics, aucun réseau ni stockage persistant, aucune annulation forcée des jobs.
