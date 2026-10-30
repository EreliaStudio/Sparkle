# PR-03 — VoxelMesher, clipping et Model

> **Fiche de préparation pour une future PR GitHub, pas une PR déjà créée.** Ne développer que cette fiche.

**Objectif :** Générer un modèle voxel complet et déterministe à partir du Volume.

**Prérequis :** [PR-02](PR-02-voxel-volume.md)

## Spécifications normatives

- [02-modele-voxel.md](../02-modele-voxel.md)
- [03-meshing-occlusion.md](../03-meshing-occlusion.md)
- [08-lumieres-et-ombres.md](../08-lumieres-et-ombres.md)
- [10-contrats-et-validation.md](../10-contrats-et-validation.md)
- [Spécification principale](../../voxel-engine-v0.1.4-specification.md)
- [Contrats et scénarios d'acceptation](../10-contrats-et-validation.md)

## Périmètre exact à réaliser

- [ ] VoxelMesher synchrone
- [ ] voisinage extérieur Empty/Occupied/Unknown
- [ ] clipping polygonal et union des occultants
- [ ] règle innerShell
- [ ] Model immuable
- [ ] opaque/translucide et ShadowMap::Mesh de profondeur
- [ ] slots lexicographiques.

## Instructions techniques et intégration

Utiliser Mesh<T> existant; tolérance de clipping configurable 1e-5; orientation et CCW préservés; interpoler UV/normales, ignorer alpha texture pour occlusion et ombres; opacity slot 1 opaque, <1 translucide; aucune texture nécessaire pour bake.

Avant de coder, inspecter la forme **actuelle** des classes, constructions, tests et CMake ; réutiliser `Mesh<T>`, `Registry`, `ContractProvider`, `ThreadSafeSlot`, `GPUResourceCollection`, `RenderSnapshot::Builder`, `ShaderStorageBuffer`, `spk::Exception` selon pertinence. Les exemples d'API dans les spécifications sont parfois indicatifs : la signature finale doit respecter les invariants sans créer une abstraction parallèle.

## Tests obligatoires

- [ ] Occlusion partielle et polygones opaques superposés
- [ ] faces arrière/translucides non occultantes
- [ ] 6 voisins et 3 états extérieurs
- [ ] innerShell global
- [ ] indices/UV/normales
- [ ] slots totalement masqués et collision d'opacité
- [ ] mesh d'ombre opaque.

**Non-régression commune :** conserver la compilation et les tests existants. Lancer `cmake --workflow --preset testDebug` et `cmake --workflow --preset testRelease` pour les configurations prises en charge, en plus des builds Core sans Graphics lorsque pertinent. Tests GPU et golden-image sur l'infrastructure graphique existante (Windows/WGL) ; ne pas remplacer automatiquement une golden image de référence.

## Définition de terminé / critère de fusion

Meshes position/indices/materialIndex inspectables sans contexte GL; anciens Models immuables.

- [ ] Tous les invariants et erreurs publiques ci-dessus sont testés et documentés ; aucun état partiel n'est publié après un échec lorsqu'une garantie forte est prescrite.
- [ ] Les dépendances listées sont disponibles sur la branche de base ; aucune fonctionnalité des fiches suivantes n'est embarquée sans nécessité stricte.
- [ ] La PR présente le périmètre, les tests réellement exécutés et les résultats, les screenshots/diffs si graphiques, et les limitations éventuelles.
- [ ] Aucune fusion sans validation du propriétaire.

## Hors périmètre

OcclusionCache, greedy meshing, rebake incrémental, shaders.

## Consigne de passation

Confier **uniquement PR-03** à l'IA. Les détails locaux (signatures C++, choix numérique aux bornes, états GPU) peuvent être finalisés pendant l'implémentation avec tests. Ne pas réouvrir les choix validés ni recréer le registre de questions supprimé. En cas de contradiction fonctionnelle publique **démontrée**, soumettre au plus une question ciblée ; poursuivre les éléments indépendants. [Prompt de démarrage générique](AI-HANDOFF.md).
