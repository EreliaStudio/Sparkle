# PR-25 — Dessin voxel translucide sur WBOIT/HDR existants

> **Fiche de préparation pour une future PR GitHub, pas une PR déjà créée.** Ne développer que cette fiche.
>
> **Branche source :** `impl/v0.1.4/25-wboit-hdr`, à créer depuis la **tête courante de `V0.1.4`** après fusion des prérequis. **Branche cible de la PR GitHub : `V0.1.4`**. Ne jamais cibler `main` ni partir d'une autre branche de feature.

**Objectif :** Rendre les faces voxel translucides dans la passe WBOIT et la chaîne HDR **déjà présentes depuis PR12**, en réutilisant `Voxel::Renderer` de PR13.

**Prérequis :** [PR-11](PR-11-gpu-attachments-surfaces.md), [PR-12](PR-12-enginewidget-first-view.md), [PR-13](PR-13-voxel-render-first.md), [PR-19](PR-19-forward-lighting.md)

## Adaptation à la baseline PR13 (obligatoire)

**PR12 a déjà livré HDR, attachments WBOIT, passes et composition/tone mapping EngineView ; PR13 a déjà livré `DrawVoxelCommand`, palettes, remap et instances opaques.** Cette fiche ajoute essentiellement le dessin de `Voxel::Geometry::transparentMesh()` dans la passe WBOIT existante avec les mêmes occurrences et lumières PR19. Pas de second WBOIT, de seconde composition HDR ou de renderer alternatif.

## Spécifications normatives

- [09-pipeline-gpu-transparence.md](../09-pipeline-gpu-transparence.md)
- [08-lumieres-et-ombres.md](../08-lumieres-et-ombres.md)
- [04-materiaux-textures-palettes.md](../04-materiaux-textures-palettes.md)
- [10-contrats-et-validation.md](../10-contrats-et-validation.md)
- [Spécification principale](../../voxel-engine-v0.1.4-specification.md)
- [Contrats et scénarios d'acceptation](../10-contrats-et-validation.md)

## Périmètre exact à réaliser

- [ ] Capturer le `transparentMesh()` déjà produit dans `Voxel::Geometry`, les matrices d'instances, palettes et remap du pipeline PR13 ; ajouter la commande/shader translucide sur **les MRT, attachments, depth, blending WBOIT et composition HDR/Reinhard/sRGB existants** sans les réimplémenter. Préserver le poids w=1+7*(1-z) et les équations de composition.

## Instructions techniques et intégration

Depth LEQUAL en opaque/WBOIT, depth mask disabled WBOIT; clears (0,0,0,0) et reveal=1; alpha=textureAlbedo.a*slotOpacity; un draw MRT par géométrie; même Light::Selection et shadow map admise que opaque; pas de double gamma widgets.

Avant de coder, inspecter la forme **actuelle** des classes, constructions, tests et CMake ; réutiliser `Mesh<T>`, `Registry`, `ContractProvider`, `ThreadSafeSlot`, `GPUResourceCollection`, `RenderSnapshot::Builder`, `ShaderStorageBuffer`, `spk::Exception` selon pertinence. Les exemples d'API dans les spécifications sont parfois indicatifs : la signature finale doit respecter les invariants sans créer une abstraction parallèle.

## Tests obligatoires

- [ ] Golden voxels texturés 0,1,2 couches et ordre, avec non-régression des golden images opaques PR13
- [ ] z=0,0.5,1 => w=8,4.5,1
- [ ] alpha 0 skip, alpha 1 T=0
- [ ] opaque cache translucent arrière
- [ ] R8 quantification
- [ ] overflow ne produit pas de NaN accepté
- [ ] alpha finale 1 et UI non tonemappée.

**Non-régression commune :** conserver la compilation et les tests existants. Lancer `cmake --workflow --preset testDebug` et `cmake --workflow --preset testRelease` pour les configurations prises en charge, en plus des builds Core sans Graphics lorsque pertinent. Tests GPU et golden-image sur l'infrastructure graphique existante (Windows/WGL) ; ne pas remplacer automatiquement une golden image de référence.

## Définition de terminé / critère de fusion

Transparence voxel colorée dans `EngineView` en conservant la composition générique HDR existante ; les tests PR13 opaques restent inchangés.

- [ ] Tous les invariants et erreurs publiques ci-dessus sont testés et documentés ; aucun état partiel n'est publié après un échec lorsqu'une garantie forte est prescrite.
- [ ] Les dépendances listées sont disponibles sur la branche de base ; aucune fonctionnalité des fiches suivantes n'est embarquée sans nécessité stricte.
- [ ] La PR présente le périmètre, les tests réellement exécutés et les résultats, les screenshots/diffs si graphiques, et les limitations éventuelles.
- [ ] Aucune fusion sans validation du propriétaire.

## Hors périmètre

tri alpha, reveal 2e draw, autres formats, tone mapping additionnel.

## Consigne de passation

Confier **uniquement PR-25** à l'IA. Les détails locaux (signatures C++, choix numérique aux bornes, états GPU) peuvent être finalisés pendant l'implémentation avec tests. Ne pas réouvrir les choix validés ni recréer le registre de questions supprimé. En cas de contradiction fonctionnelle publique **démontrée**, soumettre au plus une question ciblée ; poursuivre les éléments indépendants. [Prompt de démarrage générique](AI-HANDOFF.md).
