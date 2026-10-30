# PR-18 — Light::Channel, Top-N et cohérence CPU

> **Fiche de préparation pour une future PR GitHub, pas une PR déjà créée.** Ne développer que cette fiche.

**Objectif :** Fournir une sélection immuable par EngineView cohérente avec ses mutations.

**Prérequis :** [PR-09](PR-09-rendering-engine-inheritance.md), [PR-10](PR-10-engineview-snapshot.md), [PR-17](PR-17-light-components.md)

## Spécifications normatives

- [07-vues-et-passes.md](../07-vues-et-passes.md)
- [08-lumieres-et-ombres.md](../08-lumieres-et-ombres.md)
- [10-contrats-et-validation.md](../10-contrats-et-validation.md)
- [Spécification principale](../../voxel-engine-v0.1.4-specification.md)
- [Contrats et scénarios d'acceptation](../10-contrats-et-validation.md)

## Périmètre exact à réaliser

- [ ] Light::Channel/Prioritizer/Selection
- [ ] N=8/h=0,10
- [ ] ordinals d'inscription
- [ ] conservation et places libres
- [ ] reset sur N/h modifié
- [ ] generations
- [ ] invalidation diffuse
- [ ] verrou moteur→Channel
- [ ] fermeture lors removeView.

## Instructions techniques et intégration

Entourer mutation CPU réelle et invalidation aux points source dans setters de lumières, Transform3D/parents, Registry et caméra/projection, pas seulement par callbacks tardifs ; copie sous verrou moteur, tri sans verrou, validation finale génération; snapshot historique de PR10 reste permis.

Avant de coder, inspecter la forme **actuelle** des classes, constructions, tests et CMake ; réutiliser `Mesh<T>`, `Registry`, `ContractProvider`, `ThreadSafeSlot`, `GPUResourceCollection`, `RenderSnapshot::Builder`, `ShaderStorageBuffer`, `spk::Exception` selon pertinence. Les exemples d'API dans les spécifications sont parfois indicatifs : la signature finale doit respecter les invariants sans créer une abstraction parallèle.

## Tests obligatoires

- [ ] Top-N début et hystérésis
- [ ] candidate plus forte mais sous seuil conserve titulaires
- [ ] place libre sans hystérésis
- [ ] N=0 et Directional maintenues
- [ ] reset h seul
- [ ] deux vues partageant caméra
- [ ] interleaving mutation/commit avec barrières
- [ ] absence de deadlock.

**Non-régression commune :** conserver la compilation et les tests existants. Lancer `cmake --workflow --preset testDebug` et `cmake --workflow --preset testRelease` pour les configurations prises en charge, en plus des builds Core sans Graphics lorsque pertinent. Tests GPU et golden-image sur l'infrastructure graphique existante (Windows/WGL) ; ne pas remplacer automatiquement une golden image de référence.

## Définition de terminé / critère de fusion

Light::Selection stable, mêmes entrées/historique => même résultat; aucun nouveau snapshot moteur imposé pour simple mutation.

- [ ] Tous les invariants et erreurs publiques ci-dessus sont testés et documentés ; aucun état partiel n'est publié après un échec lorsqu'une garantie forte est prescrite.
- [ ] Les dépendances listées sont disponibles sur la branche de base ; aucune fonctionnalité des fiches suivantes n'est embarquée sans nécessité stricte.
- [ ] La PR présente le périmètre, les tests réellement exécutés et les résultats, les screenshots/diffs si graphiques, et les limitations éventuelles.
- [ ] Aucune fusion sans validation du propriétaire.

## Hors périmètre

mutex global ajouté à chaque type, scheduler de light tiles.

## Consigne de passation

Confier **uniquement PR-18** à l'IA. Les détails locaux (signatures C++, choix numérique aux bornes, états GPU) peuvent être finalisés pendant l'implémentation avec tests. Ne pas réouvrir les choix validés ni recréer le registre de questions supprimé. En cas de contradiction fonctionnelle publique **démontrée**, soumettre au plus une question ciblée ; poursuivre les éléments indépendants. [Prompt de démarrage générique](AI-HANDOFF.md).
