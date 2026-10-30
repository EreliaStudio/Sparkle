# PR-26 — Intégration complète et publication v0.1.4

> **Fiche de préparation pour une future PR GitHub, pas une PR déjà créée.** Ne développer que cette fiche.

**Objectif :** Vérifier collectivement toutes les fonctionnalités sans introduire une nouvelle architecture.

**Prérequis :** [PR-04](PR-04-occlusion-cache.md), [PR-08](PR-08-assembly-hierarchy.md), [PR-14](PR-14-assembly-render.md), [PR-15](PR-15-material-view-source.md), [PR-16](PR-16-skybox-component.md), [PR-19](PR-19-forward-lighting.md), [PR-20](PR-20-shadow-contributor.md), [PR-21](PR-21-shadowresults-surfaces.md), [PR-22](PR-22-shadowmaps-spot-directional.md), [PR-23](PR-23-point-cubemaps.md), [PR-24](PR-24-point-shadow-budget.md), [PR-25](PR-25-wboit-hdr.md)

## Spécifications normatives

- [02-modele-voxel.md](../02-modele-voxel.md)
- [03-meshing-occlusion.md](../03-meshing-occlusion.md)
- [04-materiaux-textures-palettes.md](../04-materiaux-textures-palettes.md)
- [05-asset-manager.md](../05-asset-manager.md)
- [06-assemblies-scene.md](../06-assemblies-scene.md)
- [07-vues-et-passes.md](../07-vues-et-passes.md)
- [08-lumieres-et-ombres.md](../08-lumieres-et-ombres.md)
- [09-pipeline-gpu-transparence.md](../09-pipeline-gpu-transparence.md)
- [10-contrats-et-validation.md](../10-contrats-et-validation.md)
- [Spécification principale](../../voxel-engine-v0.1.4-specification.md)
- [Contrats et scénarios d'acceptation](../10-contrats-et-validation.md)

## Périmètre exact à réaliser

- [ ] Scène intégrée Model/Assembly, deux vues, Materials de vues, Skybox, Ambient/Point/Spot/Directional, ShadowMap statique/dynamique, WBOIT, HDR/Reinhard, deux surfaces et cycles resize/fermeture
- [ ] documentation et packaging.

## Instructions techniques et intégration

Réutiliser tests GTest/CTest/Graphics TestLibrary et golden images actuels; aucune réécriture globale ; corriger régressions découvertes avec tests ; vérifier exemples utilisateurs et cibles installées `sparkle::core` et `sparkle::sparkle`.

Avant de coder, inspecter la forme **actuelle** des classes, constructions, tests et CMake ; réutiliser `Mesh<T>`, `Registry`, `ContractProvider`, `ThreadSafeSlot`, `GPUResourceCollection`, `RenderSnapshot::Builder`, `ShaderStorageBuffer`, `spk::Exception` selon pertinence. Les exemples d'API dans les spécifications sont parfois indicatifs : la signature finale doit respecter les invariants sans créer une abstraction parallèle.

## Tests obligatoires

- [ ] Debug/Release et Core Windows/Linux
- [ ] Golden-image scene intégrée revues humain
- [ ] 2 fenêtres séparées et ordre de budget
- [ ] resize/suppression de vues, vie GPU et snapshots en vol
- [ ] compatibilité tests v0.1.3
- [ ] package CMake/vcpkg et exemple client compile.

**Non-régression commune :** conserver la compilation et les tests existants. Lancer `cmake --workflow --preset testDebug` et `cmake --workflow --preset testRelease` pour les configurations prises en charge, en plus des builds Core sans Graphics lorsque pertinent. Tests GPU et golden-image sur l'infrastructure graphique existante (Windows/WGL) ; ne pas remplacer automatiquement une golden image de référence.

## Définition de terminé / critère de fusion

CI existante verte, démonstration complète, documentation de livraison v0.1.4 et rapport de couverture.

- [ ] Tous les invariants et erreurs publiques ci-dessus sont testés et documentés ; aucun état partiel n'est publié après un échec lorsqu'une garantie forte est prescrite.
- [ ] Les dépendances listées sont disponibles sur la branche de base ; aucune fonctionnalité des fiches suivantes n'est embarquée sans nécessité stricte.
- [ ] La PR présente le périmètre, les tests réellement exécutés et les résultats, les screenshots/diffs si graphiques, et les limitations éventuelles.
- [ ] Aucune fusion sans validation du propriétaire.

## Hors périmètre

nouvelles fonctionnalités, nouveau framework de tests, optimisation prématurée.

## Consigne de passation

Confier **uniquement PR-26** à l'IA. Les détails locaux (signatures C++, choix numérique aux bornes, états GPU) peuvent être finalisés pendant l'implémentation avec tests. Ne pas réouvrir les choix validés ni recréer le registre de questions supprimé. En cas de contradiction fonctionnelle publique **démontrée**, soumettre au plus une question ciblée ; poursuivre les éléments indépendants. [Prompt de démarrage générique](AI-HANDOFF.md).
