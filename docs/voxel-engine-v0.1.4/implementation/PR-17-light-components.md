# PR-17 — Point, Spot, Directional et scores

> **Fiche de préparation pour une future PR GitHub, pas une PR déjà créée.** Ne développer que cette fiche.

**Objectif :** Introduire les sources lumineuses directes et leur mathématique CPU.

**Prérequis :** [PR-09](PR-09-rendering-engine-inheritance.md)

## Spécifications normatives

- [08-lumieres-et-ombres.md](../08-lumieres-et-ombres.md)
- [10-contrats-et-validation.md](../10-contrats-et-validation.md)
- [Spécification principale](../../voxel-engine-v0.1.4-specification.md)
- [Contrats et scénarios d'acceptation](../10-contrats-et-validation.md)

## Périmètre exact à réaliser

- [ ] Light::Contributor/Point/Spot/Directional Component3D
- [ ] couleur RGB/intensity
- [ ] range et angle Spot
- [ ] priorityWeight
- [ ] volumes frustum sphère et cône
- [ ] priorité score
- [ ] shadowDistance Directional
- [ ] shadowResolution 1024 D32F.

## Instructions techniques et intégration

Distance en U ; Smoothstep radiale A=1-3x²+2x³, Spot angulaire; culling conservateur; Directional hors Top-N; pas inverse carré; ordinals monotones à l'enregistrement moteur, sans dépendre de Registry order.

Avant de coder, inspecter la forme **actuelle** des classes, constructions, tests et CMake ; réutiliser `Mesh<T>`, `Registry`, `ContractProvider`, `ThreadSafeSlot`, `GPUResourceCollection`, `RenderSnapshot::Builder`, `ShaderStorageBuffer`, `spk::Exception` selon pertinence. Les exemples d'API dans les spécifications sont parfois indicatifs : la signature finale doit respecter les invariants sans créer une abstraction parallèle.

## Tests obligatoires

- [ ] Range=0 pas lumière/ombre
- [ ] angle Spot et projection near/far
- [ ] score finite, quadruple avec range*2
- [ ] égalité par ordinal inscription
- [ ] types RGB bornés et intensity non plafonnée
- [ ] invalid values refusées.

**Non-régression commune :** conserver la compilation et les tests existants. Lancer `cmake --workflow --preset testDebug` et `cmake --workflow --preset testRelease` pour les configurations prises en charge, en plus des builds Core sans Graphics lorsque pertinent. Tests GPU et golden-image sur l'infrastructure graphique existante (Windows/WGL) ; ne pas remplacer automatiquement une golden image de référence.

## Définition de terminé / critère de fusion

Trois types lumineux utilisables dans des tests sans rendu d'ombres.

- [ ] Tous les invariants et erreurs publiques ci-dessus sont testés et documentés ; aucun état partiel n'est publié après un échec lorsqu'une garantie forte est prescrite.
- [ ] Les dépendances listées sont disponibles sur la branche de base ; aucune fonctionnalité des fiches suivantes n'est embarquée sans nécessité stricte.
- [ ] La PR présente le périmètre, les tests réellement exécutés et les résultats, les screenshots/diffs si graphiques, et les limitations éventuelles.
- [ ] Aucune fusion sans validation du propriétaire.

## Hors périmètre

Forward shader, Tiled Forward+, nouvelle hiérarchie de transforms.

## Consigne de passation

Confier **uniquement PR-17** à l'IA. Les détails locaux (signatures C++, choix numérique aux bornes, états GPU) peuvent être finalisés pendant l'implémentation avec tests. Ne pas réouvrir les choix validés ni recréer le registre de questions supprimé. En cas de contradiction fonctionnelle publique **démontrée**, soumettre au plus une question ciblée ; poursuivre les éléments indépendants. [Prompt de démarrage générique](AI-HANDOFF.md).
