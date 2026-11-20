# PR03 — Generator et passes de génération typées
**Couche :** Core. **Dépendances :** PR01. **État :** cahier des charges.

## Contrat public
- Generator<TChunk,TGenerationData> est une recette synchrone. Pour les types de chunk utilisés avec Generator seulement, TChunk::Builder existe ; la base spk::Chunk n'impose aucun Builder.
- Pas de Generator::Builder de configuration : le générateur expose addPass(...) pour enregistrer définitivement et dans l'ordre des passes polymorphes. Elles sont configurées avant les générations concurrentes.
- Pass::apply(TChunk::Builder&, const ChunkCoordinate&, TGenerationData&) est virtuelle. Les coordonnées sont disponibles dans chaque passe sans imposition de membre coordinate dans TGenerationData.
- createChunkBuilder(const ChunkCoordinate&) retourne std::optional<typename TChunk::Builder>. Un nullopt indique un chunk définitivement absent **pour cette demande**, et évite createGenerationData ainsi que toutes les passes.
- createGenerationData(coord) fournit une nouvelle TGenerationData pour chaque demande valide. Elle peut contenir des références aux structures persistantes de l'application, des résultats intermédiaires, des caches, des seeds optionnelles ; Sparkle n'en impose ni l'origine ni la durée de vie, ni le verrouillage.
- Exécuter toutes les passes dans l'ordre ; puis builder.build() consomme le Builder et retourne TChunk par valeur ; generate(coord) retourne std::optional<TChunk>. Les échecs sont des spk::Exception, jamais assimilés à nullopt ; ne pas publier de résultat incomplet.
- TChunk::Builder::load(const TChunk&) est recommandé pour les chunks construits avec Builder et requis par l'exemple voxel : nouvelle édition par reconstruction ; ce n'est pas un remplacement de l'édition directe des chunks publiés.
- Déterminisme attendu de l'exemple : mêmes réglages pertinents + mêmes coordonnées => mêmes données, quel que soit l'ordre des requêtes. Sparkle impose l'ordre des passes mais **pas** une seed unique ni la sécurité des données partagées gérées par l'utilisateur.

## Tests
Aucun Builder sur une classe utilisée seulement dans ChunkCollection ; passes appliquées dans l'ordre avec coord, data scratch fraîche, nullopt sans passes, exception en n'importe quelle étape, builder consommé, load et construction by-value, déterminisme sur demande en ordre inverse.

## Exclusions
Ni Task, ni WorkerPool, ni génération automatique de villes/biomes/routes/cavernes, ni contrôle centralisé des structures utilisateur.
