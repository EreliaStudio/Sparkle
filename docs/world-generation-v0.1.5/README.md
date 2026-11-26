# Sparkle v0.1.5 — Infrastructure générique de chunks
> Documents de conception destinés à la PR draft [#85](https://github.com/EreliaStudio/Sparkle/pull/85), basée sur V0.1.5. Ces fichiers ne constituent pas une autorisation d'implémentation ou de fusion.

## Objectif
Fournir une infrastructure réutilisable pour gérer des chunks, leurs entités headless, la génération par passes, les demandes asynchrones, le streaming multi-observateurs et un adaptateur de rendu voxel optionnel. Sparkle ne définit ni biomes, ni routes, ni villes, ni cavernes, ni planification géographique obligatoire.

## Modèle retenu
- spk::World::Chunk<SX,SY,SZ> est une base héritable, sans données de jeu ni coordonnée membre ; les dimensions indiquent les unités du monde. Les setters de chaque chunk modifiable appellent notifyChunkEdition().
- ChunkCollection<TChunk> possède ses chunks par valeur et une Entity3D + ChunkFilter<TChunk> non propriétaire par coordonnée ; toutes les éditions sont relayées immédiatement avec la coordonnée. Fonctionne en EngineHeadless.
- Les événements de copie/déplacement restent attachés aux instances, ne suivent jamais les chunks copiés. Le remplacement d'une entrée préserve son Entity3D.
- Generator<TChunk,TGenerationData> possède les données persistantes ; les passes reçoivent apply(coord,data). _prepareGenerationData(coord,data) peut refuser une demande ; _instantiateChunk(coord,data) construit le chunk, _cleanGenerationData(coord,data) nettoie le scratch.
- ChunkRequester<TChunk> encapsule des Tasks, avec réponse en std::optional<TChunk>. WorldStreamingSystem applique le résultat seulement sur l'Engine update, ignore les anciens résultats, ne cancelle pas les jobs, respecte une limite physique de requêtes, et oublie les absences quand l'intérêt disparaît.
- WorldObserver fournit rayon XYZ et position ; union multi-observateurs. Recalcul de la file au changement de coordonnée centrale de chunk, de rayon ou d'ensemble d'observateurs seulement ; Manhattan en unités du monde.
- World::Voxel::Chunk a un Volume de dimensions exactement SX×SY×SZ. World::Voxel::ChunkMesher redéfinit outsideCell pour consulter les six voisins à chaque bake. Modification => rebake du chunk et des six voisins, regroupé au rendu.

## Lecture
1. [Contrats actuels](contracts.md)
2. [Architecture et possession](architecture.md)
3. [Fiches PR00–PR10](implementation/README.md)
4. [Plan et dépendances](roadmap.md)
5. [Revue des risques](architecture-review.md)
6. [Tests et validation](validation.md)
7. [Passes applicatives](features.md)
8. [LifeCycle et Pool](life-cycle-design.md)

## Réutilisation du code Sparkle
EngineHeadless, Entity3D, Component3D, Registry, System, WorkerPool, Task, ContractProvider, Vector3Int, Voxel::Volume::Builder, Voxel::Mesher et Voxel::Renderer. Ne pas remplacer ces briques, et ne pas ajouter de protocole de réseau.

En dehors de la v0.1.5 : persistences/sauvegardes de terrain, systèmes de collision, mondes multiples dans un Engine, plans de régions et générateurs de géographie.
