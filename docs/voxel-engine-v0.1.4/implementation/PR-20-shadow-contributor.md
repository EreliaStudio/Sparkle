# PR-20 — ShadowMap::Contributor et MeshFilter

> **Fiche de préparation pour une future PR GitHub, pas une PR déjà créée.** Ne développer que cette fiche.

**Objectif :** Définir des sources d'ombres géométriques explicites indépendantes du visuel.

**Prérequis :** [PR-03](PR-03-voxel-mesher-model.md), [PR-09](PR-09-rendering-engine-inheritance.md)

## Spécifications normatives

- [08-lumieres-et-ombres.md](../08-lumieres-et-ombres.md)
- [03-meshing-occlusion.md](../03-meshing-occlusion.md)
- [06-assemblies-scene.md](../06-assemblies-scene.md)
- [10-contrats-et-validation.md](../10-contrats-et-validation.md)
- [Spécification principale](../../voxel-engine-v0.1.4-specification.md)
- [Contrats et scénarios d'acceptation](../10-contrats-et-validation.md)

## Périmètre exact à réaliser

- [ ] ShadowMap::Mesh position seule et factory extracteur si non déjà livrée PR03
- [ ] MeshInstance
- [ ] MeshFilter collection + baseTransform
- [ ] ShadowMode None/Static/Dynamic
- [ ] Contributor avec layers et notification de bind.

## Instructions techniques et intégration

Le binding remplace atomiquement toutes instances+baseTransform ; matrice monde Entity*base*instance ; bind(collection) remet base à identité ; aucune propagation parent/enfant ShadowMode, aucun caster créé par VoxelRenderingSystem; opaques seulement et aucune alpha texture.

Avant de coder, inspecter la forme **actuelle** des classes, constructions, tests et CMake ; réutiliser `Mesh<T>`, `Registry`, `ContractProvider`, `ThreadSafeSlot`, `GPUResourceCollection`, `RenderSnapshot::Builder`, `ShaderStorageBuffer`, `spk::Exception` selon pertinence. Les exemples d'API dans les spécifications sont parfois indicatifs : la signature finale doit respecter les invariants sans créer une abstraction parallèle.

## Tests obligatoires

- [ ] Factory conserve indices/winding
- [ ] matrices non finies refusées avant commit
- [ ] bind identique notifie une seule fois
- [ ] Dynamic invalide finale, Static statique+finale
- [ ] vieux snapshots conservent meshes
- [ ] parent None avec enfant Dynamic
- [ ] layers distincts du visuel.

**Non-régression commune :** conserver la compilation et les tests existants. Lancer `cmake --workflow --preset testDebug` et `cmake --workflow --preset testRelease` pour les configurations prises en charge, en plus des builds Core sans Graphics lorsque pertinent. Tests GPU et golden-image sur l'infrastructure graphique existante (Windows/WGL) ; ne pas remplacer automatiquement une golden image de référence.

## Définition de terminé / critère de fusion

Contributor peut projeter n'importe quel Mesh opaque explicitement fourni par utilisateur.

- [ ] Tous les invariants et erreurs publiques ci-dessus sont testés et documentés ; aucun état partiel n'est publié après un échec lorsqu'une garantie forte est prescrite.
- [ ] Les dépendances listées sont disponibles sur la branche de base ; aucune fonctionnalité des fiches suivantes n'est embarquée sans nécessité stricte.
- [ ] La PR présente le périmètre, les tests réellement exécutés et les résultats, les screenshots/diffs si graphiques, et les limitations éventuelles.
- [ ] Aucune fusion sans validation du propriétaire.

## Hors périmètre

shadows automatiques pour Assembly, cutout albedo, modes hérités.

## Consigne de passation

Confier **uniquement PR-20** à l'IA. Les détails locaux (signatures C++, choix numérique aux bornes, états GPU) peuvent être finalisés pendant l'implémentation avec tests. Ne pas réouvrir les choix validés ni recréer le registre de questions supprimé. En cas de contradiction fonctionnelle publique **démontrée**, soumettre au plus une question ciblée ; poursuivre les éléments indépendants. [Prompt de démarrage générique](AI-HANDOFF.md).
