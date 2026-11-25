# Sparkle v0.1.5 — Passes applicatives et extensibilité

## Principe
Sparkle fournit l'infrastructure de chunks, mais pas le contenu d'un monde : relief, bruit, routes, villes, bâtiments, végétation, cavernes et mécanismes de gameplay sont écrits par l'application. Ni WorldPlan ni RegionPlan ni index global de métastructures obligatoires dans la version.

## Generator / données de génération
Generator<TChunk,TGenerationData> possède une instance persistante des données et exécute les passes dans l'ordre via addPass. Chaque passe reçoit les coordonnées et TGenerationData&. `_instantiateChunk(coord,data)` construit le chunk final et `_cleanGenerationData(coord,data)` nettoie les données temporaires après une préparation réussie, même en cas d'échec des passes.

L'utilisateur choisit sa seed, ses caches, sa logique de région, ses règles de synchronisation et sa politique de nettoyage. Sparkle ne verrouille pas les données et ne lie pas leur durée de vie aux observateurs.

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
// apply(coordinate, data) est appelé à chaque passe.
~~~

La reproductibilité des données pour un même ensemble de paramètres et coordonnées est un objectif strict des passes déterministes de l'exemple ; la bibliothèque ne rend pas un générateur arbitraire déterministe si ses données externes ne le sont pas. Les informations sur les grandes structures doivent être consultables depuis tous les chunks qu'elles traversent.

## Mondes finis
Une demande de chunk hors limites peut retourner false depuis `_prepareGenerationData(coordinate,data)` : aucune passe ni instanciation n'est exécutée. Le hook doit alors laisser les données sans état temporaire de cette demande. Le Generator retourne nullopt ; le ChunkRequester retourne Task::Completed avec un résultat vide. Ce résultat n'est pas un échec et reste enregistré comme absent seulement tant que la coordonnée est intéressante.

## Surcouche voxel
spk::Voxel::Chunk<SX,SY,SZ> contient une valeur de type Voxel::Volume de dimensions exactes SX×SY×SZ. Les données de génération voxel utilisent le Voxel::Volume::Builder existant. On peut éditer le chunk par setVolume(nouveauVolume), qui notifie la collection ; l'ancien Volume demeure une valeur immuable valide.

spk::Voxel::ChunkMesher exploite l'API existante du Mesher ; il cherche les six voisins au moment du bake, et outsideCell() consulte leur volume quand une face sort du chunk. L'adaptateur graphique reconstruit sans différentiel de bord le chunk modifié et ses six voisins.

## Extension headless
Toute Entity3D de chunk existe sans Graphic Engine. Le ChunkFilter<TChunk> est une référence non propriétaire vers le chunk de la collection ; un futur composant physique ou gameplay peut utiliser le même accès. Aucun composant de collision n'est fourni par cette release.

## Hors scope
Aucun format de sauvegarde du terrain, aucun protocole réseau, aucune IA de géographie, aucune politique de génération d'Erelia ou d'un jeu donné.
