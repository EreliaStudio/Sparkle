# Sparkle v0.1.5 — Infrastructure générique de chunks
> Documents de conception destinés à la PR draft [#85](https://github.com/EreliaStudio/Sparkle/pull/85), basée sur V0.1.5. Ces fichiers ne constituent pas une autorisation d'implémentation ou de fusion.

## Objectif
Fournir une infrastructure réutilisable pour gérer des chunks, leurs entités headless, la génération par passes, les demandes asynchrones, le streaming multi-observateurs et les usages spécialisés. **Le socle `spk::World` reste générique** et ne dépend pas des voxels. En parallèle, **`spk::Voxel` doit offrir une solution de génération de mondes voxel 3D directement utilisable** : l'implémentation spécialisée est une valeur ajoutée de Sparkle, sans devenir une contrainte des APIs génériques. Sparkle ne fixe pas de géographie ou de politique de placement propre à un jeu.

## Modèle retenu
- spk::World::Chunk<SX,SY,SZ> est une base héritable, sans données de jeu ni coordonnée membre ; les dimensions indiquent les unités du monde. Les setters de chaque chunk modifiable appellent notifyChunkEdition().
- ChunkCollection<TChunk> possède ses chunks par valeur et une Entity3D + ChunkFilter<TChunk> non propriétaire par coordonnée ; toutes les éditions sont relayées immédiatement avec la coordonnée. Fonctionne en EngineHeadless.
- Les événements de copie/déplacement restent attachés aux instances, ne suivent jamais les chunks copiés. Le remplacement d'une entrée préserve son Entity3D.
- Generator<TChunk,TGenerationData> reçoit des passes via addPass, chaque apply(builder, coordinate, data) travaillant sur des données temporaires. createChunkBuilder et generate retournent std::optional ; nullopt est un succès sans chunk.
- ChunkRequester<TChunk> encapsule des Tasks, avec réponse en std::optional<TChunk>. WorldStreamingSystem applique le résultat seulement sur l'Engine update, ignore les anciens résultats, ne cancelle pas les jobs, respecte une limite physique de requêtes, et oublie les absences quand l'intérêt disparaît.
- WorldObserver fournit rayon XYZ et position ; union multi-observateurs. Recalcul de la file au changement de coordonnée centrale de chunk, de rayon ou d'ensemble d'observateurs seulement ; Manhattan en unités du monde.
- Voxel::Chunk a un Volume de dimensions exactement SX×SY×SZ. Voxel::ChunkMesher redéfinit outsideCell pour consulter les six voisins à chaque bake. Modification => rebake du chunk et des six voisins, regroupé au rendu.

## Périmètre des documents

Les contrats historiques PR00–PR10 décrivent le socle de chunks et de streaming déjà traité ; leurs exclusions de biomes s'appliquaient à **ces PR**, et ne signifient pas que Sparkle doit exclure les biomes à l'avenir. La **nouvelle conception** porte sur une distribution générique d'influences pondérées, leur utilisation pour des hauteurs génériques, puis les couches spécialisées (notamment voxel). Voronoï et distribution procédurale infinie restent des exemples de stratégies, pas des choix imposés par `spk::World`.

La passe générique normalisera les poids bruts fournis par les hooks : cette responsabilité est validée. Les signatures et politiques de validation restantes sont encore ouvertes.

## Lecture
1. [Contrats actuels](contracts.md)
2. [Architecture et possession](architecture.md)
3. [Fiches PR00–PR10](implementation/README.md)
4. [Plan et dépendances](roadmap.md)
5. [Revue des risques](architecture-review.md)
6. [Tests et validation](validation.md)
7. [Passes applicatives](features.md)
8. [LifeCycle et Pool](life-cycle-design.md)
9. [Biomes — décisions d'architecture et propositions](biome-architecture.md) (conception en cours, sans implémentation)
10. [Questions ouvertes — inventaire pour conversations futures](biome-open-questions.md) (ne tranche aucune décision)

## Réutilisation du code Sparkle
EngineHeadless, Entity3D, Component3D, Registry, System, WorkerPool, Task, ContractProvider, Vector3Int, Voxel::Volume::Builder, Voxel::Mesher et Voxel::Renderer. Ne pas remplacer ces briques, et ne pas ajouter de protocole de réseau.

En dehors de la v0.1.5 : persistences/sauvegardes de terrain, systèmes de collision, mondes multiples dans un Engine, plans de régions et générateurs de géographie.
