# PR-14 — AssemblyRenderingComponent et instancing articulé

> **Fiche de préparation pour une future PR GitHub, pas une PR déjà créée.** Ne développer que cette fiche.

**Objectif :** Afficher plusieurs Parts d'Assembly sans rémailler.

**Prérequis :** [PR-08](PR-08-assembly-hierarchy.md), [PR-13](PR-13-voxel-render-first.md)

## Spécifications normatives

- [06-assemblies-scene.md](../06-assemblies-scene.md)
- [09-pipeline-gpu-transparence.md](../09-pipeline-gpu-transparence.md)
- [07-vues-et-passes.md](../07-vues-et-passes.md)
- [10-contrats-et-validation.md](../10-contrats-et-validation.md)
- [Spécification principale](../../voxel-engine-v0.1.4-specification.md)
- [Contrats et scénarios d'acceptation](../10-contrats-et-validation.md)

## Périmètre exact à réaliser

- [ ] AssemblyRenderingComponent non propriétaire
- [ ] capture des Parts et de leurs matrices
- [ ] M_entityWorld * M_assembly * M_partAccumulate
- [ ] regroupement de buckets compatibles dans VoxelRenderingSystem.

## Instructions techniques et intégration

Réutiliser le rendu PR13; chaque vue refait sa collecte; matrices capturées avant GPU; pas d'Entity par Part ni SSBO partagé entre deux snapshots; préserver layer du component et validation de compatibilité palette.

Avant de coder, inspecter la forme **actuelle** des classes, constructions, tests et CMake ; réutiliser `Mesh<T>`, `Registry`, `ContractProvider`, `ThreadSafeSlot`, `GPUResourceCollection`, `RenderSnapshot::Builder`, `ShaderStorageBuffer`, `spk::Exception` selon pertinence. Les exemples d'API dans les spécifications sont parfois indicatifs : la signature finale doit respecter les invariants sans créer une abstraction parallèle.

## Tests obligatoires

- [ ] Golden bras/torse articulé
- [ ] transformation parent + enfant non commutatives
- [ ] reparentage
- [ ] deux Parts même mesh à transforms différents
- [ ] palette différente implique bucket distinct
- [ ] anciennes captures inchangées.

**Non-régression commune :** conserver la compilation et les tests existants. Lancer `cmake --workflow --preset testDebug` et `cmake --workflow --preset testRelease` pour les configurations prises en charge, en plus des builds Core sans Graphics lorsque pertinent. Tests GPU et golden-image sur l'infrastructure graphique existante (Windows/WGL) ; ne pas remplacer automatiquement une golden image de référence.

## Définition de terminé / critère de fusion

Une scène d'Assembly animée reste stable et instanciée.

- [ ] Tous les invariants et erreurs publiques ci-dessus sont testés et documentés ; aucun état partiel n'est publié après un échec lorsqu'une garantie forte est prescrite.
- [ ] Les dépendances listées sont disponibles sur la branche de base ; aucune fonctionnalité des fiches suivantes n'est embarquée sans nécessité stricte.
- [ ] La PR présente le périmètre, les tests réellement exécutés et les résultats, les screenshots/diffs si graphiques, et les limitations éventuelles.
- [ ] Aucune fusion sans validation du propriétaire.

## Hors périmètre

rebake sur animation, ombres liées automatiquement.

## Consigne de passation

Confier **uniquement PR-14** à l'IA. Les détails locaux (signatures C++, choix numérique aux bornes, états GPU) peuvent être finalisés pendant l'implémentation avec tests. Ne pas réouvrir les choix validés ni recréer le registre de questions supprimé. En cas de contradiction fonctionnelle publique **démontrée**, soumettre au plus une question ciblée ; poursuivre les éléments indépendants. [Prompt de démarrage générique](AI-HANDOFF.md).
