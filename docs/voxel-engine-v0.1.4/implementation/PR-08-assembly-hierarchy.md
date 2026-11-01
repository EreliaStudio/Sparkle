# PR-08 — Voxel::Assembly par copie et expansion

> **Fiche de préparation pour une future PR GitHub, pas une PR déjà créée.** Ne développer que cette fiche.
>
> **Branche source :** `impl/v0.1.4/08-assembly-hierarchy`, à créer depuis la **tête courante de `V0.1.4`** après fusion des prérequis. **Branche cible de la PR GitHub : `V0.1.4`**. Ne jamais cibler `main` ni partir d'une autre branche de feature.

**Objectif :** Composer une hiérarchie de Parts animables sans Entity par Part.

**Prérequis :** [PR-07](PR-07-voxel-palette.md)

## Spécifications normatives

- [06-assemblies-scene.md](../06-assemblies-scene.md)
- [10-contrats-et-validation.md](../10-contrats-et-validation.md)
- [Spécification principale](../../voxel-engine-v0.1.4-specification.md)
- [Contrats et scénarios d'acceptation](../10-contrats-et-validation.md)

## Périmètre exact à réaliser

- [x] Assembly, PartID, transformations locales et relations parentales
- [x] add/remove/reparent
- [x] insertion par copie et expansion avec ancrage
- [x] partage des Models/Palettes
- [x] VersionedTrait indépendant.

## Instructions techniques et intégration

PartID monotones non réutilisés; réaffecter IDs uniquement à la copie importée; refuser cycles et auto-insertion; source jamais liée vivante; opération d'insertion garantie forte; calcul de pose locale cumulé, aucune propagation de ShadowMode.

Avant de coder, inspecter la forme **actuelle** des classes, constructions, tests et CMake ; réutiliser `Mesh<T>`, `Registry`, `ContractProvider`, `ThreadSafeSlot`, `GPUResourceCollection`, `RenderSnapshot::Builder`, `ShaderStorageBuffer`, `spk::Exception` selon pertinence. Les exemples d'API dans les spécifications sont parfois indicatifs : la signature finale doit respecter les invariants sans créer une abstraction parallèle.

## Tests obligatoires

- [ ] Source avec plusieurs racines ou vide
- [ ] insertion deux fois IDs différents
- [ ] reparentage garde ID
- [ ] suppression sous-arbre
- [ ] matrices non commutatives
- [ ] source détruite n'affecte pas destination
- [ ] erreur avant commit et version inchangée.

**Non-régression commune :** conserver la compilation et les tests existants. Lancer `cmake --workflow --preset testDebug` et `cmake --workflow --preset testRelease` pour les configurations prises en charge, en plus des builds Core sans Graphics lorsque pertinent. Tests GPU et golden-image sur l'infrastructure graphique existante (Windows/WGL) ; ne pas remplacer automatiquement une golden image de référence.

## Définition de terminé / critère de fusion

Une Assembly articulée modifiable et copiable indépendamment.

- [ ] Tous les invariants et erreurs publiques ci-dessus sont testés et documentés ; aucun état partiel n'est publié après un échec lorsqu'une garantie forte est prescrite.
- [ ] Les dépendances listées sont disponibles sur la branche de base ; aucune fonctionnalité des fiches suivantes n'est embarquée sans nécessité stricte.
- [ ] La PR présente le périmètre, les tests réellement exécutés et les résultats, les screenshots/diffs si graphiques, et les limitations éventuelles.
- [ ] Aucune fusion sans validation du propriétaire.

## Hors périmètre

Binding automatique des ombres, Engine Entity pour chaque membre.

## Consigne de passation

Confier **uniquement PR-08** à l'IA. Les détails locaux (signatures C++, choix numérique aux bornes, états GPU) peuvent être finalisés pendant l'implémentation avec tests. Ne pas réouvrir les choix validés ni recréer le registre de questions supprimé. En cas de contradiction fonctionnelle publique **démontrée**, soumettre au plus une question ciblée ; poursuivre les éléments indépendants. [Prompt de démarrage générique](AI-HANDOFF.md).

## API retenue pour PR-08

- `spk::Voxel::PartID` : entier 64 bits monotone, attribué à partir de 1 et jamais réattribué.
- `Assembly::Part` : `id`, `name`, `parent`, `localTransform`, `std::optional<Model> model` (palette déjà liée dans le Model).
- `addPart(name, parent = nullopt, model = nullopt, transform = identity())`, `part(id) const`, `size()`, `empty()`, `contains(id)`.
- `setName`, `setModel`, `setLocalTransform`, `setParent`, `removePart`.
- `roots()`, `children(id)`, `partIDs()` et `accumulatedTransform(id)`.
- `insertAssembly(parent, source, anchorTransform = identity(), anchorName = "attachment")` renvoie `{anchor, partIDs}`.

Implémentation dans `include/voxel/assembly.hpp` et `src/graphics/voxel/assembly.cpp` ; tests `tests/TU/Graphics/srcs/voxel/voxel_assembly_test.cpp`. L'Assembly appartient à la bibliothèque graphique, à côté du Model, mais ne contient aucune opération GPU. La copie repart à version 1 sans recopier les abonnements. Les modifications des Parts ne modifient pas automatiquement la version.

Voir l'exemple de composition dans [06-assemblies-scene.md](../06-assemblies-scene.md).
