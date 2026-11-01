# PR-07 — Palette générique et validateur voxel, compatibilité des Models

> **PR GitHub Draft #48** : implémentation sur `impl/v0.1.4/07-voxel-palette`, cible `V0.1.4` ; ne pas fusionner sans validation.
>
> **Branche source :** `impl/v0.1.4/07-voxel-palette`, à créer depuis la **tête courante de `V0.1.4`** après fusion des prérequis. **Branche cible de la PR GitHub : `V0.1.4`**. Ne jamais cibler `main` ni partir d'une autre branche de feature.

**Objectif :** Associer les slots sémantiques de Models à des Materials sans rebake.

**Prérequis :** [PR-04](PR-04-voxel-mesher-model.md), [PR-06](PR-06-materials-json.md)

## Spécifications normatives

- [04-materiaux-textures-palettes.md](../04-materiaux-textures-palettes.md)
- [03-meshing-occlusion.md](../03-meshing-occlusion.md)
- [10-contrats-et-validation.md](../10-contrats-et-validation.md)
- [Spécification principale](../../voxel-engine-v0.1.4-specification.md)
- [Contrats et scénarios d'acceptation](../10-contrats-et-validation.md)

## Périmètre exact à réaliser

- [x] `spk::Palette` immuable : collection générique de Materials nommés.
- [x] `spk::Palette` : unique valeur de palette, Builder générique avec validateur optionnel ; `spk::Voxel::Palette::Builder` fournit les règles voxel.
- [x] Aucune opacité dans les palettes ; `Voxel::Definition` et `Voxel::Model` portent seuls ce contrat.
- [x] `Model::isCompatible(const spk::Palette &)` vérifie uniquement les noms de slots ; le validateur de `spk::Voxel::Palette::Builder` impose immédiatement un atlas unique à chaque insertion.

## Instructions techniques et intégration

Laisser chaque `Model` vérifier les noms de slots nécessaires, même occultés ; créer et construire une palette avant tout Model, en autorisant des noms librement choisis. Ignorer les matériaux surnuméraires lors de la vérification des slots du Model ; imposer un même atlas `albedo` à tous les Materials de la palette, y compris les matériaux surnuméraires, au moment de chaque `setMaterial()`. Aucun SSBO ni mesh additionnel possédé par les palettes.

Avant de coder, inspecter la forme **actuelle** des classes, constructions, tests et CMake ; réutiliser `Mesh<T>`, `Registry`, `ContractProvider`, `ThreadSafeSlot`, `GPUResourceCollection`, `RenderSnapshot::Builder`, `ShaderStorageBuffer`, `spk::Exception` selon pertinence. Les exemples d'API dans les spécifications sont parfois indicatifs : la signature finale doit respecter les invariants sans créer une abstraction parallèle.

## Révision architecturale validée pendant la PR

La palette générique sans validateur n'impose aucun rôle de texture. Le validateur installé par `spk::Voxel::Palette::Builder` exige `albedo` pour chaque Material ajouté ; `build()` n'impose aucune complétude. Le contrôle de l'atlas unique appartient au validateur de `spk::Voxel::Palette::Builder`, exécuté à chaque `setMaterial()` ; `build()` est celui du Builder générique ; la complétude appartient exclusivement à `Model::isCompatible(palette)` ; la classification opaque/translucide repose exclusivement sur le Model ; une paire de Models peut partager une palette malgré des opacités distinctes pour un nom commun. Les conflits d'opacité **à l'intérieur d'un Model** restent refusés au bake par le Mesher, inchangé.

## Tests obligatoires

- [x] Palette construite avant tout Model, puis contrôlée par celui-ci.
- [x] Compatibilité modèle/palette et Materials surnuméraires.
- [x] Un seul atlas `albedo` pour toute la palette, y compris entre les slots opaques et translucides.
- [x] Rejet immédiat de deux atlas à `setMaterial()`, y compris pour un slot surnuméraire ; récupération du Builder après échec.
- [x] Slot occulté manquant détecté par le Model.
- [x] Changer de palette ne change aucun vertex.
- [x] Échec de `setMaterial()` sans mutation partielle.

**Non-régression commune :** conserver la compilation et les tests existants. Lancer `cmake --workflow --preset testDebug` et `cmake --workflow --preset testRelease` pour les configurations prises en charge, en plus des builds Core sans Graphics lorsque pertinent. Tests GPU et golden-image sur l'infrastructure graphique existante (Windows/WGL) ; ne pas remplacer automatiquement une golden image de référence.

## Définition de terminé / critère de fusion

Un Model peut recevoir une nouvelle palette sans régénérer ses meshes.

- [ ] Tous les invariants et erreurs publiques ci-dessus sont testés et documentés ; aucun état partiel n'est publié après un échec lorsqu'une garantie forte est prescrite.
- [ ] Les dépendances listées sont disponibles sur la branche de base ; aucune fonctionnalité des fiches suivantes n'est embarquée sans nécessité stricte.
- [ ] La PR présente le périmètre, les tests réellement exécutés et les résultats, les screenshots/diffs si graphiques, et les limitations éventuelles.
- [ ] Aucune fusion sans validation du propriétaire.

## Hors périmètre

Assembly, dessin GL, regroupement automatique de meshes multi-atlas.

## Consigne de passation

Confier **uniquement PR-07** à l'IA. Les détails locaux (signatures C++, choix numérique aux bornes, états GPU) peuvent être finalisés pendant l'implémentation avec tests. Ne pas réouvrir les choix validés ni recréer le registre de questions supprimé. En cas de contradiction fonctionnelle publique **démontrée**, soumettre au plus une question ciblée ; poursuivre les éléments indépendants. [Prompt de démarrage générique](AI-HANDOFF.md).
