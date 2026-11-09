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

### 6.4 Applications voxel futures

`Voxel::Volume` ne dérive pas lui-même de `Blueprint::Element`. Une classe d'élément dérivée pourra embarquer ou référencer un Volume ; ce type spécialisé de production n'existe pas encore.
Les golden tests PR-08 utilisent `ColorVolume` et `ColorView` déclarés localement pour dessiner un personnage articulé et son épée. Ils valident Model3D et OpenGL, pas un renderer voxel de production.
**PR-13** doit réaliser le rendu de `Voxel::Geometry` avec `spk::Palette`, atlas et commandes GPU. **PR-14** doit relier ce rendu aux occurrences articulées `Model3D` sans rebake lors de rotations.
Les composants ECS, couches de filtrage, ombres et passes d'EngineView sont des objectifs des PR suivantes. Aucun `ModelRenderingComponent` ni `AssemblyRenderingComponent` des anciens plans n'est déjà implémenté.

[État consolidé des PR-01 à PR-08](implementation/IMPLEMENTED-01-08.md)
