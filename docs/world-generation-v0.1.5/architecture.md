# Sparkle v0.1.5 — Architecture et responsabilités
Statut : conception, PR draft #85. Le C++ n'est pas encore implémenté ; les noms de signatures restent illustratifs.

## Chaîne fonctionnelle
WorldObserver (Component3D + EngineHeadless Registry) → WorldStreamingSystem<TChunk> (union, file Manhattan, gestion des requêtes) → ChunkRequester<TChunk> (Task<std::optional<TChunk>>) → Generator<TChunk,TGenerationData> ou source externe → ChunkCollection<TChunk> (TChunk + Entity3D/ChunkFilter) → consommateurs headless / adaptateur Graphics.
En Graphics, World::Voxel::ChunkMesher consulte les six voisins lors de chaque cuisson, puis les composants de rendu sont actualisés.

## Types et coordonnées
- `spk::World` est un namespace regroupant les types du monde (`Chunk`, `ChunkCollection`, `ChunkCoordinate`, `ChunkFilter`, `ChunkInterest`, `ChunkRequester`, `Generator`, `LocalChunkRequester`, `WorldObserver`, `WorldStreamingSystem`). Ce n'est pas une classe ni une instance de monde.
- spk::World::Chunk<SX,SY,SZ> est une base héritable dont les dimensions positives compile-time sont en unités du monde ; ne possède pas sa coordonnée, ses données applicatives ni une Entity.
- ChunkCoordinate est un triplet signé, avec conversions static worldToChunkCoordinate (floor pour les négatifs), chunkCoordinateToWorld (origine), worldToLocalCoordinate ; vérification des débordements.
- Un Chunk est mutable. Il expose un événement propre et la méthode protégée notifyChunkEdition(), que chaque setter de classe dérivée appelle explicitement après une vraie modification.
- Copie/déplacement construction : créer un nouveau fournisseur de notification, jamais transférer les souscriptions de l'original. Affectation d'une valeur existante : conserver les souscriptions de la destination, sans importer celles de la source ; la collection signale le remplacement.

## ChunkCollection et EngineHeadless
- ChunkCollection<TChunk> contient une std::map ordonnée de ChunkCoordinate → ChunkEntry, avec TChunk **par valeur** (adresse stable tant que présent), entité 3D et contrat de souscription à l'édition. Les pointeurs propriétaires pour Entity3D restent un détail interne d'implémentation, non une contrainte de l'API de TChunk.
- Une Entity3D racine regroupe les Entity3D des chunks ; mode headless complet : composants physiques futurs possibles sans affichage.
- Chaque Entity3D possède un ChunkFilter<TChunk> (Component3D) pointant non-propriétairement vers l'instance de chunk de sa collection. Aucune copie de chunk dans le filtre.
- addChunk(coord,TChunk) insère ou remplace les données **à l'adresse stable** en conservant Entity3D, ses composants et ChunkFilter, réabonne si besoin les événements, puis notifie l'ajout/remplacement. Les objets modifiés via leurs setters notifient d'eux-mêmes ; la collection relaie avec coordonnée immédiatement.
- removeChunk détruit les composants et l'Entity3D avant TChunk, résilie l'abonnement, et déclenche la destruction LifeCycle ; pas de second événement de suppression générique. Les consommateurs peuvent capturer la coordonnée sans toucher l'objet en destruction.
- Accès : at et operator[] renvoient une référence ou lancent spk::Exception en cas d'absence (pas d'insertion), tryGet renvoie un pointeur nullable, contains un bool ; itération [coord, entry] avec chunk() et entity() non propriétaires.
- Attach/detach/rebind explicite de la racine à un EngineHeadless, sans perdre les chunks. Les systèmes liés à un autre Engine se délient ; reconnexion explicite.
- Tout ajout/édition/suppression de collection attachée se fait sur le thread Engine ; aucune modification directe depuis un worker.

## Génération sans propriétaire caché
- Le Generator<TChunk,TGenerationData> possède une instance persistante de TGenerationData, construite avec les arguments fournis à son constructeur ; il ne connaît ni colonnes, ni régions, ni builders.
- `_prepareGenerationData(coord,data)` initialise éventuellement les données propres à la demande ; false signale une absence valide. En cas de rejet ou d'exception, ce hook ne doit laisser aucun état temporaire.
- L'état de génération choisit ses propres structures, caches, synchronisation et durées de vie ; aucun verrou ni compteur d'intérêt n'est imposé par le générateur.
- `Pass::apply(coord,data)` est appelé dans l'ordre ; `_instantiateChunk(coord,data)` retourne le chunk final. `_cleanGenerationData(coord,data) noexcept` est appelé après une préparation réussie, même si une passe ou l'instanciation échoue. `generate(coord)` renvoie `std::optional<TChunk>`.
- Une exception spk::Exception fait échouer la génération, sans résultat partiel. La reproductibilité provient des règles utilisateur : mêmes données/coordonnées pertinentes => même chunk, pas de seed universelle imposée.

