# PR-02 — Volume dense et Builder

> **Fiche de préparation pour une future PR GitHub, pas une PR déjà créée.** Ne développer que cette fiche.

**Objectif :** Éditer une grille voxel puis publier des snapshots immuables.

**Prérequis :** [PR-01](PR-01-voxel-foundations.md)

## Spécifications normatives

- [02-modele-voxel.md](../02-modele-voxel.md)
- [10-contrats-et-validation.md](../10-contrats-et-validation.md)
- [Spécification principale](../../voxel-engine-v0.1.4-specification.md)
- [Contrats et scénarios d'acceptation](../10-contrats-et-validation.md)

## Périmètre exact à réaliser

- [ ] Volume::Builder depuis dimensions ou Volume existant
- [ ] grille dense compacte
- [ ] table locale de Definitions
- [ ] Air index 0
- [ ] lecture Cell par valeur
- [ ] set et erase
- [ ] tailles immuables.

## Instructions techniques et intégration

Indexation et allocation sans débordement; déduplication par identité Definition::Data; copie légère des snapshots; toute exception build laisse Builder inchangé et réessayable; pas de resize ni de span<Cell>.

Avant de coder, inspecter la forme **actuelle** des classes, constructions, tests et CMake ; réutiliser `Mesh<T>`, `Registry`, `ContractProvider`, `ThreadSafeSlot`, `GPUResourceCollection`, `RenderSnapshot::Builder`, `ShaderStorageBuffer`, `spk::Exception` selon pertinence. Les exemples d'API dans les spécifications sont parfois indicatifs : la signature finale doit respecter les invariants sans créer une abstraction parallèle.

## Tests obligatoires

- [ ] Initialisation Air partout
- [ ] duplication par identité et non par contenu
- [ ] orientation maintenue
- [ ] set/erase et hors bornes
- [ ] overflow
- [ ] ancien Volume après nouveau build
- [ ] construction échouée non destructive.

**Non-régression commune :** conserver la compilation et les tests existants. Lancer `cmake --workflow --preset testDebug` et `cmake --workflow --preset testRelease` pour les configurations prises en charge, en plus des builds Core sans Graphics lorsque pertinent. Tests GPU et golden-image sur l'infrastructure graphique existante (Windows/WGL) ; ne pas remplacer automatiquement une golden image de référence.

## Définition de terminé / critère de fusion

Volume de plusieurs Definitions relu sans Engine ni OpenGL.

- [ ] Tous les invariants et erreurs publiques ci-dessus sont testés et documentés ; aucun état partiel n'est publié après un échec lorsqu'une garantie forte est prescrite.
- [ ] Les dépendances listées sont disponibles sur la branche de base ; aucune fonctionnalité des fiches suivantes n'est embarquée sans nécessité stricte.
- [ ] La PR présente le périmètre, les tests réellement exécutés et les résultats, les screenshots/diffs si graphiques, et les limitations éventuelles.
- [ ] Aucune fusion sans validation du propriétaire.

## Hors périmètre

chunking, grille sparse, streaming, meshing.

## Consigne de passation

Confier **uniquement PR-02** à l'IA. Les détails locaux (signatures C++, choix numérique aux bornes, états GPU) peuvent être finalisés pendant l'implémentation avec tests. Ne pas réouvrir les choix validés ni recréer le registre de questions supprimé. En cas de contradiction fonctionnelle publique **démontrée**, soumettre au plus une question ciblée ; poursuivre les éléments indépendants. [Prompt de démarrage générique](AI-HANDOFF.md).
