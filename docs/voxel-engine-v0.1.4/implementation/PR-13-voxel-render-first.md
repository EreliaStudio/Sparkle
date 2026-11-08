# PR-13 — Premier rendu voxel opaque texturé (à réaliser)

> **Branche prévue :** `impl/v0.1.4/13-voxel-render-first` depuis la tête courante de `V0.1.4`, PR vers `V0.1.4`. Ne pas développer cette fiche avant les prérequis.

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

Articulations Model3D (PR-14), WBOIT, lumières et ombres. Aucun renderer voxel de production n'a été livré par les golden tests `ColorVolume`/`ColorView` de PR-08.

**Définition de terminé :** première `Voxel::Geometry` opaque réellement rendue et golden image approuvée, sans API obsolète.
