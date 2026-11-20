# PR07 — spk::Voxel::Chunk et démonstrateur headless
**Couche :** Voxel Core. **Dépendances :** PR01, PR02, PR03, PR06. **État :** cahier des charges.

## Contrat
- Fournir la surcouche générique spk::Voxel::Chunk<SX,SY,SZ> héritant de spk::Chunk<SX,SY,SZ>, contenant exactement un spk::Voxel::Volume de dimensions **SX × SY × SZ voxels**. Aucune résolution différente ni facteur d'échelle caché dans v0.1.5.
- Le chunk reste modifiable, sans imposer shared_ptr<const Data> à Chunk ni à Voxel::Chunk. Seul le Volume est une valeur immuable qui partage déjà ses Data via shared_ptr<const Data>.
- Un setter setVolume(Volume) remplace la valeur du Volume et appelle explicitement notifyChunkEdition() ; les autres setters ajoutés par un jeu suivent le même contrat. On n'édite pas les cellules d'un Volume publié sur place.
- Voxel::Chunk::Builder utilise le Voxel::Volume::Builder existant avec dimensions exactes, supporte load(existingChunk), et build() produit Voxel::Chunk par valeur en consommant le Builder.
- Échantillon d'une génération déterministe multi-passes avec ses propres seeds/références dans TGenerationData ; pas de seed, plan, cache ou gestion de cohérence inter-chunks imposés par Sparkle. Au moins une structure de démonstration traversant une frontière de chunk.
- Un chunk voxel copié ou déplacé n'emporte pas les abonnements de l'autre chunk ; sa collection relayant l'événement est correctement abonnée.

## Tests
Dimensions 8×16×32 correspondent exactement au Volume, setter notify et remplace sans éditer l'ancien Volume, builder.load, nullopt hors carte, cohérence aux coordonnées négatives et avec ordre inverse, une structure multi-chunks reproductible, lecture depuis ChunkFilter en EngineHeadless, pas de dépendance graphique.

## Exclusions
Aucun mesher (PR08), aucun moteur de terrain/biome/route/ville/caverne standard, aucun format voxel supplémentaire.
