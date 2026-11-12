# PR-13 — Premier rendu voxel opaque texturé (livrée, PR GitHub #63 fusionnée)

> **État :** [PR #63](https://github.com/EreliaStudio/Sparkle/pull/63) fusionnée dans `V0.1.4`. La liste ci-dessous est le périmètre historique, non un nouveau chantier.

**Objectif :** dessiner dans une EngineView une `spk::Voxel::Geometry` réellement issue d'un `spk::Voxel::Volume`, avec une `spk::Palette` compatible. Ne pas reprendre l'ancienne classe `Voxel::Model`.

**Prérequis :** PR-04, PR-07 (déjà livrées) ; [PR-10](PR-10-engineview-snapshot.md), [PR-11](PR-11-gpu-attachments-surfaces.md), [PR-12](PR-12-enginewidget-first-view.md).

## Spécifications

- [Meshing et Geometry](../03-meshing-occlusion.md)
- [Materials et Palette](../04-materiaux-textures-palettes.md)
- [EngineView et passes](../07-vues-et-passes.md)
- [GPU et buckets](../09-pipeline-gpu-transparence.md)
- [État PR-01 à PR-08](IMPLEMENTED-01-08.md)

## Périmètre attendu

- [ ] Définir la représentation graphique d'une occurrence de `Voxel::Geometry` et sa `spk::Palette`, **sans ajouter de `bind(palette)` à Geometry**.
- [ ] Choisir une interface d'intégration avec le système de rendu actuel (composant ECS ou autre Contributor) après inspection de `RenderingEngine` et des Registry, sans imposer le nom historique `ModelRenderingComponent`.
- [ ] Construire les commandes opaques, atlas/UV, mappings des slots sémantiques vers les Materials et shaders compatibles.
- [ ] Préparer buckets, SSBO d'instances et de Materials par snapshot, instancing de `Matrix4x4` (64 octets) et bindings réactivés avant chaque draw.
- [ ] Préparer la capture CPU immuable : `execute()` ne relit ni volume, ni objet de pose, ni palette mutable.
- [ ] Valider la compatibilité des slots et de l'atlas avant la publication, sans re-bake en cas de changement de palette.

## Tests obligatoires

- [ ] Golden image d'un vrai cube **voxel texturé**, plus une orientation non identité.
- [ ] Deux apparences ou atlas incompatibles ne partagent pas un même bucket.
- [ ] ABI std430 de la matrice d'instance, conservation des anciens snapshots, rebind des SSBO après draw tiers.
- [ ] Rejet du scale non uniforme/nul/négatif si le shader emploie la matrice de modèle seule pour les normales.
- [ ] Compilation Debug/Release Core/Graphics, tests GPU et formatage.

## Hors périmètre

Couplage direct à `Model3D` (PR14 retirée), dessins voxel translucides (PR25), éclairage direct et ombres. L'infrastructure EngineView HDR/WBOIT générique est déjà livrée par PR12. Aucun renderer voxel de production n'a été livré par les golden tests `ColorVolume`/`ColorView` de PR-08.

**Définition de terminé :** première `Voxel::Geometry` opaque réellement rendue et golden image approuvée, sans API obsolète.

## Contrat de production à réutiliser pour PR15–PR26

- `Voxel::Renderer` associe une `Geometry` immuable et une `spk::Palette` à une `Entity3D`. `Voxel::RenderingSystem` collecte, vérifie layers et scale et émet des `DrawVoxelCommand` opaques instanciés par EngineView.
- `Renderer::resolveMaterialRemap` calcule les indices de Materials au binding, le remap étant consommé par un SSBO dans le dessin existant ; changer la palette n'impose pas de rebake.
- Les [golden tests de PR13](../../../../tests/TU/Graphics/srcs/voxel/voxel_scenes_golden_test.cpp) couvrent personnages, poses et arme par parenting ECS, maisons, matériaux et caméras. **Les poses du personnage sont rebakées comme géométries distinctes dans ces tests** : elles ne prouvent pas la consommation directe d'un `Model3D::Blueprint`.
- `Model3D::Blueprint` (PR08) n'est pas une entrée de `Voxel::Renderer` ; aucune intégration supplémentaire n'est exigée en v0.1.4. [PR14 retirée](PR-14-assembly-render.md).
- Les PR19 et PR25 étendent les shaders et le pipeline de dessin de PR13 plutôt que de les dupliquer ; les ombres PR20–24 sont fournies explicitement.
