# PR-25 — WBOIT MRT et composition HDR complète

> **Fiche de préparation pour une future PR GitHub, pas une PR déjà créée.** Ne développer que cette fiche.

**Objectif :** Afficher des surfaces translucides avec une seule passe de géométrie et HDR jusqu'à la sortie.

**Prérequis :** [PR-11](PR-11-gpu-attachments-surfaces.md), [PR-12](PR-12-enginewidget-first-view.md), [PR-13](PR-13-voxel-render-first.md), [PR-19](PR-19-forward-lighting.md)

## Spécifications normatives

- [09-pipeline-gpu-transparence.md](../09-pipeline-gpu-transparence.md)
- [08-lumieres-et-ombres.md](../08-lumieres-et-ombres.md)
- [04-materiaux-textures-palettes.md](../04-materiaux-textures-palettes.md)
- [10-contrats-et-validation.md](../10-contrats-et-validation.md)
- [Spécification principale](../../voxel-engine-v0.1.4-specification.md)
- [Contrats et scénarios d'acceptation](../10-contrats-et-validation.md)

## Périmètre exact à réaliser

- [ ] Mesh translucent, MRT accumulation RGBA16F et reveal R8, depth partagé opaque read-only pour translucent, poids fixe w=1+7*(1-z), blending par attachement, composition opaque*T+accum.rgb/accum.a*(1-T), Reinhard, sRGB final.

## Instructions techniques et intégration

Depth LEQUAL en opaque/WBOIT, depth mask disabled WBOIT; clears (0,0,0,0) et reveal=1; alpha=textureAlbedo.a*slotOpacity; un draw MRT par géométrie; même Light::Selection et shadow map admise que opaque; pas de double gamma widgets.

Avant de coder, inspecter la forme **actuelle** des classes, constructions, tests et CMake ; réutiliser `Mesh<T>`, `Registry`, `ContractProvider`, `ThreadSafeSlot`, `GPUResourceCollection`, `RenderSnapshot::Builder`, `ShaderStorageBuffer`, `spk::Exception` selon pertinence. Les exemples d'API dans les spécifications sont parfois indicatifs : la signature finale doit respecter les invariants sans créer une abstraction parallèle.

## Tests obligatoires

- [ ] Golden 0,1,2 couches et ordre
- [ ] z=0,0.5,1 => w=8,4.5,1
- [ ] alpha 0 skip, alpha 1 T=0
- [ ] opaque cache translucent arrière
- [ ] R8 quantification
- [ ] overflow ne produit pas de NaN accepté
- [ ] alpha finale 1 et UI non tonemappée.

**Non-régression commune :** conserver la compilation et les tests existants. Lancer `cmake --workflow --preset testDebug` et `cmake --workflow --preset testRelease` pour les configurations prises en charge, en plus des builds Core sans Graphics lorsque pertinent. Tests GPU et golden-image sur l'infrastructure graphique existante (Windows/WGL) ; ne pas remplacer automatiquement une golden image de référence.

## Définition de terminé / critère de fusion

Transparence colorée stable sur sortie EngineView et consommation via Material.

- [ ] Tous les invariants et erreurs publiques ci-dessus sont testés et documentés ; aucun état partiel n'est publié après un échec lorsqu'une garantie forte est prescrite.
- [ ] Les dépendances listées sont disponibles sur la branche de base ; aucune fonctionnalité des fiches suivantes n'est embarquée sans nécessité stricte.
- [ ] La PR présente le périmètre, les tests réellement exécutés et les résultats, les screenshots/diffs si graphiques, et les limitations éventuelles.
- [ ] Aucune fusion sans validation du propriétaire.

## Hors périmètre

tri alpha, reveal 2e draw, autres formats, tone mapping additionnel.

## Consigne de passation

Confier **uniquement PR-25** à l'IA. Les détails locaux (signatures C++, choix numérique aux bornes, états GPU) peuvent être finalisés pendant l'implémentation avec tests. Ne pas réouvrir les choix validés ni recréer le registre de questions supprimé. En cas de contradiction fonctionnelle publique **démontrée**, soumettre au plus une question ciblée ; poursuivre les éléments indépendants. [Prompt de démarrage générique](AI-HANDOFF.md).
