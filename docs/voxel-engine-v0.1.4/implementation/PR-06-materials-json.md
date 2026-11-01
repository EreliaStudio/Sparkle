# PR-06 — Material multi-rôles, Texture et SpriteSheet

> **Fiche de préparation pour une future PR GitHub, pas une PR déjà créée.** Ne développer que cette fiche.
>
> **Branche source :** `impl/v0.1.4/06-materials-json`, à créer depuis la **tête courante de `V0.1.4`** après fusion des prérequis. **Branche cible de la PR GitHub : `V0.1.4`**. Ne jamais cibler `main` ni partir d'une autre branche de feature.

**Objectif :** Construire des Materials immuables via Builder ou JSON.

**Prérequis :** [PR-05](PR-05-asset-manager.md)

## Spécifications normatives

- [04-materiaux-textures-palettes.md](../04-materiaux-textures-palettes.md)
- [05-asset-manager.md](../05-asset-manager.md)
- [10-contrats-et-validation.md](../10-contrats-et-validation.md)
- [Spécification principale](../../voxel-engine-v0.1.4-specification.md)
- [Contrats et scénarios d'acceptation](../10-contrats-et-validation.md)

## Périmètre exact à réaliser

- [ ] Material et Builder
- [ ] rôles nommés libres avec Texture::Handle + Texture::Section
- [ ] SpriteSheet existant
- [ ] racine JSON {textures:{...}}
- [ ] sources path ou asset typé
- [ ] UV whole, sprite ou anchor/size
- [ ] factory Material.

## Instructions techniques et intégration

Utiliser Assets explicite pour JSON; bind(nullptr) lève Exception; parser de texte refuse clés dupliquées; path+size charge SpriteSheet; rôle normal sans logique spéciale; un objet Material vide en rôles reste génériquement valide; sources EngineView non exposées dans JSON.

Avant de coder, inspecter la forme **actuelle** des classes, constructions, tests et CMake ; réutiliser `Mesh<T>`, `Registry`, `ContractProvider`, `ThreadSafeSlot`, `GPUResourceCollection`, `RenderSnapshot::Builder`, `ShaderStorageBuffer`, `spk::Exception` selon pertinence. Les exemples d'API dans les spécifications sont parfois indicatifs : la signature finale doit respecter les invariants sans créer une abstraction parallèle.

## Tests obligatoires

- [ ] Builder manuel == JSON
- [ ] deux grilles d'un atlas chargées indépendamment
- [ ] UV sprite hors domaine refusée
- [ ] path/asset équivalents même triplet
- [ ] doublons textuels rejetés
- [ ] rôle albedo absent accepté par Material mais pas par consommateur.

**Non-régression commune :** conserver la compilation et les tests existants. Lancer `cmake --workflow --preset testDebug` et `cmake --workflow --preset testRelease` pour les configurations prises en charge, en plus des builds Core sans Graphics lorsque pertinent. Tests GPU et golden-image sur l'infrastructure graphique existante (Windows/WGL) ; ne pas remplacer automatiquement une golden image de référence.

## Définition de terminé / critère de fusion

Une source texture unique résolue en Material sans GPU actif lors du Builder.

- [ ] Tous les invariants et erreurs publiques ci-dessus sont testés et documentés ; aucun état partiel n'est publié après un échec lorsqu'une garantie forte est prescrite.
- [ ] Les dépendances listées sont disponibles sur la branche de base ; aucune fonctionnalité des fiches suivantes n'est embarquée sans nécessité stricte.
- [ ] La PR présente le périmètre, les tests réellement exécutés et les résultats, les screenshots/diffs si graphiques, et les limitations éventuelles.
- [ ] Aucune fusion sans validation du propriétaire.

## Hors périmètre

VoxelPalette, Shader, source EngineView depuis JSON.

## Consigne de passation

Confier **uniquement PR-06** à l'IA. Les détails locaux (signatures C++, choix numérique aux bornes, états GPU) peuvent être finalisés pendant l'implémentation avec tests. Ne pas réouvrir les choix validés ni recréer le registre de questions supprimé. En cas de contradiction fonctionnelle publique **démontrée**, soumettre au plus une question ciblée ; poursuivre les éléments indépendants. [Prompt de démarrage générique](AI-HANDOFF.md).
