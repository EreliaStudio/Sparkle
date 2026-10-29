# Sparkle V0.1.4 — Matériaux, textures et palettes

[Sommaire de l'architecture](../voxel-engine-v0.1.4-specification.md)

## 4. Matériaux, textures et palettes

```mermaid
classDiagram
  direction LR
  class Material {
    +Builder
    +textures nommées
    +régions UV normalisées
  }
  class Texture {
    +Handle
  }
  class SpriteSheet {
    +sprite(coord) TextureSection
    +grille de sprites
  }
  class VoxelPalette {
    +Builder
    +association slot -> Material
    -Data immutable partagé
  }
  class Model {
    +slots sémantiques triés
    +mesh opaque
    +mesh translucide
  }
  Texture <|-- SpriteSheet
  Material --> Texture : Handle statique
  Material ..> EngineView : source empruntée (API à finaliser)
  VoxelPalette --> Material : valeurs immuables
  Model ..> VoxelPalette : association validée
```

### 4.1 `spk::Material` : ressource générique

`spk::Material` est un **type générique Sparkle**, sans spécialisation `VoxelMaterial`. Il est construit via `Material::Builder`, puis devient **immuable**. Il peut associer zéro, une ou plusieurs textures à des **noms de rôles libres** (`albedo`, ou tout autre nom choisi par le programme). Seul le système voxel donne une signification au rôle `albedo`. Le Builder générique ne valide pas les conventions propres aux voxels.

Chaque liaison à une texture conserve un **`spk::Texture::Handle`** obtenu depuis la `const spk::Texture&` fournie lors du binding, ainsi qu'une région UV normalisée (`Texture::Section`, exprimable en `anchor` et `size`). Le Material conserve le **handle de l'état exact choisi** et non un pointeur brut vers l'objet Texture, une copie complète de Texture ou un pointeur vers l'AssetManager.

Un handle conserve l'état GPU partagé de la Texture après destruction du wrapper Texture initial. Il ne conserve toutefois **ni la grille du SpriteSheet, ni à lui seul son ensemble de résolutions**. Le Material final n'a pas besoin de la grille : la conversion d'un sprite en `Texture::Section` est réalisée **avant** sa construction.

La résolution effectivement capturée est celle de la Texture explicitement passée au Builder. Il n'existe **aucune sélection automatique de résolution** dans Material ou dans le rendu voxel. Changer de résolution implique de fournir la nouvelle Texture choisie à un **nouveau Material**, puis de choisir la nouvelle palette correspondante. Les mipmaps standards du State GPU restent possibles.

**Extension pour les textures de vues :** la source statique conserve le contrat `Texture::Handle` ci-dessus. Une liaison à une `EngineView` est une **source non propriétaire**, dont l'identité reste fixe dans le Material immuable. **La construction CPU du nouveau snapshot** sélectionne et capture la **version logique courante de la texture** (ou la texture de remplacement explicite), avec vérification des erreurs logiques déjà connues ; **l'exécution GPU** matérialise ce handle pour chaque `Window::Surface` et contrôle la disponibilité de son image au moment de l'utilisation. Le Material ne prolonge pas la vie de la vue et ne conserve pas durablement les handles de sa sortie ; les commandes déjà publiées peuvent conserver les handles logiques capturés. L'API exacte de liaison et la compatibilité avec les règles d'atlas voxel restent à finaliser. Les JSON d'assets ne recherchent pas implicitement des vues.

Les **propriétés et l'identité des handles** d'un Material sont immuables. Les **pixels de l'état Texture partagé** peuvent évoluer selon les API graphiques existantes, avec validation et synchronisation CPU/GPU adaptées : les Materials qui partagent ce même State verront son contenu actualisé. Si une opération détache/remplace le State d'une Texture source, un handle déjà capturé continue de référencer **l'ancien State** : il ne suit pas automatiquement cette substitution. Les paramètres du Material et les anciennes palettes restent inchangés.

### 4.2 Texture simple, SpriteSheet et région d'atlas

La configuration JSON d'une texture peut représenter une simple image :

```json
{
  "Texture": { "path": "terrain.png" },
  "UV": {
    "anchor": { "x": 0.25, "y": 0.5 },
    "size": { "x": 0.125, "y": 0.125 }
  }
}
```

Elle peut également demander un `spk::SpriteSheet`, en donnant une **grille de nombres de sprites**, et sélectionner une cellule de la grille :

```json
{
  "Texture": {
    "path": "terrain.png",
    "size": { "x": 16, "y": 16 }
  },
  "UV": {
    "sprite": { "x": 3, "y": 5 }
  }
}
```

Ici, `Texture.size` signifie **16 sprites en largeur et 16 en hauteur**, **pas une résolution en pixels**. `SpriteSheet::sprite(Vector2UInt)` retourne une `Texture::Section` normalisée. Des UV explicites (`anchor`/`size`) restent utilisables **même avec un SpriteSheet**. La syntaxe `sprite` exige une grille valide et des coordonnées de sprite valides.

Ces exemples illustrent le **bloc de description d'une texture**, pas nécessairement la structure complète d'un Material à rôles multiples ; les noms et l'organisation des rôles restent génériques. Le chargement JSON et la construction manuelle doivent aboutir à la **même représentation Material : handle + section UV**.

La Shape conserve des UV locaux ; le shader applique le rectangle du Material :

```text
uvFinal = material.anchor + shapeUV * material.size
```

Plusieurs Materials peuvent donc référencer le **même atlas** avec des rectangles UV distincts, sans rebake. Un UV local hors `[0,1]` est utilisé tel quel, sans répétition forcée dans la région ni clamp automatique. Le résultat visuel et la maîtrise des bords du sprite sont sous la responsabilité de l'appelant.

### 4.3 `Material::Builder` et l'AssetManager

Le Builder peut fonctionner **sans AssetManager** lorsque le programme lui passe directement des Textures existantes. En revanche, un chargement JSON qui doit résoudre un chemin ou un asset requiert un AssetManager préalablement lié :

```cpp
spk::Material::Builder builder;
builder.bind(&assets);  // Liaison non propriétaire
// Construction depuis un JSON de matériau.
```

`void bind(spk::AssetManager*)` est la forme retenue. **`bind(nullptr)` provoque immédiatement une `spk::Exception`.** Une liaison à cible unique n'est pas implicitement remplacée sans détachement préalable, selon les conventions Sparkle. Le Material obtenu ne conserve aucune référence au gestionnaire.

### 4.4 `spk::VoxelPalette` : association des slots aux matériaux

Une `VoxelPalette` associe des **noms de slots sémantiques et leurs contrats d'opacité** à des valeurs `spk::Material`. C'est une valeur immuable, à copies légères partageant un bloc de données internes ; elle ne contient **aucun SSBO** et n'hérite d'aucun objet GPU.

Son Builder mutable peut être initialisé à partir d'un Model, de plusieurs Models, d'un Assembly (union de leurs besoins) ou d'une palette existante pour en créer une variante. Les collisions de **même nom avec des opacités différentes** entre sources sont rejetées explicitement. Construire une nouvelle palette n'altère pas l'ancienne, ses Materials ou les snapshots existants.

**Chaque slot déclaré** doit recevoir un Material compatible, **même s'il n'a aucun triangle visible**. Le Builder valide que chaque Material affecté possède au moins la texture nommée exactement **`albedo`**, avec exception descriptive sur absence. Le système voxel ne requiert, n'examine ni n'utilise d'autres noms de textures dans cette version. La présence d'autres rôles sur un Material reste autorisée.

Un Model peut être associé à une palette contenant **plus de slots** que lui. L'association est compatible seulement si tous ses noms et opacités requis sont présents. `Model::isCompatible(palette)` est la forme d'API envisagée pour vérifier ce contrat, y compris la limite d'atlas par passe décrite ci-dessous. La palette n'est pas obligée d'être identique au Model, et une même palette peut servir à plusieurs Models.

**Textures issues d'une EngineView.** L'identité d'une source de vue est logique et non celle d'un `Texture::Handle` permanent : après un resize explicite, une nouvelle capture résout la nouvelle version de texture. La compatibilité d'atlas des sources dynamiques reste un contrat à préciser ; les contraintes ci-dessous portent sur les sources statiques.

**Contrainte des atlas :** pour chaque association **Model + VoxelPalette**, les Materials des slots **opaques** requis par ce Model doivent tous référencer **un unique `albedo` `Texture::Handle` au maximum** ; la même règle s'applique, indépendamment, aux slots **translucides**. Cette vérification concerne **tous les slots déclarés par le Model, même sans triangles émis**, pas l'intégralité des slots supplémentaires de la palette. Les passes opaque et translucide peuvent utiliser le même atlas ou deux atlas différents.

Une palette peut donc contenir arbitrairement plusieurs atlas pour répondre aux besoins de divers Models ; elle n'est pas rejetée globalement pour cette raison. C'est une **association** qui est refusée explicitement lorsqu'elle demanderait deux atlas différents dans une même passe. Aucun remaillage silencieux ni découpage des meshes en sous-meshes par texture n'est prévu.

### 4.5 Alpha, opacité et visibilité

La classification géométrique reste celle de la Shape :

- `slot.opacity == 1.0` : triangles dans le mesh opaque ;
- `slot.opacity < 1.0` : triangles dans le mesh translucide (WBOIT).

Dans les **deux** fragment shaders :

```glsl
float alphaFinal = texture(albedo, uvFinal).a * slotOpacity;
if (alphaFinal == 0.0)
    discard;
```

Le mesh opaque est dessiné sans blending de translucence. Une texture contenant de l'alpha intermédiaire sur un slot d'opacité 1 **ne déclenche donc pas automatiquement** un passage translucide ; c'est un choix explicite de l'auteur. Le mesh translucide utilise Weighted Blended Order-Independent Transparency (WBOIT).

L'alpha des pixels n'a **jamais** d'incidence sur la décision de culling effectuée au bake. Il n'existe ni troisième mesh voxel cutout, ni booléen transparent supplémentaire, ni contrat d'occlusion indépendant.
