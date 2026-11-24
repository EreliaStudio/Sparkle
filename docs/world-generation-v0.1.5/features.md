# Sparkle v0.1.5 — Passes applicatives et extensibilité

## Principe
Dans le périmètre historique PR00–PR10, Sparkle fournit l'infrastructure de chunks et les passes utilisateur, sans imposer de géographie. La suite de la v0.1.5 vise en plus des **briques génériques réutilisables de biomes et de hauteur**, ainsi qu'une **génération voxel spécialisée directement exploitable**, sans exiger que chaque jeu réécrive ces mécanismes. Les règles de monde propres à un jeu (routes, villes, bâtiments, cavernes, etc.) restent extensibles ; aucun WorldPlan, RegionPlan ou index global de métastructures n'est obligatoire.

## Generator / données de génération
Generator<TChunk,TGenerationData> reçoit ses passes dans l'ordre via addPass. Chaque passe virtuelle reçoit (TChunk::Builder&, ChunkCoordinate, TGenerationData&) ; une passe peut remplir seulement les données intermédiaires, une autre peut modifier le Builder. Le Builder est propre à une seule génération et finalise par build(), puis est consommé.

L'utilisateur choisit lui-même sa seed éventuelle, ses paramètres, son cache, sa logique de région et ses règles de synchronisation. Sparkle n'impose aucun cycle de vie, ni mutex, ni single seed dans GenerationData.

Illustration non normative :
~~~cpp
struct ExampleGenerationData {
    WorldSettings& settings;        // Stockage utilisateur
    RegionCache& cache;             // Peut être mutable : synchronization utilisateur
    HeightSamples heights;          // Intermédiaire par demande
    std::vector<BuildingPlan> buildings;
};

// Les passes sont enregistrées par addPass dans cet ordre :
// HeightPlanning -> StructurePlanning -> VoxelRealization.
// apply(chunkBuilder, coordinate, data) est appelé à chaque passe.
~~~

La reproductibilité des données pour un même ensemble de paramètres et coordonnées est un objectif strict des passes déterministes de l'exemple ; la bibliothèque ne rend pas un générateur arbitraire déterministe si ses données externes ne le sont pas. Les informations sur les grandes structures doivent être consultables depuis tous les chunks qu'elles traversent.

## Mondes finis
Une demande de chunk hors limites peut retourner un std::optional<Builder> vide dès createChunkBuilder : les passes ne sont pas exécutées. Le Generator produit std::optional<TChunk>{} ; le ChunkRequester retourne Task::Completed avec un résultat vide. Ce résultat n'est pas un échec et reste enregistré comme absent seulement tant que la coordonnée est intéressante.

## Extension de génération biome / hauteur (conception en cours)

Une passe générique de distribution doit parcourir un domaine X/Z, appeler un hook fourni par la stratégie pour les références de biomes et poids relatifs, **normaliser elle-même** les poids et publier temporairement le résultat dans `TGenerationData`. Les détails de conteneur, hooks exacts, validations, comportement en cas d'égalité et intégration à plusieurs sources de hauteur restent ouverts. Les stratégies de placement concrètes, dont Voronoï ou la génération infinie, ne sont pas définies ici.

La couche `spk::World` ne connaît aucun type voxel ; une couche spécialisée peut consommer ses résultats, sans recopier la logique de distribution. Voir [conception des biomes](biome-architecture.md) et [questions ouvertes](biome-open-questions.md).

## Surcouche voxel
spk::Voxel::Chunk<SX,SY,SZ> contient une valeur de type Voxel::Volume de dimensions exactes SX×SY×SZ. Son Builder utilise le Voxel::Volume::Builder existant. On peut éditer le chunk par setVolume(nouveauVolume), qui notifie la collection ; l'ancien Volume demeure une valeur immuable valide.

spk::Voxel::ChunkMesher exploite l'API existante du Mesher ; il cherche les six voisins au moment du bake, et outsideCell() consulte leur volume quand une face sort du chunk. L'adaptateur graphique reconstruit sans différentiel de bord le chunk modifié et ses six voisins.

## Extension headless
Toute Entity3D de chunk existe sans Graphic Engine. Le ChunkFilter<TChunk> est une référence non propriétaire vers le chunk de la collection ; un futur composant physique ou gameplay peut utiliser le même accès. Aucun composant de collision n'est fourni par cette release.

## Hors scope
Aucun format de sauvegarde du terrain, aucun protocole réseau, aucune IA de géographie, aucune politique de génération d'Erelia ou d'un jeu donné.
