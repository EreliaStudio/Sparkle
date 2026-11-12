# Sparkle v0.1.4 — Model3D, hiérarchies articulées et scène

[Sommaire de l'architecture](../voxel-engine-v0.1.4-specification.md)

## 6. Model3D : contrat implémenté en PR-08

> L'ancien système `spk::Voxel::Assembly` (Parts mutables, PartID, insertion par expansion, reparentage, VersionedTrait) a été abandonné. Les règles suivantes décrivent les classes effectivement livrées. Elles remplacent les anciennes prescriptions d'Assembly.

### 6.1 `spk::Model3D::Blueprint` (Core)

`Blueprint` est une définition immuable, partagée par copie légère, publiée via `Blueprint::Builder`.
Un `Blueprint::Element` est un type polymorphe nommé, possédant un anchor rigide et des `AttachmentPoint` nommés. Chaque point définit un repère rigide et peut avoir au plus un enfant structurel.
Les éléments peuvent être assemblés par déplacement ou créés directement via les méthodes template du Builder. Un nom d'élément est unique dans le Blueprint. Les références de construction ne doivent plus être utilisées pour modifier les éléments publiés.

### 6.2 `spk::Model3D::ObjectHeadless` (Core)

`ObjectHeadless` instancie un Blueprint. Chaque `ElementNode` référence sa définition et ses articulations. `AttachmentNode::setRotation(Quaternion|Vector3)` ou `rotate(...)` ajuste la rotation locale ; les Euler sont exprimés en degrés.
`Attachable::globalTransform()` calcule les matrices globales avec cache et invalidation ciblée. `AttachmentNode::attach(Blueprint|ObjectHeadless)` crée ou déplace un sous-objet autonome ; plusieurs occurrences peuvent être liées au même point. `detach(ObjectHeadless&)` rend l'occurrence choisie par valeur.
Les copies d'ObjectHeadless dupliquent les états mutables et les sous-objets tout en partageant le Blueprint immuable. Aucun mesh ni OpenGL n'est requis en Core.

### 6.3 `spk::Model3D::Object<TGraphics>` (Graphics)

`Model3D::Object<TGraphics>` dérive d'ObjectHeadless. `TGraphics` respecte `ObjectGraphics` : `ElementType` dérivé de `Blueprint::Element`, construction depuis `const ElementType&`, et `buildRenderSnapshot(RenderSnapshot::Builder&, const Matrix4x4&)`.
`synchronize()` maintient un cache de vues par ElementNode. `buildRenderSnapshot(builder, transform)` synchronise et demande à chaque vue retenue de contribuer avec sa matrice cumulée. Une rotation ne recrée pas les vues.
Un Object<TGraphics> utilise une seule spécialisation graphique. La ViewFactory hétérogène et l'auto-enregistrement sont des travaux futurs, non livrés par PR-08.

### 6.4 Frontière entre Model3D générique et voxel PR13

`Voxel::Volume` ne dérive pas de `Model3D::Blueprint::Element`. Les golden tests `ColorVolume`/`ColorView` de PR08 valident la hiérarchie `Model3D` générique et OpenGL, **pas** le pipeline voxel de production.

PR13 a livré `Voxel::Mesher` → `Voxel::Geometry`, `spk::Palette`, `Voxel::Renderer` et `Voxel::RenderingSystem`. Les pièces voxel peuvent être composées à travers des `Entity3D` parentées et leurs `Transform3D`. Dans ses golden tests, les poses de personnages sont préparées comme volumes/géométries distincts ; l'épée est attachée par parenting ECS.

**Décision après PR13 : PR14 retirée.** Aucun lien obligatoire `Model3D::Blueprint` → voxel renderer, aucune vue `ModelElementView` ou `Voxel::ModelRenderer` supplémentaire. Les API de PR08 restent disponibles pour d'autres usages graphiques. Les PR suivantes éclairent le même renderer voxel et lient leurs meshes d'ombres explicitement via `ShadowMap::Contributor`. Voir [PR14 retirée](implementation/PR-14-assembly-render.md).

[État consolidé des PR-01 à PR-08](implementation/IMPLEMENTED-01-08.md)
