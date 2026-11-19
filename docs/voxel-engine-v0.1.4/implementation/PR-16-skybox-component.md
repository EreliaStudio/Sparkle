# PR-16 — Skybox unlit à six Materials

> **Livrée : [PR #67](https://github.com/EreliaStudio/Sparkle/pull/67) fusionnée dans `V0.1.4`.** Les checklists ci-dessous sont historiques et ne sont pas des tâches ouvertes.
>
> **Branche historique :** `impl/v0.1.4/16-skybox-component`, déjà fusionnée dans `V0.1.4` via #67.

**Objectif :** Ajouter un Skybox générique comme composant RenderingComponent3D ordinaire.

**Prérequis :** [PR-06](PR-06-materials-json.md), [PR-10](PR-10-engineview-snapshot.md), [PR-11](PR-11-gpu-attachments-surfaces.md), [PR-12](PR-12-enginewidget-first-view.md), [PR-15](PR-15-material-view-source.md)

## Adaptation à la baseline PR13 (obligatoire)

La passe Skybox est déjà définie et initialisée par `EngineView` depuis PR10–12. Ajouter seulement le composant et son dessin générique à six Materials ; réutiliser la source EngineView introduite en PR15. **Aucun Model3D ou adaptateur PR14 n'est nécessaire**.

## Spécifications normatives

- [07-vues-et-passes.md](../07-vues-et-passes.md)
- [04-materiaux-textures-palettes.md](../04-materiaux-textures-palettes.md)
- [09-pipeline-gpu-transparence.md](../09-pipeline-gpu-transparence.md)
- [10-contrats-et-validation.md](../10-contrats-et-validation.md)
- [Spécification principale](../../voxel-engine-v0.1.4-specification.md)
- [Contrats et scénarios d'acceptation](../10-contrats-et-validation.md)

## Périmètre historique

- [ ] Skybox sur Entity3D
- [ ] cube inversé centré caméra
- [ ] six Materials génériques séparés et six sections UV
- [ ] même identité albedo obligatoire
- [ ] contribution à passe Skybox -1
- [ ] pas de profondeur ni lumière.

## Instructions techniques et intégration

Validation des six sources communes lors de capture CPU; pas de pointeur Skybox détenu par EngineView; capturer ressources/UV par valeur ; le retrait/désactivation ne touche que prochaines captures ; source EngineView admise si ordre valide.

Avant de coder, inspecter la forme **actuelle** des classes, constructions, tests et CMake ; réutiliser `Mesh<T>`, `Registry`, `ContractProvider`, `ThreadSafeSlot`, `GPUResourceCollection`, `RenderSnapshot::Builder`, `ShaderStorageBuffer`, `spk::Exception` selon pertinence. Les exemples d'API dans les spécifications sont parfois indicatifs : la signature finale doit respecter les invariants sans créer une abstraction parallèle.

## Tests historiques

- [ ] Six faces d'un atlas
- [ ] source différente ou albedo manquante => Exception sans publication
- [ ] Golden ciel différent du fond
- [ ] changer lumières ne change pas ciel
- [ ] remove component sans use-after-free
- [ ] plusieurs vues/layers.

**Non-régression commune :** conserver la compilation et les tests existants. Lancer `cmake --workflow --preset testDebug` et `cmake --workflow --preset testRelease` pour les configurations prises en charge, en plus des builds Core sans Graphics lorsque pertinent. Tests GPU et golden-image sur l'infrastructure graphique existante (Windows/WGL) ; ne pas remplacer automatiquement une golden image de référence.

## Définition de terminé / critère de fusion

Skybox dessiné en HDR avant opaque, inchangé par éclairage, alpha finale 1.

- [ ] Tous les invariants et erreurs publiques ci-dessus sont testés et documentés ; aucun état partiel n'est publié après un échec lorsqu'une garantie forte est prescrite.
- [ ] Les dépendances listées sont disponibles sur la branche de base ; aucune fonctionnalité des fiches suivantes n'est embarquée sans nécessité stricte.
- [ ] La PR présente le périmètre, les tests réellement exécutés et les résultats, les screenshots/diffs si graphiques, et les limitations éventuelles.
- [ ] Aucune fusion sans validation du propriétaire.

## Hors périmètre

SkyboxMaterial, glTextureCube couleur, EngineView.setSkybox.

## Consigne de passation

Confier **uniquement PR-16** à l'IA. Les détails locaux (signatures C++, choix numérique aux bornes, états GPU) peuvent être finalisés pendant l'implémentation avec tests. Ne pas réouvrir les choix validés ni recréer le registre de questions supprimé. En cas de contradiction fonctionnelle publique **démontrée**, soumettre au plus une question ciblée ; poursuivre les éléments indépendants. [Prompt de démarrage générique](AI-HANDOFF.md).

## Résultat réel fusionné (#67)

`Skybox` sur `RenderingComponent3D`, six Materials de même source `albedo`, UV propres et rendu unlit HDR. `CameraUniformRenderCommand` capture projection, view, inverseView et viewProjection dans un UBO `CameraData` (`std140`, binding 1), partagé avec le voxel. Tests et goldens Skybox livrés.

Les nouvelles PR réutilisent ce code livré et ne recommencent pas cette ancienne fiche.
