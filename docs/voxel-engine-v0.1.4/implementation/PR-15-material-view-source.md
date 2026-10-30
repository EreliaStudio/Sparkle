# PR-15 — EngineView comme source de Material

> **Fiche de préparation pour une future PR GitHub, pas une PR déjà créée.** Ne développer que cette fiche.

**Objectif :** Permettre l'usage de la sortie finale d'une EngineView par un Material dans une autre vue.

**Prérequis :** [PR-06](PR-06-materials-json.md), [PR-10](PR-10-engineview-snapshot.md), [PR-11](PR-11-gpu-attachments-surfaces.md), [PR-12](PR-12-enginewidget-first-view.md), [PR-13](PR-13-voxel-render-first.md)

## Spécifications normatives

- [04-materiaux-textures-palettes.md](../04-materiaux-textures-palettes.md)
- [07-vues-et-passes.md](../07-vues-et-passes.md)
- [09-pipeline-gpu-transparence.md](../09-pipeline-gpu-transparence.md)
- [10-contrats-et-validation.md](../10-contrats-et-validation.md)
- [Spécification principale](../../voxel-engine-v0.1.4-specification.md)
- [Contrats et scénarios d'acceptation](../10-contrats-et-validation.md)

## Périmètre exact à réaliser

- [ ] Material::Builder source EngineView& explicite en C++
- [ ] identité incarnation de vue
- [ ] Texture::Handle finale tone-mappée
- [ ] dépendance de vues sans boucle
- [ ] DetachedOutputPolicy Invalid/KeepLastFrame/DefaultTexture
- [ ] fallback configuré.

## Instructions techniques et intégration

Choix logique à capture CPU, validité réelle GPU par Surface et version; pas de HDR exposé; même identité malgré resize/fallback, source différente si vue détruite et homonyme recréée; ordre producteur avant consommateur imposé; aucune lecture feedback.

Avant de coder, inspecter la forme **actuelle** des classes, constructions, tests et CMake ; réutiliser `Mesh<T>`, `Registry`, `ContractProvider`, `ThreadSafeSlot`, `GPUResourceCollection`, `RenderSnapshot::Builder`, `ShaderStorageBuffer`, `spk::Exception` selon pertinence. Les exemples d'API dans les spécifications sont parfois indicatifs : la signature finale doit respecter les invariants sans créer une abstraction parallèle.

## Tests obligatoires

- [ ] Surveillance avant main fonctionne première image sur Surface B
- [ ] ordre inverse/cycle refusé CPU
- [ ] même texture widget/material
- [ ] sortie absente sur B échoue GPU
- [ ] trois politiques détachées
- [ ] changement de résolution préserve snapshot historique.

**Non-régression commune :** conserver la compilation et les tests existants. Lancer `cmake --workflow --preset testDebug` et `cmake --workflow --preset testRelease` pour les configurations prises en charge, en plus des builds Core sans Graphics lorsque pertinent. Tests GPU et golden-image sur l'infrastructure graphique existante (Windows/WGL) ; ne pas remplacer automatiquement une golden image de référence.

## Définition de terminé / critère de fusion

Vue source utilisée comme albedo dans une autre vue sans gamma incorrect.

- [ ] Tous les invariants et erreurs publiques ci-dessus sont testés et documentés ; aucun état partiel n'est publié après un échec lorsqu'une garantie forte est prescrite.
- [ ] Les dépendances listées sont disponibles sur la branche de base ; aucune fonctionnalité des fiches suivantes n'est embarquée sans nécessité stricte.
- [ ] La PR présente le périmètre, les tests réellement exécutés et les résultats, les screenshots/diffs si graphiques, et les limitations éventuelles.
- [ ] Aucune fusion sans validation du propriétaire.

## Hors périmètre

réordonnancement automatique des vues, lien EngineView depuis JSON.

## Consigne de passation

Confier **uniquement PR-15** à l'IA. Les détails locaux (signatures C++, choix numérique aux bornes, états GPU) peuvent être finalisés pendant l'implémentation avec tests. Ne pas réouvrir les choix validés ni recréer le registre de questions supprimé. En cas de contradiction fonctionnelle publique **démontrée**, soumettre au plus une question ciblée ; poursuivre les éléments indépendants. [Prompt de démarrage générique](AI-HANDOFF.md).
