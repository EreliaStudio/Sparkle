# PR03 — Generator et passes de génération typées
**Couche :** Core. **Dépendances :** PR01. **État :** cahier des charges.

## Contrat public
- Generator<TChunk,TGenerationData> est une recette synchrone. Aucun Builder ni constructeur particulier n'est imposé au chunk.
- Pas de Generator::Builder de configuration : le générateur expose uniquement addPass<TPass>(...) pour créer et posséder les passes (pas de surcharge prenant std::unique_ptr) pour enregistrer définitivement et dans l'ordre des passes polymorphes. Elles sont configurées avant les générations concurrentes.
- Pass::apply(const ChunkCoordinate&, TGenerationData&) est virtuelle. Les coordonnées sont transmises à chaque passe.
- Le hook virtuel protégé _createGenerationData(const ChunkCoordinate&) retourne std::optional<TGenerationData>. Un nullopt indique une absence valide et évite toutes les passes et l'instanciation.
- Les données de génération peuvent contenir des références, des résultats intermédiaires, des caches, des seeds ou des builders spécifiques. Elles peuvent être move-only.
- Exécuter toutes les passes dans l'ordre ; puis _instantiateChunk(TGenerationData&) construit TChunk par valeur, avec le constructeur de son choix. generate(coord) retourne std::optional<TChunk>. Les échecs sont des spk::Exception et ne sont jamais assimilés à nullopt.
- Les éventuels builders sont propres aux données de génération et ne font pas partie du contrat du Generator.
- Déterminisme attendu de l'exemple : mêmes réglages pertinents + mêmes coordonnées => mêmes données, quel que soit l'ordre des requêtes. Sparkle impose l'ordre des passes mais **pas** une seed unique ni la sécurité des données partagées gérées par l'utilisateur.

## Tests
Aucun Builder requis ; chunk non default-constructible, données move-only, passes ordonnées, données fraîches, nullopt sans passes, exception en n'importe quelle étape, instanciation finale par valeur et déterminisme.

## Exclusions
Ni Task, ni WorkerPool, ni génération automatique de villes/biomes/routes/cavernes, ni contrôle centralisé des structures utilisateur.
