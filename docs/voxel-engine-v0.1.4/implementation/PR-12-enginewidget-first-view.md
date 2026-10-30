# PR-12 — EngineWidget et première EngineView visible

> **Fiche de préparation pour une future PR GitHub, pas une PR déjà créée.** Ne développer que cette fiche.

**Objectif :** Afficher une vue et sa sortie HDR finale dans un widget sans dessin voxel.

**Prérequis :** [PR-09](PR-09-rendering-engine-inheritance.md), [PR-10](PR-10-engineview-snapshot.md), [PR-11](PR-11-gpu-attachments-surfaces.md)

## Spécifications normatives

- [07-vues-et-passes.md](../07-vues-et-passes.md)
- [09-pipeline-gpu-transparence.md](../09-pipeline-gpu-transparence.md)
- [10-contrats-et-validation.md](../10-contrats-et-validation.md)
- [Spécification principale](../../voxel-engine-v0.1.4-specification.md)
- [Contrats et scénarios d'acceptation](../10-contrats-et-validation.md)

## Périmètre exact à réaliser

- [ ] EngineWidget avec rôles Update/Render/Drawing configurables
- [ ] référence RenderingEngine*
- [ ] choix view name main par défaut sans création
- [ ] fond RGB
- [ ] AmbientLight possédée avec intensité initiale 0
- [ ] exposition=1, tone mapping Reinhard
- [ ] sortie SRGB8_ALPHA8 opaque
- [ ] retour UI.

## Instructions techniques et intégration

Le jeu contrôle explicitement taille EngineView/projection caméra; pas de resize automatique; 5 phases initialisées même vides; compose couleur de fond HDR puis Reinhard vers texture finale; alpha toujours 1; restore GL après chaque délégation et avant UI.

Avant de coder, inspecter la forme **actuelle** des classes, constructions, tests et CMake ; réutiliser `Mesh<T>`, `Registry`, `ContractProvider`, `ThreadSafeSlot`, `GPUResourceCollection`, `RenderSnapshot::Builder`, `ShaderStorageBuffer`, `spk::Exception` selon pertinence. Les exemples d'API dans les spécifications sont parfois indicatifs : la signature finale doit respecter les invariants sans créer une abstraction parallèle.

## Tests obligatoires

- [ ] Golden fond noir et couleur choisie
- [ ] Reinhard pour 0/1/4 à E=1 et à E=2
- [ ] image EngineView réutilisable dans deux widgets
- [ ] Drawing sans Render/Update
- [ ] capture sur demande
- [ ] framebuffer UI correctement restauré
- [ ] valeur invalide rejetée.

**Non-régression commune :** conserver la compilation et les tests existants. Lancer `cmake --workflow --preset testDebug` et `cmake --workflow --preset testRelease` pour les configurations prises en charge, en plus des builds Core sans Graphics lorsque pertinent. Tests GPU et golden-image sur l'infrastructure graphique existante (Windows/WGL) ; ne pas remplacer automatiquement une golden image de référence.

## Définition de terminé / critère de fusion

Première texture EngineView réellement visible via EngineWidget.

- [ ] Tous les invariants et erreurs publiques ci-dessus sont testés et documentés ; aucun état partiel n'est publié après un échec lorsqu'une garantie forte est prescrite.
- [ ] Les dépendances listées sont disponibles sur la branche de base ; aucune fonctionnalité des fiches suivantes n'est embarquée sans nécessité stricte.
- [ ] La PR présente le périmètre, les tests réellement exécutés et les résultats, les screenshots/diffs si graphiques, et les limitations éventuelles.
- [ ] Aucune fusion sans validation du propriétaire.

## Hors périmètre

Mesh voxel, lighting direct, skybox, auto exposure.

## Consigne de passation

Confier **uniquement PR-12** à l'IA. Les détails locaux (signatures C++, choix numérique aux bornes, états GPU) peuvent être finalisés pendant l'implémentation avec tests. Ne pas réouvrir les choix validés ni recréer le registre de questions supprimé. En cas de contradiction fonctionnelle publique **démontrée**, soumettre au plus une question ciblée ; poursuivre les éléments indépendants. [Prompt de démarrage générique](AI-HANDOFF.md).
