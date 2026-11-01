# PR-11 — Attachments partagés et ressources GPU par Surface

> **Fiche de préparation pour une future PR GitHub, pas une PR déjà créée.** Ne développer que cette fiche.
>
> **Branche source :** `impl/v0.1.4/11-gpu-attachments-surfaces`, à créer depuis la **tête courante de `V0.1.4`** après fusion des prérequis. **Branche cible de la PR GitHub : `V0.1.4`**. Ne jamais cibler `main` ni partir d'une autre branche de feature.

**Objectif :** Rendre sûrs Texture/Framebuffer versionnés et la récupération GPU en multifenêtres.

**Prérequis :** Aucune PR d'implémentation requise.

## Spécifications normatives

- [07-vues-et-passes.md](../07-vues-et-passes.md)
- [08-lumieres-et-ombres.md](../08-lumieres-et-ombres.md)
- [09-pipeline-gpu-transparence.md](../09-pipeline-gpu-transparence.md)
- [10-contrats-et-validation.md](../10-contrats-et-validation.md)
- [Spécification principale](../../voxel-engine-v0.1.4-specification.md)
- [Contrats et scénarios d'acceptation](../10-contrats-et-validation.md)

## Périmètre exact à réaliser

- [ ] Texture render-target 2D et profondeur cubemap
- [ ] formats RGBA16F, SRGB8_ALPHA8 et Depth32F
- [ ] Framebuffer avec Texture::Handle explicites partagés
- [ ] depth-only et face explicite
- [ ] resize non destructif
- [ ] cycle Surface Ready/Releasing/Released
- [ ] reclaim par tick.

## Instructions techniques et intégration

Réutiliser GPUResourceCollection et ses pools, aucun nouveau manager; un State capturé ne change jamais lors du resize; `Framebuffer::resize` sur partage explicite refuse; matérialiser par contexte; makeCurrent avant reclaimReleased chaque tick Ready même sans redraw; notifier fermeture avant GPU clear puis WGL.

Avant de coder, inspecter la forme **actuelle** des classes, constructions, tests et CMake ; réutiliser `Mesh<T>`, `Registry`, `ContractProvider`, `ThreadSafeSlot`, `GPUResourceCollection`, `RenderSnapshot::Builder`, `ShaderStorageBuffer`, `spk::Exception` selon pertinence. Les exemples d'API dans les spécifications sont parfois indicatifs : la signature finale doit respecter les invariants sans créer une abstraction parallèle.

## Tests obligatoires

- [ ] Deux FBO ayant exactement depth handle partagé
- [ ] anciens State valides après resize
- [ ] texture cubemap 6 faces carrées
- [ ] capacité GL absente => Exception
- [ ] deux surfaces sans partage native
- [ ] reclaim sans consommateur
- [ ] fermeture et callbacks exceptions suivis du clear.

**Non-régression commune :** conserver la compilation et les tests existants. Lancer `cmake --workflow --preset testDebug` et `cmake --workflow --preset testRelease` pour les configurations prises en charge, en plus des builds Core sans Graphics lorsque pertinent. Tests GPU et golden-image sur l'infrastructure graphique existante (Windows/WGL) ; ne pas remplacer automatiquement une golden image de référence.

## Définition de terminé / critère de fusion

Test GPU confirme depth unique dans opaque/WBOIT et absence de mutation des snapshots anciens.

- [ ] Tous les invariants et erreurs publiques ci-dessus sont testés et documentés ; aucun état partiel n'est publié après un échec lorsqu'une garantie forte est prescrite.
- [ ] Les dépendances listées sont disponibles sur la branche de base ; aucune fonctionnalité des fiches suivantes n'est embarquée sans nécessité stricte.
- [ ] La PR présente le périmètre, les tests réellement exécutés et les résultats, les screenshots/diffs si graphiques, et les limitations éventuelles.
- [ ] Aucune fusion sans validation du propriétaire.

## Hors périmètre

GLsync spécialisé, allocation GPU sur thread Update, deuxième gestionnaire de ressources.

## Consigne de passation

Confier **uniquement PR-11** à l'IA. Les détails locaux (signatures C++, choix numérique aux bornes, états GPU) peuvent être finalisés pendant l'implémentation avec tests. Ne pas réouvrir les choix validés ni recréer le registre de questions supprimé. En cas de contradiction fonctionnelle publique **démontrée**, soumettre au plus une question ciblée ; poursuivre les éléments indépendants. [Prompt de démarrage générique](AI-HANDOFF.md).
