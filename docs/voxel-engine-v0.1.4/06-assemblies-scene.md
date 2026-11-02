# Sparkle V0.1.4 — Model, Anchors, Views et harmonisation

[Sommaire de l'architecture](../voxel-engine-v0.1.4-specification.md)

## 6. Composition et rendu

### 6.1 Voxel::Geometry

Le Mesher produit un `spk::Voxel::Geometry` immuable et partageable. Il contient deux `Geometry::SurfaceMesh` (opaque et translucide), un `ShadowMap::Mesh`, et ses slots sémantiques (nom et opacité). Les indices de vertices sont toujours locaux à la géométrie et commencent à zéro. Chaque vertex est un `Geometry::Vertex`. La géométrie ne contient aucune palette.

### 6.2 Model immutable, Model::Builder et Anchors

Un `Model` contient une `Geometry` et des Anchors nommés dans des données partagées `std::shared_ptr<const Data>`. Il ne propose aucun setter. `Model::Builder` possède les opérations `setGeometry`, `addAnchor`, `removeAnchor`, `setAnchorTransform`, `attach`, `detach`, `harmonize`, `build`. Une construction terminée ne modifie jamais un Model publié.

Chaque Anchor possède un repère rigide et un `std::optional<Model>` pour son enfant : `std::nullopt` signifie qu'aucun Model n'est attaché. `Model{}` est un Model valide à géométrie vide, pas une valeur sentinelle. Quand un enfant est présent, sa copie ne duplique que son `shared_ptr<const Data>` et maintient ses données immuables en vie, sans transfert de propriété ni dépendance à l'Asset Manager. `Builder::attach(name, child, attachment)` copie légèrement le Model fourni, et `detach(name)` rétablit `std::nullopt`. `Anchor::model()` renvoie `nullptr` lorsqu'il n'y a pas d'enfant. Le nom dans le parent est obligatoire et unique. L'argument `attachment` de `Builder::attach` désigne un Anchor du Model enfant ; `Anchor::NoAttachmentPoint = ""` signifie que l'origine du Model enfant sert de pivot.

### 6.3 Model::View et Model::Anchor::View

Une `Model::View` représente une occurrence indépendante d'un Model. Elle conserve une copie légère du Model immutable, sa palette optionnelle, sa transformation locale et une `std::map<std::string, Model::Anchor::View>`. Ses Anchor Views sont créées **automatiquement et récursivement au constructeur**, d'après la hiérarchie immutable du Model ; un Anchor sans Model enfant garde sa propre View sans enfant.

Chaque `Model::Anchor::View` porte son quaternion de rotation (identité par défaut) et, si son Anchor possède un Model, un `std::unique_ptr<Model::View>` enfant. `hasChild()` indique sa présence, `child()` retourne la View enfant ou lève une exception si elle manque. Le `unique_ptr` est propriétaire uniquement de la View mutable ; le Model conserve son `shared_ptr<const Data>` et reste immutable. Les copies de Views reconstruisent leurs enfants pour assurer l'indépendance des animations et des palettes.

`view.bind(palette)` valide les slots et l'atlas puis modifie **uniquement cette View** (`PalettePropagation::Self` par défaut). La propagation à toutes les Views descendantes nécessite `PalettePropagation::Recursive` ; l'opération valide tous les enfants avant de changer les palettes. Les matériaux surnuméraires restent permis. Une View sans palette est non rendable uniquement si son Model possède des slots requis.

Les rotations se modifient sur l'Anchor View (`view.anchor("shoulder").setRotation(rotation)`) ; la palette du Model enfant se lie directement à sa View (`view.anchor("shoulder").child().bind(palette)`). Un Anchor ne porte pas de palette : cela évite de traiter différemment le Model racine. Les chemins `Model::Anchor::Path = std::vector<std::string>` permettent de retrouver les Anchors imbriqués, mais ne stockent plus l'état. Les noms courts restent utilisables uniquement s'ils sont non ambigus. `AnchorHandle`, `_palettes` et `_rotations` ont disparu.

Lors du parcours `visit()`, la transformation mondiale d'une View est `M_parent * M_view` (celle de la racine est mondiale ; celle d'un enfant est locale). Pour un enfant attaché : `M_child_parent = M_parent_world * A_parent * R_joint * inverse(A_child)`, avec inversion uniquement si l'Anchor enfant désigné existe. La View enfant applique ensuite son propre `M_view`. La View expose ses données CPU et ses transformations finales au moteur de rendu ; elle n'exécute aucun appel GPU elle-même.

### 6.4 Model::Builder::harmonize()

L'appel explicite à `harmonize()` précède `build()`. Le Builder parcourt la hiérarchie et collecte les slots par nom et opacité. Un nom existant avec la même opacité réutilise l'indice global ; une opacité contradictoire provoque une exception. Les slots globaux sont triés de façon déterministe et chaque `Geometry`, y compris celles des enfants, est réindexée sans altérer les positions, UV, normales, triangles ou mesh de profondeur.

L'harmonisation n'inspecte aucune palette ni Material concret : elle est strictement structurelle. Les Models enfants reconstruits sont conservés en `std::optional<Model>` par valeur dans les Anchors. Les anciennes géométries sources restent inchangées et les données des enfants harmonisés vivent aussi longtemps que leurs Models parents. Les Views de ce Model peuvent utiliser des palettes différentes : chaque palette doit contenir tous les noms de slots requis. Aucun champ `harmonized` n'est conservé et un nouvel appel à `harmonize()` recalcule et reconstruit systématiquement les géométries, même déjà harmonisées. Le `Model::Harmonizer` indépendant a été supprimé.

`View::isRenderable()` retourne `false` uniquement lorsque la hiérarchie manque de palettes compatibles, sans intercepter les erreurs inattendues (par exemple une allocation impossible).

### 6.5 Frontière du rendu

La PR-08 définit les Models CPU, les poses et la visite des occurrences. Les GPU buffers, la capture et les passes de rendu seront implémentés dans les PR dédiées.

### 6.6 Conventions de nommage des liaisons et collections

- `bind` associe une dépendance requise au fonctionnement concerné : `Volume::Builder::bind(Definition::Collection)` ou `Model::View::bind(spk::Palette)` ; l'absence temporaire de palette laisse la View non rendable si elle a des slots.
- `attach` et `detach` rattachent ou retirent un Model enfant facultatif d'un Anchor. Un Anchor sans enfant reste valide. `Model::Builder::attach(name, child, attachmentPoint)` utilise `Anchor::NoAttachmentPoint` pour l'origine de l'enfant.
- `Mesher::attachCache(Occlusion::Cache&)` / `detachCache()` règlent une optimisation facultative ; le bake fonctionne sans cache.
- `Collection` nomme les ensembles associatifs d'objets identifiés (`Shape::Collection`, `Definition::Collection`, `Definition::SlotCollection`) ; `Container` est réservé aux séquences lorsqu'un nom de type explicite est nécessaire. Les accesseurs simples `anchors()`, `slots()`, `triangles()` restent inchangés.
- `set` remplace une propriété, `add` crée une nouvelle entrée et refuse les doublons, `remove` supprime une entrée, `build` publie les données immuables et `bake` génère le maillage optimisé.
- `Shape::Builder::addFromJSON` ajoute des polygones et des slots ; `Definition::Builder::loadFromJSON` remplace sa sélection et ses slots ; `Volume::Builder::copyFrom` duplique un volume compatible.
- `isCompatible` vérifie la compatibilité d'une palette avec `Geometry` ou `Model` ; `at` lève en cas d'absence, `tryGet` retourne `nullptr` et `contains` vérifie l'existence.
