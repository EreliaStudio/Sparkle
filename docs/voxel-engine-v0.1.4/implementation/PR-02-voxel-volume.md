# PR-02 — Volume dense et Builder

> **PR GitHub active :** [Draft PR #41](https://github.com/EreliaStudio/Sparkle/pull/41). Ne pas fusionner sans validation du propriétaire.
>
> **Branche source :** `impl/v0.1.4/02-voxel-volume`, créée depuis `V0.1.4`. **Branche cible de la PR GitHub : `V0.1.4`**. Ne jamais cibler `main` ni partir d'une autre branche de feature.

**Objectif :** Éditer une grille voxel puis publier des snapshots immuables.

**Prérequis :** [PR-01](PR-01-voxel-foundations.md)

## Spécifications normatives

- [02-modele-voxel.md](../02-modele-voxel.md)
- [10-contrats-et-validation.md](../10-contrats-et-validation.md)
- [Spécification principale](../../voxel-engine-v0.1.4-specification.md)
- [Contrats et scénarios d'acceptation](../10-contrats-et-validation.md)

## Périmètre exact à réaliser

- [ ] Volume::Builder depuis dimensions ou Volume existant
- [ ] grille dense compacte
- [ ] Registry immuable partagé, sans table locale de Definitions
- [ ] Air ID 0
- [ ] lecture Cell par valeur via `at` et `operator[]`, lecture compacte par valeur via `atRaw`
- [ ] set(position, ID, orientations) et erase
- [ ] tailles immuables.

## Instructions techniques et intégration

ID numérique externe stocké directement dans les 29 bits de chaque cellule, sans remappage; registry immuable partagé par copie O(1); indexation et allocation sans débordement; toute exception de build laisse Builder inchangé et réessayable; pas de resize ni de span<Cell>.

Avant de coder, inspecter la forme **actuelle** des classes, constructions, tests et CMake ; réutiliser `Mesh<T>`, `Registry`, `ContractProvider`, `ThreadSafeSlot`, `GPUResourceCollection`, `RenderSnapshot::Builder`, `ShaderStorageBuffer`, `spk::Exception` selon pertinence. Les exemples d'API dans les spécifications sont parfois indicatifs : la signature finale doit respecter les invariants sans créer une abstraction parallèle.

## Architecture validée : registres immuables et IDs directs

- **`Shape::Registry`** associe `std::string → Shape`. `Shape::Registry::Builder` hérite de `spk::JSON::Catalog<Shape, std::string>` et propose `add`, `load`, `build()`.
- **`Definition::Registry`** associe `std::uint32_t → Definition` avec un ID 0 implicitement Air et des IDs explicites limités à 29 bits. `Definition::Registry::Builder` hérite de `spk::JSON::Catalog<Definition, std::uint32_t>`, peut lier un Shape Registry, charger du JSON et publier un snapshot via `build()`.
- Les deux Registry possèdent un **`shared_ptr<const Data>`**. Les Builders portent la mutation, les snapshots publiés ne sont jamais modifiables ; copier un Registry est **O(1)**. Les Builders restent réutilisables, les snapshots précédents inchangés.
- `Definition::Builder` est construit sans paramètre puis reçoit le `Shape::Registry` publié via `bind(const Shape::Registry&)`. Les deux anciens constructeurs `(Shape::Registry)` et `(Shape::Registry, JSON)` sont supprimés. `addFromJSON(value)` est appelé après la liaison. Les deux Builders qui référencent des Shapes conservent un `Shape::Registry` par défaut (vide), et `Definition::Builder` utilise `std::optional<Shape>` pour représenter la sélection explicite d'une Shape, sans booléen `_hasShape`. L'absence d'un ID texte provoque une `spk::Exception` via le registre. `Definition::Builder` suit explicitement si une Shape a été sélectionnée ; `build()` lance `spk::Exception` sinon. Une Shape vide reste permise si elle a été sélectionnée dans le registre. La Definition canonique Air provient uniquement de `Definition::air()` et de l'ID réservé `0`. Une nouvelle liaison réinitialise l'optional de Shape et annule la sélection, tout en conservant les slots. Les noms de slots sémantiques et la référence JSON `"shape":"nom"` conservent leurs contrats.
- La mémoire publiée des registres utilise `unordered_map` : recherche moyenne **O(1)**, sans `set` ou tableau géant indexé par l'espace d'IDs 29 bits. Aucune optimisation spéculative spécifique au mesher.
- `spk::JSON::Catalog` conserve son format standard `{"elements":[{"id":...,"data":...}]}`. Son paramètre de type ID reste facultatif, avec valeur par défaut `TElement::ID` ; un accesseur protégé fournit les entrées pour construire les snapshots.
- `Volume::Data` défini **directement dans `include/voxel/volume.hpp`** ne conserve que `dimensions`, une **copie légère de `Definition::Registry`** et un `vector<uint32_t>` de cellules compactes. Aucune table d'indices locale n'est conservée.
- Les 29 bits de chaque mot compact représentent **directement l'ID numérique externe** de Definition. Les 3 bits restants encodent les orientations. `Volume::at(position)` et `Volume::operator[](position)` reconstruisent une Cell par valeur via le Registry, et `Volume::atRaw(position)` retourne le mot `PackedCell::Storage` (`uint32_t`) par valeur, sans conversion en Cell. Les trois vérifient les bornes ; `cell(position)` est retirée.
- `Volume::Builder::set(position, Cell)` est **supprimé**. L'API restante est `set(position, ID, horizontal, vertical)` ; un ID absent est rejeté. `erase` remet le mot entier à zéro ; un ID 0 avec orientation non canonique reste légal.
- `Volume::Builder::bind(registry)` remplace le snapshot référencé sans copier le contenu. Si le Builder contient déjà des cellules non-Air, il refuse toute nouvelle liaison qui modifierait l'identité des Definitions désignées par leurs IDs.
- Le chargement d'un fichier compact vérifie strictement les dimensions, la taille `x*y*z*4`, les IDs et le format little-endian ; il importe les mots **sans conversion ni déduplication locale**. L'échec laisse le Builder intact.
- Les erreurs d'accès `at`/`operator[]` identifient l'ID demandé. `tryGet` ne lève pas et renvoie `nullptr` pour les IDs inconnus ; pour `Definition::Registry`, `tryGet(0)` renvoie `&Definition::air()`.
- `Volume::PackedCell` reste dans `spk::Voxel::Volume`, sans namespace `detail` ; la destruction et le partage immuable continuent à reposer sur des données propriétaires explicites. Le contournement `VolumeTestAccess` et ses déclarations `friend` sont supprimés ; les tests lisent l'encodage depuis `atRaw` et vérifient le comportement publié.

### Exemple de chargement

```cpp
Shape::Registry::Builder shapesBuilder;
shapesBuilder.load("shapes.json");
const Shape::Registry shapes = shapesBuilder.build();

Definition::Registry::Builder definitionsBuilder(shapes);
definitionsBuilder.load("definitions.json");
const Definition::Registry definitions = definitionsBuilder.build();

Volume::Builder volumeBuilder({16, 16, 16}, definitions);
volumeBuilder.set({1, 2, 3}, 42);
const Volume volume = volumeBuilder.build();
```

## Format binaire compact

Le constructeur `Volume::Builder(dimensions, filename, registry)` et `loadPacked(filename)` lisent un fichier **brut sans en-tête** de **4 octets par cellule little-endian**. Dimensions explicitement fournies ; parcours X le plus rapide, puis Y, puis Z. Deux bits décrivent la rotation horizontale, un bit l'inversion verticale et 29 bits donnent **directement l'ID** dans le `Definition::Registry` fourni. ID 0 réservé Air.

Un fichier d'une taille incorrecte, un ID non déclaré ou une erreur d'entrée-sortie rejette toute l'opération, sans publier d'état partiel. Ni dimensions, ni version, ni descriptions de Definitions ne sont stockées dans le fichier ; le Registry correspondant est indispensable pour reconstituer les cellules.

## Tests obligatoires

- [ ] Initialisation Air partout
- [ ] copies des registres O(1), identité des Definitions stable et snapshots immuables
- [ ] orientation maintenue
- [ ] set/erase, accès `at`, `operator[]` et `atRaw`, vérification des hors bornes
- [ ] overflow
- [ ] ancien Volume après nouveau build
- [ ] construction échouée non destructive
- [ ] registre JSON des Shapes puis registre JSON des Definitions
- [ ] préservation des IDs externes et rejet des IDs absents
- [ ] modification de Registry::Builder après publication sans mutation rétroactive

**Non-régression commune :** conserver la compilation et les tests existants. Lancer `cmake --workflow --preset testDebug` et `cmake --workflow --preset testRelease` pour les configurations prises en charge, en plus des builds Core sans Graphics lorsque pertinent. Tests GPU et golden-image sur l'infrastructure graphique existante (Windows/WGL) ; ne pas remplacer automatiquement une golden image de référence.

## Définition de terminé / critère de fusion

Volume de plusieurs Definitions relu sans Engine ni OpenGL.

- [ ] Tous les invariants et erreurs publiques ci-dessus sont testés et documentés ; aucun état partiel n'est publié après un échec lorsqu'une garantie forte est prescrite.
- [ ] Les dépendances listées sont disponibles sur la branche de base ; aucune fonctionnalité des fiches suivantes n'est embarquée sans nécessité stricte.
- [ ] La PR présente le périmètre, les tests réellement exécutés et les résultats, les screenshots/diffs si graphiques, et les limitations éventuelles.
- [ ] Aucune fusion sans validation du propriétaire.

## Hors périmètre

chunking, grille sparse, streaming, meshing.

## Consigne de passation

Confier **uniquement PR-02** à l'IA. Les détails locaux (signatures C++, choix numérique aux bornes, états GPU) peuvent être finalisés pendant l'implémentation avec tests. Ne pas réouvrir les choix validés ni recréer le registre de questions supprimé. En cas de contradiction fonctionnelle publique **démontrée**, soumettre au plus une question ciblée ; poursuivre les éléments indépendants. [Prompt de démarrage générique](AI-HANDOFF.md).
