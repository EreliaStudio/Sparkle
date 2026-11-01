# Sparkle V0.1.4 — Model, Anchors, Views et harmonisation

[Sommaire de l'architecture](../voxel-engine-v0.1.4-specification.md)

## 6. Composition et rendu

### 6.1 Voxel::Mesh

Le Mesher fournit un Voxel::Mesh contenant les deux SurfaceMesh opaque et translucide, le ShadowMap::Mesh de profondeur, les noms et opacités des slots et un offset de materialIndex. La géométrie est immuable, partageable et indépendante des palettes. L'ancien mesh élémentaire Voxel::Mesh est renommé Voxel::SurfaceMesh.

### 6.2 Voxel::Model et Model::Anchor

Un Model possède UN Voxel::Mesh et une collection d'Anchors nommés ; aucune palette. Les slots sémantiques viennent du Mesher, pas d'un setMaterialSlot manuel. Chaque Anchor possède une matrice de repos rigide, un Model enfant facultatif et éventuellement le nom d'un Anchor du Model enfant pour aligner les pivots. addAnchor, removeAnchor, anchor, Anchor::bind(model, attachment) et Anchor::unbind construisent une hiérarchie. Les enfants sont copiés structurellement et conservent le partage des Meshes immuables.

### 6.3 Model::View

Chaque View représente une instance indépendante d'une hiérarchie. Elle gère les palettes par occurrence, et les rotations des Anchors (pas leurs translations animables). setPalette(palette, Self|Recursive) applique une palette au Model racine seul ou récursivement ; setPalette(anchorName, palette, ...) cible un enfant. Les palettes sont validées de façon transactionnelle. Les Views peuvent avoir autant de palettes indépendantes que de Models.

Pour un parent et un enfant, la transformation finale est :

    M_child = M_parent * A_parent * R_joint * inverse(A_child)

A_parent est le repère de repos du joint parent, R_joint sa rotation par View et A_child le point d'attache du Model enfant. La position du joint reste cohérente dans le repère parent. setTransform déplace le personnage entier ; visit fournit les Models, palettes et matrices aux futures captures de rendu.

L'identification publique par nom est provisoire : un nom absent ou ambigu produit une exception. Le schéma d'identifiants/chemins d'occurrence sera discuté après implémentation des API, sans être figé ici.

### 6.4 Model::Harmonizer

Harmonizer::process(view) parcourt chaque Model et sa palette, crée des indices de matériaux communs, puis fournit un Model copié avec Meshes réindexés, une palette commune portant les Materials d'origine, et un MaterialLayout ordonné. Les noms de slots réutilisés avec des Materials différents sont distingués sans perte. L'atlas commun est vérifié : les palettes sources peuvent être indépendantes, mais la sortie harmonisée ne peut référencer plusieurs atlas albedo.

Les UV, positions, normales, indices de triangles, mesh de profondeur et hiérarchie d'Anchors sont préservés. Les nouvelles palettes harmonisées doivent conserver exactement le MaterialLayout, sans insertion de noms supplémentaires qui décalerait les indices. Plusieurs Views peuvent partager ce Model harmonisé, avec des palettes et des animations différentes sans nouveau bake.

### 6.5 Frontière du rendu

La PR-08 définit le modèle CPU, la visite de la géométrie et les transformations d'articulation. Le VoxelRenderingSystem, les GPU buffers et les passes RenderCommand restent prévus dans les PR de rendu ultérieures.