## Requester, Task et WorkerPool
- ChunkRequester<TChunk> retourne une réponse Task<std::optional<TChunk>> pour une coordonnée. Une Task Completed contenant nullopt ne constitue pas une erreur ; Failed et timeout sont indépendants.
- L'API actuelle Task::Answer::result() fournit const TResult& : en v0.1.5 le TChunk doit donc supporter la copie de valeur au transfert vers la collection (avec fournisseur d'événements indépendant), sinon il faudra revoir Task explicitement dans une PR dédiée.
- Un WorkerPool local exécute Generator. Son thread peut achever les Tasks dans un ordre quelconque ; seul le streamer publie les résultats sur l'update Engine.
- Le streamer se ferme sans attendre les tâches devenues inutiles : leurs résultats sont rejetés. Les opérations worker n'accèdent pas au streamer détruit. Le WorkerPool réel utilise std::jthread : sa destruction peut attendre ; conserver une durée de vie externe appropriée est impératif.

## Intérêt et streaming
- WorldObserver donne rayon XYZ non négatif (boîte inclusive) et position mondiale ; support multiobservateurs et union des intérêts, un monde par Engine v0.1.5.
- Maintenir par observateur sa **coordonnée centre en chunk**. Recalculer l'union et reconstruire la file seulement à son changement, à un changement de rayon ou à ajout/détachement/suppression d'observateur. Les mouvements intra-chunk ne font aucun travail de priorisation.
- Priorité des chunks demandés : minimum des distances de Manhattan en unités du monde aux observateurs intéressés, dimensions SX,SY,SZ prises en compte, égalités stables.
- Budget configurable de requêtes réellement soumises non terminées, y compris expirées. Aucune préemption ou annulation forcée ; une Task timed out est invalidée et peut être réessayée ultérieurement sous budget, son résultat tardif est ignoré.
- Identifiants de requête monotones ; rejet si intérêt retiré ou version obsolète. Retry temporisé seulement sur échec/timeout.
- Une coordonnée avec réponse nullopt est mémorisée absente **seulement pendant la période d'intérêt courante**, sans retry ; une sortie de l'intérêt supprime cette mémoire, une réentrée crée une nouvelle demande.
- Décharger immédiatement les chunks hors de l'union. Aucun cache LRU, persistance automatique ou protocole réseau.

## Voxel et Graphics
- spk::World::Voxel::Chunk<SX,SY,SZ> hérite de spk::World::Chunk<SX,SY,SZ> et contient un Voxel::Volume de dimensions exactes SX×SY×SZ. Le chunk est modifiable, mais le Volume lui-même est une valeur immuable, dont les Data sont déjà partagées de manière const ; setVolume remplace la valeur puis notifie.
- Il n'existe pas de `World::Voxel::Chunk::Builder` ni de constructeur prenant un `Volume`. Le chunk possède un volume vide par défaut et reçoit ses données avec `setVolume(Volume)`. Les modifications de cellules sont préparées dans `Voxel::Volume::Builder`.
- spk::World::Voxel::ChunkMesher hérite de Voxel::Mesher ; outsideCell() consulte les six voisins de la collection lors de chaque bake. Pas de stockage durable des voisins, pas de MesherContext requis, pas de changement d'algorithme d'occlusion.
- L'adaptateur graphique est optionnel et se lie explicitement à ChunkCollection. Il scanne les chunks chargés au bind, puis place les coordonnées notifiées dans un set de dirty, traité une fois par rendu.
- À chaque ajout/édition/remplacement : invalider systématiquement le chunk et **ses six voisins** ; à une suppression : destruction LifeCycle de l'entité puis invalidation des six voisins. Pas de détection coûteuse des frontières éditées.
- L'entité et ses composants gameplay/headless persistent lors d'une édition, le rendu ajoute/réutilise ses propres composants. Unbind révoque ses souscriptions.

## Lifecycle Core
LifeCycle expose des événements séparés Acquisition, Release, Destruction ; Widget et Entity l'adoptent. Pool<T> n'émet ces événements que pour les T compatibles, sans changer sa mécanique Lease/weak State. Voir [PR00](implementation/PR-00-lifecycle.md) et [design détaillé](life-cycle-design.md).

## Limites
Pas de terrain prédéfini, ville, route, cave, macro-plan obligatoire, nouvelle physique, sauvegarde, réseau ou plusieurs mondes par Engine. Voir [PRs](implementation/README.md).
