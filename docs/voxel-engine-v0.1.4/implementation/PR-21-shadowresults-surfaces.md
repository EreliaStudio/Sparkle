# PR-21 — ShadowResults, admission et durée de vie des surfaces

> **Fiche de préparation pour une future PR GitHub, pas une PR déjà créée.** Ne développer que cette fiche.

**Objectif :** Publier les résultats de shadow maps de façon sûre et isolée par Surface.

**Prérequis :** [PR-10](PR-10-engineview-snapshot.md), [PR-11](PR-11-gpu-attachments-surfaces.md), [PR-18](PR-18-light-channel.md)

## Spécifications normatives

- [07-vues-et-passes.md](../07-vues-et-passes.md)
- [08-lumieres-et-ombres.md](../08-lumieres-et-ombres.md)
- [09-pipeline-gpu-transparence.md](../09-pipeline-gpu-transparence.md)
- [10-contrats-et-validation.md](../10-contrats-et-validation.md)
- [Spécification principale](../../voxel-engine-v0.1.4-specification.md)
- [Contrats et scénarios d'acceptation](../10-contrats-et-validation.md)

## Périmètre exact à réaliser

- [ ] Light::ShadowResult immutable, ShadowResults dictionnaire SurfaceIdentity→Entry
- [ ] ThreadSafeSlot par Surface
- [ ] admission/générations/ordinal de capture
- [ ] callback fermeture Surface
- [ ] removeView ferme Channel.

## Instructions techniques et intégration

Un seul producteur GPU par surface, publication copie/fusion lumière par lumière; mutex dictionnaire court séparé du mutex Channel; aucune opération GL sous verrou; CPU revoke interdit admission, jamais second producteur Slot; fermeture notifiée après arrêt des rendus avant GPU clear; Channel détruit naturellement.

Avant de coder, inspecter la forme **actuelle** des classes, constructions, tests et CMake ; réutiliser `Mesh<T>`, `Registry`, `ContractProvider`, `ThreadSafeSlot`, `GPUResourceCollection`, `RenderSnapshot::Builder`, `ShaderStorageBuffer`, `spk::Exception` selon pertinence. Les exemples d'API dans les spécifications sont parfois indicatifs : la signature finale doit respecter les invariants sans créer une abstraction parallèle.

## Tests obligatoires

- [ ] Deux surfaces indépendant même nom de fenêtre recréé
- [ ] fusion A+B n'écrase pas A
- [ ] ancien snapshot refuse carte issue du futur
- [ ] admission révoquée empêche publication
- [ ] exception callback n'interrompt pas les suivants ni clear
- [ ] évite cycle shared_ptr.

**Non-régression commune :** conserver la compilation et les tests existants. Lancer `cmake --workflow --preset testDebug` et `cmake --workflow --preset testRelease` pour les configurations prises en charge, en plus des builds Core sans Graphics lorsque pertinent. Tests GPU et golden-image sur l'infrastructure graphique existante (Windows/WGL) ; ne pas remplacer automatiquement une golden image de référence.

## Définition de terminé / critère de fusion

Anciens snapshots restent utilisables avec handles logiques mais sans accéder à Surface fermée.

- [ ] Tous les invariants et erreurs publiques ci-dessus sont testés et documentés ; aucun état partiel n'est publié après un échec lorsqu'une garantie forte est prescrite.
- [ ] Les dépendances listées sont disponibles sur la branche de base ; aucune fonctionnalité des fiches suivantes n'est embarquée sans nécessité stricte.
- [ ] La PR présente le périmètre, les tests réellement exécutés et les résultats, les screenshots/diffs si graphiques, et les limitations éventuelles.
- [ ] Aucune fusion sans validation du propriétaire.

## Hors périmètre

mutex de fusion des résultats, GL cleanup ShadowMap spécialisé, purge à removeView.

## Consigne de passation

Confier **uniquement PR-21** à l'IA. Les détails locaux (signatures C++, choix numérique aux bornes, états GPU) peuvent être finalisés pendant l'implémentation avec tests. Ne pas réouvrir les choix validés ni recréer le registre de questions supprimé. En cas de contradiction fonctionnelle publique **démontrée**, soumettre au plus une question ciblée ; poursuivre les éléments indépendants. [Prompt de démarrage générique](AI-HANDOFF.md).
