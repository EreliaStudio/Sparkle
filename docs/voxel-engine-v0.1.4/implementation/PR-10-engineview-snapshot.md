# PR-10 — EngineView, layers et sous-RenderSnapshots

> **Fiche de préparation pour une future PR GitHub, pas une PR déjà créée.** Ne développer que cette fiche.
>
> **Branche source :** `impl/v0.1.4/10-engineview-snapshot`, à créer depuis la **tête courante de `V0.1.4`** après fusion des prérequis. **Branche cible de la PR GitHub : `V0.1.4`**. Ne jamais cibler `main` ni partir d'une autre branche de feature.

**Objectif :** Construire des snapshots graphiques immuables ordonnés par vues.

**Prérequis :** [PR-09](PR-09-rendering-engine-inheritance.md)

## Spécifications normatives

- [07-vues-et-passes.md](../07-vues-et-passes.md)
- [10-contrats-et-validation.md](../10-contrats-et-validation.md)
- [Spécification principale](../../voxel-engine-v0.1.4-specification.md)
- [Contrats et scénarios d'acceptation](../10-contrats-et-validation.md)

## Périmètre exact à réaliser

- [ ] EngineView nom unique et immutable
- [ ] CameraComponent non propriétaire/détachable
- [ ] ordre de capture
- [ ] layers
- [ ] CaptureContext
- [ ] Builder de sous-snapshot
- [ ] cinq passes réservées
- [ ] ThreadSafeSlot moteur
- [ ] suppression de vue.

## Instructions techniques et intégration

UpdateState séparé de refreshRenderSnapshot; tous jeux de passes construits CPU sans GL; filtres non configuré=tous et configuré vide=aucun; rejeter clés personnalisées invalides; autoriser snapshot historique cohérent après mutation, sauf removeView avant commit; valeurs/handles capturés sans relecture à execute.

Avant de coder, inspecter la forme **actuelle** des classes, constructions, tests et CMake ; réutiliser `Mesh<T>`, `Registry`, `ContractProvider`, `ThreadSafeSlot`, `GPUResourceCollection`, `RenderSnapshot::Builder`, `ShaderStorageBuffer`, `spk::Exception` selon pertinence. Les exemples d'API dans les spécifications sont parfois indicatifs : la signature finale doit respecter les invariants sans créer une abstraction parallèle.

## Tests obligatoires

- [ ] Deux vues mêmes contributeurs avec layers distincts
- [ ] 5 clés système et collision nom/ordre
- [ ] vues ordonnées/non-ordonnées
- [ ] changement de caméra
- [ ] suppression/recréation nom
- [ ] publication échouée conserve précédent
- [ ] snapshot T1 publié après mutation T2.

**Non-régression commune :** conserver la compilation et les tests existants. Lancer `cmake --workflow --preset testDebug` et `cmake --workflow --preset testRelease` pour les configurations prises en charge, en plus des builds Core sans Graphics lorsque pertinent. Tests GPU et golden-image sur l'infrastructure graphique existante (Windows/WGL) ; ne pas remplacer automatiquement une golden image de référence.

## Définition de terminé / critère de fusion

Sous-snapshots lisibles et exécutables via RenderCommand d'essai, sans contexte GL actif pendant capture.

- [ ] Tous les invariants et erreurs publiques ci-dessus sont testés et documentés ; aucun état partiel n'est publié après un échec lorsqu'une garantie forte est prescrite.
- [ ] Les dépendances listées sont disponibles sur la branche de base ; aucune fonctionnalité des fiches suivantes n'est embarquée sans nécessité stricte.
- [ ] La PR présente le périmètre, les tests réellement exécutés et les résultats, les screenshots/diffs si graphiques, et les limitations éventuelles.
- [ ] Aucune fusion sans validation du propriétaire.

## Hors périmètre

ordonnanceur de vues, EngineViewWidget, ouverture implicite de vue main.

## Consigne de passation

Confier **uniquement PR-10** à l'IA. Les détails locaux (signatures C++, choix numérique aux bornes, états GPU) peuvent être finalisés pendant l'implémentation avec tests. Ne pas réouvrir les choix validés ni recréer le registre de questions supprimé. En cas de contradiction fonctionnelle publique **démontrée**, soumettre au plus une question ciblée ; poursuivre les éléments indépendants. [Prompt de démarrage générique](AI-HANDOFF.md).
