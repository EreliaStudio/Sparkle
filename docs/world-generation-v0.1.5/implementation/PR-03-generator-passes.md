# PR03 — Generator et passes de génération typées
**Couche :** Core. **Dépendances :** PR01. **État :** cahier des charges.

## Contrat public
- Generator<TChunk,TGenerationData> est une recette synchrone. Aucun Builder ni constructeur particulier n'est imposé au chunk.
- Pas de Generator::Builder de configuration : le générateur expose addPass<TPass>(...) pour ajouter en fin et insertPass<TPass>(index, ...) pour insérer avant la passe à l'index donné. Les index commencent à zéro ; un index égal au nombre de passes ajoute en fin, un index supérieur déclenche une exception. Le générateur crée et possède les passes (pas de surcharge prenant std::unique_ptr). Elles sont configurées avant les générations concurrentes.
- Pass::apply(const ChunkCoordinate&, TGenerationData&) est virtuelle. Les coordonnées sont transmises à chaque passe.
- Le hook virtuel protégé _prepareGenerationData(coord,data) retourne bool (true par défaut). false signifie absence valide ; en cas de refus ou d'exception il ne laisse aucun état temporaire.
- TGenerationData est une instance persistante détenue par le Generator. Elle peut contenir caches globaux, données de régions, colonnes, builders et scratch par coordonnée. Elle peut être non copiable et non déplaçable ; la synchronisation est à sa charge.
- Exécuter toutes les passes dans l'ordre ; `_instantiateChunk(coord,data)` construit TChunk par valeur. `_cleanGenerationData(coord,data) noexcept` nettoie les données temporaires après succès ou échec, si la préparation a réussi. generate(coord) retourne optional<TChunk> ; les erreurs ne sont pas des absences.
- Les éventuels builders sont propres aux données de génération et ne font pas partie du contrat du Generator.
- Déterminisme attendu de l'exemple : mêmes réglages pertinents + mêmes coordonnées => mêmes données, quel que soit l'ordre des requêtes. Sparkle impose l'ordre des passes mais **pas** une seed unique ni la sécurité des données partagées gérées par l'utilisateur.

## Tests
Aucun Builder requis ; chunk non default-constructible, données move-only, passes ordonnées, état persistant, scratch indépendant et nettoyage après succès ou échec, nullopt sans passes, exception en n'importe quelle étape, instanciation finale par valeur et déterminisme.

## Exclusions
Ni Task, ni WorkerPool, ni génération automatique de villes/biomes/routes/cavernes, ni contrôle centralisé des structures utilisateur.
