# Sparkle V0.1.4 — Model, Anchors, Views et harmonisation

[Sommaire de l'architecture](../voxel-engine-v0.1.4-specification.md)

## 6. Composition et rendu

### 6.1 Voxel::Mesh

Le Mesher produit un `spk::Voxel::Mesh` immuable et partageable. Il contient deux `Mesh::SurfaceMesh` (opaque et translucide), un `ShadowMap::Mesh`, et ses slots sémantiques (nom, opacité, offset). Chaque vertex est un `Mesh::Vertex`. La géométrie ne contient aucune palette.

### 6.2 Model immutable, Model::Builder et Anchors

Un `Model` contient un Mesh et des Anchors nommés dans des données partagées `std::shared_ptr<const Data>`. Il ne propose aucun setter. `Model::Builder` possède les opérations `setMesh`, `addAnchor`, `removeAnchor`, `setAnchorTransform`, `bind`, `unbind`, `harmonize`, `build`. Une construction terminée ne modifie jamais un Model publié.

Chaque Anchor possède un repère rigide et éventuellement un Model enfant immuable partagé. Le nom dans le parent est obligatoire et unique. L'argument `attachment` de `Builder::bind(name, child, attachment)` désigne un Anchor du Model enfant ; sa valeur par défaut `Anchor::NoAttachmentPoint = ""` signifie que l'origine du Model enfant sert de pivot.

### 6.3 Model::View

Une View représente une instance indépendante, avec ses propres palettes, rotations de joints et transformation globale. Les palettes sont validées transactionnellement et peuvent être propres à chaque Model tant que l'harmonisation n'est pas demandée.

La transformation d'un enfant est `M_child = M_parent * A_parent * R_joint * inverse(A_child)` si un point d'attache enfant est spécifié ; sinon, la dernière opération d'inversion est absente.

Les chemins d'Anchors utilisent `Model::Anchor::Path = std::vector<std::string>`, avec des segments explicites, sans décodage de `/`. La recherche courte par nom est acceptée seulement lorsqu'elle est non ambiguë. `View::AnchorHandle` est attaché à sa View et devient invalide lors de sa destruction, de son déplacement ou de son remplacement.

### 6.4 Model::Builder::harmonize()

L'appel explicite à `harmonize()` précède `build()`. Le Builder parcourt la hiérarchie et collecte les slots par nom et opacité. Un nom existant avec la même opacité réutilise l'indice global ; une opacité contradictoire provoque une exception. Les slots globaux sont triés de façon déterministe et chaque Mesh, y compris celui des enfants, est réindexé sans altérer les positions, UV, normales, triangles ou mesh de profondeur.

L'harmonisation n'inspecte aucune palette ni Material concret : elle est strictement structurelle. Les Models enfants sont reconstruits en valeurs immuables ; les Models sources restent inchangés. Les Views de ce Model harmonisé peuvent utiliser des palettes différentes, mais chacune doit fournir exactement les clés du layout commun dans l'ordre attendu. Le `Model::Harmonizer` indépendant a été supprimé.

### 6.5 Frontière du rendu

La PR-08 définit les Models CPU, les poses et la visite des occurrences. Les GPU buffers, la capture et les passes de rendu seront implémentés dans les PR dédiées.
