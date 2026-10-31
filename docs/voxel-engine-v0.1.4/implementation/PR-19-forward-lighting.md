# PR-19 — Lighting::System et éclairage Forward

> **Fiche de préparation pour une future PR GitHub, pas une PR déjà créée.** Ne développer que cette fiche.
>
> **Branche source :** `impl/v0.1.4/19-forward-lighting`, à créer depuis la **tête courante de `V0.1.4`** après fusion des prérequis. **Branche cible de la PR GitHub : `V0.1.4`**. Ne jamais cibler `main` ni partir d'une autre branche de feature.

**Objectif :** Éclairer les meshes opaques à partir d'une sélection globale par vue.

**Prérequis :** [PR-12](PR-12-enginewidget-first-view.md), [PR-13](PR-13-voxel-render-first.md), [PR-17](PR-17-light-components.md), [PR-18](PR-18-light-channel.md)

## Spécifications normatives

- [08-lumieres-et-ombres.md](../08-lumieres-et-ombres.md)
- [09-pipeline-gpu-transparence.md](../09-pipeline-gpu-transparence.md)
- [07-vues-et-passes.md](../07-vues-et-passes.md)
- [10-contrats-et-validation.md](../10-contrats-et-validation.md)
- [Spécification principale](../../voxel-engine-v0.1.4-specification.md)
- [Contrats et scénarios d'acceptation](../10-contrats-et-validation.md)

## Périmètre exact à réaliser

- [ ] Lighting::System
- [ ] AmbientLight par EngineView à RGB blanc/intensity=0
- [ ] SSBO lumières et états de shader Point/Spot/Directional
- [ ] données capturées des lumières
- [ ] fallback sans shadow map.

## Instructions techniques et intégration

Opaque calcul linéaire HDR, sans clamp avant Reinhard; toutes les sources directes issues du même Selection par vue; AmbientLight non ombrée; réactiver programme et tous SSBO aux bindings choisis par pipeline à chaque draw.

Avant de coder, inspecter la forme **actuelle** des classes, constructions, tests et CMake ; réutiliser `Mesh<T>`, `Registry`, `ContractProvider`, `ThreadSafeSlot`, `GPUResourceCollection`, `RenderSnapshot::Builder`, `ShaderStorageBuffer`, `spk::Exception` selon pertinence. Les exemples d'API dans les spécifications sont parfois indicatifs : la signature finale doit respecter les invariants sans créer une abstraction parallèle.

## Tests obligatoires

- [ ] Golden un modèle sous Ambient seul puis Point/Spot/Directional
- [ ] score/Top-N influence vue mais non une liste par objet
- [ ] portée/angle/intensité
- [ ] ambiance autre vue indépendante
- [ ] changements après capture n'altèrent pas ancien snapshot
- [ ] bindings après commande tierce.

**Non-régression commune :** conserver la compilation et les tests existants. Lancer `cmake --workflow --preset testDebug` et `cmake --workflow --preset testRelease` pour les configurations prises en charge, en plus des builds Core sans Graphics lorsque pertinent. Tests GPU et golden-image sur l'infrastructure graphique existante (Windows/WGL) ; ne pas remplacer automatiquement une golden image de référence.

## Définition de terminé / critère de fusion

Première scène voxel éclairée sans dépendre d'une shadow map.

- [ ] Tous les invariants et erreurs publiques ci-dessus sont testés et documentés ; aucun état partiel n'est publié après un échec lorsqu'une garantie forte est prescrite.
- [ ] Les dépendances listées sont disponibles sur la branche de base ; aucune fonctionnalité des fiches suivantes n'est embarquée sans nécessité stricte.
- [ ] La PR présente le périmètre, les tests réellement exécutés et les résultats, les screenshots/diffs si graphiques, et les limitations éventuelles.
- [ ] Aucune fusion sans validation du propriétaire.

## Hors périmètre

Deferred/Tiled, cache GPU de lumière global, ombres (PR22+).

## Consigne de passation

Confier **uniquement PR-19** à l'IA. Les détails locaux (signatures C++, choix numérique aux bornes, états GPU) peuvent être finalisés pendant l'implémentation avec tests. Ne pas réouvrir les choix validés ni recréer le registre de questions supprimé. En cas de contradiction fonctionnelle publique **démontrée**, soumettre au plus une question ciblée ; poursuivre les éléments indépendants. [Prompt de démarrage générique](AI-HANDOFF.md).
