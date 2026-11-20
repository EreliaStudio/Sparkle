# PR06 — WorldStreamingSystem : file, priorités, timeout et publication
**Couche :** Core/EngineHeadless. **Dépendances :** PR02, PR04, PR05. **État :** implémentation PR #92.

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

## API et comportement réalisé

- `WorldStreamingSystem<TChunk>` est un `System` construit avec une `ChunkCollection<TChunk>&`, un `ChunkRequester<TChunk>&` et des `Settings`. L'Engine doit déjà posséder la collection avant l'appel à `EngineHeadless::addSystem`. Un transfert vers un autre Engine nécessite de se détacher au préalable ; changer l'Engine de la collection détache automatiquement le système.
- `Settings` contient `maxInFlight` (par défaut 4), `timeout` (10 s) et `retryDelay` (500 ms), chacun strictement positif. Horloge de supervision : `UpdateContext::time`. Un timeout est constaté lors d'un update tant que la réponse est encore `Pending`; le système ne peut pas reconstruire rétroactivement l'instant précis de completion de la Task.
- `inFlightCount()` mesure le nombre de Tasks soumises encore conservées physiquement (y compris expirées) et `planRevision()` mesure les reconstructions de la file pour le suivi du système.
- Les égalités de priorité ont un ordre déterministe selon l'ordre naturel de `ChunkCoordinate` ; l'ordre effectif de completion dépend du fournisseur de tâches.
- Les réponses sont interrogées sur update et ne sont **pas** traitées par abonnement sur thread worker. Une réponse déjà terminée peut être consommée dans le même update, libérant immédiatement son budget. Une réponse tardive invalidée n'écrit jamais dans la collection.
- L'abonnement à l'édition du contexte de la racine de `ChunkCollection` est résilié lors de la destruction du streaming. `ChunkCollection` et `ChunkRequester` doivent rester vivants tant que le streaming est lié ; les workers et leurs dépendances conservent leurs propres exigences de durée de vie, indépendantes du streamer.
- Cette PR ne change pas `Task` ni `WorkerPool` et ne tente pas d'annuler les tâches.

## Tests
Priorité Manhattan anisotrope et multiobs, pas de rebuild intra-chunk, frontières négatives, recalcul après rayon/observateur, Task périmée ignorée, Task non annulée, budget physique sous timeouts répétés, retries temporisés, nullopt dans l'intérêt seulement, réentrée, shutdown pendant callback, thread Engine unique pour collection.

## Exclusions
Aucune Graphics, aucun réseau ni stockage persistant, aucune annulation forcée des jobs.
