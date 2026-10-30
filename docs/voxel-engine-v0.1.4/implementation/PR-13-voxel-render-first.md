# PR-13 — VoxelRenderingSystem et rendu de Model opaque

> **Fiche de préparation pour une future PR GitHub, pas une PR déjà créée.** Ne développer que cette fiche.
>
> **Branche source :** `impl/v0.1.4/13-voxel-render-first`, à créer depuis la **tête courante de `V0.1.4`** après fusion des prérequis. **Branche cible de la PR GitHub : `V0.1.4`**. Ne jamais cibler `main` ni partir d'une autre branche de feature.

**Objectif :** Dessiner un Model voxel texturé dans sa première EngineView.

**Prérequis :** [PR-03](PR-03-voxel-mesher-model.md), [PR-07](PR-07-voxel-palette.md), [PR-10](PR-10-engineview-snapshot.md), [PR-11](PR-11-gpu-attachments-surfaces.md), [PR-12](PR-12-enginewidget-first-view.md)

## Spécifications normatives

- [06-assemblies-scene.md](../06-assemblies-scene.md)
- [04-materiaux-textures-palettes.md](../04-materiaux-textures-palettes.md)
- [09-pipeline-gpu-transparence.md](../09-pipeline-gpu-transparence.md)
- [07-vues-et-passes.md](../07-vues-et-passes.md)
- [10-contrats-et-validation.md](../10-contrats-et-validation.md)
- [Spécification principale](../../voxel-engine-v0.1.4-specification.md)
- [Contrats et scénarios d'acceptation](../10-contrats-et-validation.md)

## Périmètre exact à réaliser

- [ ] ModelRenderingComponent empruntant Model/Palette
- [ ] VoxelRenderingSystem
- [ ] filtrage Registry/layers
- [ ] mesh opaque et atlas
- [ ] buckets, DrawVoxelBucketCommand
- [ ] nouveaux SSBO d'instances/matériaux par snapshot
- [ ] instancing Matrix4x4.

## Instructions techniques et intégration

Program, ShaderStorageBuffer et Mesh<T> existants; bindings par pipeline avant chaque draw; slots materialIndex sans table GPU; clone snapshot immuable; scale uniformément positif dans instance, normals re-normalisées; pas de lecture modèle mutable en execute.

Avant de coder, inspecter la forme **actuelle** des classes, constructions, tests et CMake ; réutiliser `Mesh<T>`, `Registry`, `ContractProvider`, `ThreadSafeSlot`, `GPUResourceCollection`, `RenderSnapshot::Builder`, `ShaderStorageBuffer`, `spk::Exception` selon pertinence. Les exemples d'API dans les spécifications sont parfois indicatifs : la signature finale doit respecter les invariants sans créer une abstraction parallèle.

## Tests obligatoires

- [ ] Golden cube texturé et orienté
- [ ] mêmes modèles atlas différents ne batchent pas
- [ ] instance matrix 64 octets/offsets std430 validés
- [ ] anciens SSBO non modifiés
- [ ] réactivation des bindings après draw tiers
- [ ] violation scale rejetée.

**Non-régression commune :** conserver la compilation et les tests existants. Lancer `cmake --workflow --preset testDebug` et `cmake --workflow --preset testRelease` pour les configurations prises en charge, en plus des builds Core sans Graphics lorsque pertinent. Tests GPU et golden-image sur l'infrastructure graphique existante (Windows/WGL) ; ne pas remplacer automatiquement une golden image de référence.

## Définition de terminé / critère de fusion

Premier Model voxel visible et pixels de golden-image approuvés manuellement.

- [ ] Tous les invariants et erreurs publiques ci-dessus sont testés et documentés ; aucun état partiel n'est publié après un échec lorsqu'une garantie forte est prescrite.
- [ ] Les dépendances listées sont disponibles sur la branche de base ; aucune fonctionnalité des fiches suivantes n'est embarquée sans nécessité stricte.
- [ ] La PR présente le périmètre, les tests réellement exécutés et les résultats, les screenshots/diffs si graphiques, et les limitations éventuelles.
- [ ] Aucune fusion sans validation du propriétaire.

## Hors périmètre

WBOIT, lumières, ShadowMap, gestion persistante offset d'instance.

## Consigne de passation

Confier **uniquement PR-13** à l'IA. Les détails locaux (signatures C++, choix numérique aux bornes, états GPU) peuvent être finalisés pendant l'implémentation avec tests. Ne pas réouvrir les choix validés ni recréer le registre de questions supprimé. En cas de contradiction fonctionnelle publique **démontrée**, soumettre au plus une question ciblée ; poursuivre les éléments indépendants. [Prompt de démarrage générique](AI-HANDOFF.md).
