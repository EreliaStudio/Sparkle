# PR-01 — Shape, Definition et Cell

> **Fiche de préparation pour une future PR GitHub, pas une PR déjà créée.** Ne développer que cette fiche.

**Objectif :** Constituer le modèle voxel public sans Engine et sans OpenGL.

**Prérequis :** Aucune PR d'implémentation requise.

## Spécifications normatives

- [02-modele-voxel.md](../02-modele-voxel.md)
- [03-meshing-occlusion.md](../03-meshing-occlusion.md)
- [10-contrats-et-validation.md](../10-contrats-et-validation.md)
- [Spécification principale](../../voxel-engine-v0.1.4-specification.md)
- [Contrats et scénarios d'acceptation](../10-contrats-et-validation.md)

## Périmètre exact à réaliser

- [ ] Shape et Builder
- [ ] polygones CCW plans convexes
- [ ] slots géométriques opacity
- [ ] outerShell et innerShell
- [ ] Definition et correspondance de slots
- [ ] Definition Air
- [ ] Cell par valeur et 8 orientations.

## Instructions techniques et intégration

Valider avant chaque ajout avec garantie forte; conserver positions dans [0,1]^3, UV arbitraires et warning Logger; bloc de données partagé immuable et identité distincte entre constructions; flip vertical conserve winding et normales; build réussi consomme Builder.

Avant de coder, inspecter la forme **actuelle** des classes, constructions, tests et CMake ; réutiliser `Mesh<T>`, `Registry`, `ContractProvider`, `ThreadSafeSlot`, `GPUResourceCollection`, `RenderSnapshot::Builder`, `ShaderStorageBuffer`, `spk::Exception` selon pertinence. Les exemples d'API dans les spécifications sont parfois indicatifs : la signature finale doit respecter les invariants sans créer une abstraction parallèle.

## Tests obligatoires

- [ ] Shape vide et polygones invalides
- [ ] Builder inchangé après échec
- [ ] deux Shapes de même contenu mais identités différentes
- [ ] UV hors [0,1] avertis
- [ ] 8 orientations géométrie UV normale winding
- [ ] Air sans face.

**Non-régression commune :** conserver la compilation et les tests existants. Lancer `cmake --workflow --preset testDebug` et `cmake --workflow --preset testRelease` pour les configurations prises en charge, en plus des builds Core sans Graphics lorsque pertinent. Tests GPU et golden-image sur l'infrastructure graphique existante (Windows/WGL) ; ne pas remplacer automatiquement une golden image de référence.

## Définition de terminé / critère de fusion

Exemple de cube par Definition et 8 Cell orientées vérifié numériquement en tests CPU.

- [ ] Tous les invariants et erreurs publiques ci-dessus sont testés et documentés ; aucun état partiel n'est publié après un échec lorsqu'une garantie forte est prescrite.
- [ ] Les dépendances listées sont disponibles sur la branche de base ; aucune fonctionnalité des fiches suivantes n'est embarquée sans nécessité stricte.
- [ ] La PR présente le périmètre, les tests réellement exécutés et les résultats, les screenshots/diffs si graphiques, et les limitations éventuelles.
- [ ] Aucune fusion sans validation du propriétaire.

## Hors périmètre

Volume, Model, maillage GPU, cache par orientation.

## Consigne de passation

Confier **uniquement PR-01** à l'IA. Les détails locaux (signatures C++, choix numérique aux bornes, états GPU) peuvent être finalisés pendant l'implémentation avec tests. Ne pas réouvrir les choix validés ni recréer le registre de questions supprimé. En cas de contradiction fonctionnelle publique **démontrée**, soumettre au plus une question ciblée ; poursuivre les éléments indépendants. [Prompt de démarrage générique](AI-HANDOFF.md).
