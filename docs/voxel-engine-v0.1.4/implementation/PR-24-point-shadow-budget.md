# PR-24 — Sélection et budget PointLight multi-vues

> **Fiche de préparation pour une future PR GitHub, pas une PR déjà créée.** Ne développer que cette fiche.

**Objectif :** Limiter à chaque image/surface les reconstructions intégrales de cubemaps selon la priorité réelle.

**Prérequis :** [PR-18](PR-18-light-channel.md), [PR-21](PR-21-shadowresults-surfaces.md), [PR-22](PR-22-shadowmaps-spot-directional.md), [PR-23](PR-23-point-cubemaps.md)

## Spécifications normatives

- [08-lumieres-et-ombres.md](../08-lumieres-et-ombres.md)
- [07-vues-et-passes.md](../07-vues-et-passes.md)
- [09-pipeline-gpu-transparence.md](../09-pipeline-gpu-transparence.md)
- [10-contrats-et-validation.md](../10-contrats-et-validation.md)
- [Spécification principale](../../voxel-engine-v0.1.4-specification.md)
- [Contrats et scénarios d'acceptation](../10-contrats-et-validation.md)

## Périmètre exact à réaliser

- [ ] Point sous-sélection de Channel X=2/Y=4/epsilon=0,10/h=0,10
- [ ] éviction/réadmission des caches
- [ ] budget reconstruction default 1
- [ ] commande préalable du snapshot moteur
- [ ] état plan éphémère RenderContext
- [ ] diagnostic raisons.

## Instructions techniques et intégration

Scores déjà calculés, top X stricts et suppléments dans tolérance; hystérésis sur suppléments; préparer demandes toutes vues avant toute passe; ordre score sans bonus attente; paire statique+finale six faces=1 unité; vérifier admission avant reconstruction et après; anciennes cartes restent utilisables si admissibles.

Avant de coder, inspecter la forme **actuelle** des classes, constructions, tests et CMake ; réutiliser `Mesh<T>`, `Registry`, `ContractProvider`, `ThreadSafeSlot`, `GPUResourceCollection`, `RenderSnapshot::Builder`, `ShaderStorageBuffer`, `spk::Exception` selon pertinence. Les exemples d'API dans les spécifications sont parfois indicatifs : la signature finale doit respecter les invariants sans créer une abstraction parallèle.

## Tests obligatoires

- [ ] Budget 1 avec vue tardive score fort gagnante
- [ ] zéro reconstructions préserve carte valide
- [ ] réadmission utilise nouveaux State
- [ ] égalité vue/ordinal
- [ ] dédoublonnage
- [ ] image suivante re-évalue
- [ ] absence d'étoilevation favorisée
- [ ] aucune publication partielle.

**Non-régression commune :** conserver la compilation et les tests existants. Lancer `cmake --workflow --preset testDebug` et `cmake --workflow --preset testRelease` pour les configurations prises en charge, en plus des builds Core sans Graphics lorsque pertinent. Tests GPU et golden-image sur l'infrastructure graphique existante (Windows/WGL) ; ne pas remplacer automatiquement une golden image de référence.

## Définition de terminé / critère de fusion

Deux EngineView partagent budget sur une Surface sans changer leur ordre d'affichage.

- [ ] Tous les invariants et erreurs publiques ci-dessus sont testés et documentés ; aucun état partiel n'est publié après un échec lorsqu'une garantie forte est prescrite.
- [ ] Les dépendances listées sont disponibles sur la branche de base ; aucune fonctionnalité des fiches suivantes n'est embarquée sans nécessité stricte.
- [ ] La PR présente le périmètre, les tests réellement exécutés et les résultats, les screenshots/diffs si graphiques, et les limitations éventuelles.
- [ ] Aucune fusion sans validation du propriétaire.

## Hors périmètre

targetFPS adaptatif, round robin, réparation automatique de starvation, LRU d'ombres.

## Consigne de passation

Confier **uniquement PR-24** à l'IA. Les détails locaux (signatures C++, choix numérique aux bornes, états GPU) peuvent être finalisés pendant l'implémentation avec tests. Ne pas réouvrir les choix validés ni recréer le registre de questions supprimé. En cas de contradiction fonctionnelle publique **démontrée**, soumettre au plus une question ciblée ; poursuivre les éléments indépendants. [Prompt de démarrage générique](AI-HANDOFF.md).
