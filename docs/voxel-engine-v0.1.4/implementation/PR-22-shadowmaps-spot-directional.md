# PR-22 — ShadowMap::RenderingSystem pour Spot/Directional

> **Fiche de préparation pour une future PR GitHub, pas une PR déjà créée.** Ne développer que cette fiche.

**Objectif :** Produire des ombres 2D avec caches depth statiques et finals, puis éclairer avec.

**Prérequis :** [PR-17](PR-17-light-components.md), [PR-18](PR-18-light-channel.md), [PR-19](PR-19-forward-lighting.md), [PR-20](PR-20-shadow-contributor.md), [PR-21](PR-21-shadowresults-surfaces.md)

## Spécifications normatives

- [08-lumieres-et-ombres.md](../08-lumieres-et-ombres.md)
- [09-pipeline-gpu-transparence.md](../09-pipeline-gpu-transparence.md)
- [07-vues-et-passes.md](../07-vues-et-passes.md)
- [10-contrats-et-validation.md](../10-contrats-et-validation.md)
- [Spécification principale](../../voxel-engine-v0.1.4-specification.md)
- [Contrats et scénarios d'acceptation](../10-contrats-et-validation.md)

## Périmètre exact à réaliser

- [ ] ShadowMap::RenderingSystem, passe unique d'ombres par vue, caches D32F statique/final par lumière/vue/surface
- [ ] casters Static et Dynamic
- [ ] copie depth
- [ ] projections Spot perspective, Directional orthographique issue du frustum tronqué à shadowDistance
- [ ] PCF 3x3.

## Instructions techniques et intégration

Mesher visuel ne fournit jamais la passe d'ombres; le rendu depth n'a pas UV/alpha; movement Dynamic ne rebake que finale; géométrie historique de carte gardée avec matrices/projections; ancienne carte compatible en attendant; lumière éclaire sans ombre si pas de carte.

Avant de coder, inspecter la forme **actuelle** des classes, constructions, tests et CMake ; réutiliser `Mesh<T>`, `Registry`, `ContractProvider`, `ThreadSafeSlot`, `GPUResourceCollection`, `RenderSnapshot::Builder`, `ShaderStorageBuffer`, `spk::Exception` selon pertinence. Les exemples d'API dans les spécifications sont parfois indicatifs : la signature finale doit respecter les invariants sans créer une abstraction parallèle.

## Tests obligatoires

- [ ] Golden caster Spot et Directional
- [ ] transformation dynamique, statique, suppression
- [ ] 2 surfaces ressources isolées
- [ ] ShadowMode None
- [ ] projecteurs multiples dans une seule RenderPass
- [ ] PCF neuf comparaisons
- [ ] projection far infinie bornée shadowDistance.

**Non-régression commune :** conserver la compilation et les tests existants. Lancer `cmake --workflow --preset testDebug` et `cmake --workflow --preset testRelease` pour les configurations prises en charge, en plus des builds Core sans Graphics lorsque pertinent. Tests GPU et golden-image sur l'infrastructure graphique existante (Windows/WGL) ; ne pas remplacer automatiquement une golden image de référence.

## Définition de terminé / critère de fusion

Premières ombres visibles, 2D, avec cache et publication atomique.

- [ ] Tous les invariants et erreurs publiques ci-dessus sont testés et documentés ; aucun état partiel n'est publié après un échec lorsqu'une garantie forte est prescrite.
- [ ] Les dépendances listées sont disponibles sur la branche de base ; aucune fonctionnalité des fiches suivantes n'est embarquée sans nécessité stricte.
- [ ] La PR présente le périmètre, les tests réellement exécutés et les résultats, les screenshots/diffs si graphiques, et les limitations éventuelles.
- [ ] Aucune fusion sans validation du propriétaire.

## Hors périmètre

CSM, source alpha-aware, séparation ShadowMap par voxel.

## Consigne de passation

Confier **uniquement PR-22** à l'IA. Les détails locaux (signatures C++, choix numérique aux bornes, états GPU) peuvent être finalisés pendant l'implémentation avec tests. Ne pas réouvrir les choix validés ni recréer le registre de questions supprimé. En cas de contradiction fonctionnelle publique **démontrée**, soumettre au plus une question ciblée ; poursuivre les éléments indépendants. [Prompt de démarrage générique](AI-HANDOFF.md).
