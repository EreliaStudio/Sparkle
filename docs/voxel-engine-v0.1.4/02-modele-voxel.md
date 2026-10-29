# Sparkle V0.1.4 — Modèle de données voxel

[Sommaire de l'architecture](../voxel-engine-v0.1.4-specification.md)

## 2. Modèle de données voxel

```mermaid
classDiagram
  direction LR
  class Shape {
    +Builder
    +slots locaux et polygones
    -Data immutable partagé
  }
  class Definition {
    +shape
    +correspondance des slots
    -Data immutable partagé
  }
  class Cell {
    +Definition par valeur
    +orientation
  }
  class Volume {
    +Builder
    +dimensions fixes
    +cell(position) Cell
    -grille dense + table locale
  }
  class Model {
    +opaqueMesh()
    +transparentMesh()
    +materialSlots()
    -données immuables partagées
  }
  class VoxelMesher {
    +bake(Volume) Model
    +bind(OcclusionCache*)
    +unbind()
  }
  Shape <-- Definition
  Definition <-- Cell
  Cell <-- Volume
  Volume --> VoxelMesher
  VoxelMesher --> Model
```

### 2.1 `spk::Voxel::Shape` : géométrie locale réutilisable

Une `Shape` représente les surfaces d'une cellule dans le **cube local `[0,1]³`**. Elle ne connaît ni matériaux concrets, ni volume, ni Engine.

Chaque polygone :

- possède au moins trois sommets, de positions flottantes dans le cube unité ; il est **plan et convexe**. Une forme globalement concave est représentée par plusieurs polygones ;
- est **simple face**. Son ordre de sommets est CCW vu du côté avant, et sa normale géométrique est orientée par `(v1 - v0) × (v2 - v0)` pour des sommets initiaux non colinéaires ;
- peut porter des UV arbitraires et des normales d'éclairage par sommet. La normale géométrique est utilisée par défaut pour l'éclairage, mais les normales personnalisées ne modifient jamais l'orientation géométrique ou l'occlusion ;
- référence un **slot géométrique local** déclaré par son nom dans la Shape. Ce slot porte `opacity`, un flottant de `[0,1]`, de valeur `1.0` par défaut.

Pour afficher une feuille ou une pancarte des deux côtés, l'application fournit **deux polygones explicites** de windings opposés. Il n'existe pas de propriété `doubleSided` ni de duplication implicite. Des polygones coplanaires de même winding peuvent se recouvrir, même totalement ; leur existence séparée est conservée. Une Shape de **zéro polygone** est valide.

Les surfaces sont réparties selon leur position :

- `outerShell` : polygones dont tous les sommets sont situés sur l'un des six plans de frontière (`x`, `y` ou `z` égal à `0` ou `1`), identifiables par face et par slot local ;
- `innerShell` : autres polygones, notamment les surfaces inclinées ou internes, qui ne sont jamais projetés artificiellement sur une frontière.

Les positions et les UV ne sont pas confondus : les **positions** doivent être dans `[0,1]³`, mais les **UV** peuvent être hors `[0,1]`. Dans ce dernier cas, la construction/validation CPU de la Shape émet un `Warning` non bloquant via `spk::Logger`. Aucun clamp ou modulo de sprite n'est ajouté automatiquement et cet avertissement n'est jamais répété à chaque frame.

#### Construction et immutabilité

`Shape::Builder` recueille des slots et des polygones, les valide **au moment de leur ajout** et publie une Shape immuable par `build()`. Toute géométrie invalide provoque une `spk::Exception` descriptive à l'ajout. Un ajout qui échoue, quelle qu'en soit la cause, laisse le Builder **strictement inchangé**. Un `build()` échoué laisse également son état entièrement intact, autorisant une nouvelle tentative. **Seul un build réussi consomme le Builder** ; il ne peut plus être exploité pour une nouvelle construction.

Une Shape publiée conserve un bloc `shared_ptr<const Data>` interne : les copies sont légères et partagent les mêmes données et la même identité. Deux Shapes construites indépendamment peuvent avoir des données géométriquement identiques mais des **identités de Data distinctes**. Cette identité est notamment utilisée par le cache d'occlusion. La représentation propriétaire n'est pas exposée dans les API publiques.

### 2.2 `spk::Voxel::Definition` : sens des slots

Une `Definition` immuable associe une `Shape` à une correspondance **slot géométrique de Shape → nom de slot sémantique** destiné au Model. Elle ne contient pas de `Material` et n'ajoute ni catégorie de rendu, ni règle d'occlusion indépendante à l'opacité déclarée dans la Shape.

`Definition` utilise également des données immuables partagées (`shared_ptr<const Data>`) et conserve la durée de vie de sa Shape. Copier une Definition est léger. Une même Shape peut être réutilisée dans différentes Definitions pour produire des noms de slots sémantiques différents.

Sparkle fournit une **Definition Air prédéfinie**. Elle représente une vraie cellule d'air, sans surface occultante ni slot de matériau supplémentaire présumé. Elle est créée automatiquement dans les Volumes ; l'application n'a pas à la fabriquer pour initialiser une grille vide.

### 2.3 `spk::Voxel::Cell` et orientations

L'API publique représente une Cell **par valeur**, contenant sa Definition (copie légère) et une orientation discrète. Les orientations disponibles comprennent les quatre rotations de 90° autour de Y et une possibilité d'inversion verticale, soit **huit orientations**.

L'orientation transforme correctement la géométrie, les UV et les normales. Un flip qui modifie la parité doit également préserver la **face avant physique voulue** par adaptation appropriée du winding/des indices, afin d'éviter qu'une surface visible ou occultante ne se retourne accidentellement.

Une cellule d'air est une Cell valide, et non `null`, un `optional` vide ou un indicateur « absence ».

### 2.4 `spk::Voxel::Volume` : snapshot dense immuable

Un Volume représente une grille **dense et contiguë** de cellules, d'extensions fixes. Chaque position a une entrée, y compris l'air. Pour éviter un `shared_ptr` complet par cellule, le stockage interne repose sur :

1. une **table locale de Definitions** partageant leurs blocs Data ;
2. une grille compacte d'indices vers cette table, avec l'orientation de chaque cellule.

L'index interne `0` est **toujours réservé à Air**. Une Cell d'air référence l'entrée `0`, jamais un sentinel d'absence.

La table locale déduplique une Definition **par identité de `Definition::Data`**, pas par comparaison profonde de contenu. Deux copies d'une même Definition partagent donc un index ; deux Definitions indépendantes au contenu identique peuvent occuper deux entrées. La lecture publique `cell(position)` reconstitue une **Cell par valeur** et n'expose pas les indices ni une référence vers le stockage compact. Ne pas promettre un `span<const Cell>` vers la grille.

`Volume::Builder` peut être initialisé depuis les dimensions d'une nouvelle grille entièrement remplie d'air ou depuis un `const Volume&` existant. Il propose des opérations de remplacement `set(position, Cell)` et de remise à l'air `erase(position)` ; les noms exacts peuvent être ajustés. **Ses dimensions ne changent jamais** : pas de `resize()` ni d'expansion implicite. Les accès hors limites sont invalides. Le produit des dimensions, les tailles d'indices et les allocations doivent être vérifiés avant tout débordement.

Un build réussi publie un **nouveau snapshot immuable**, à copies légères, sans modifier d'anciens Volumes, puis consomme le Builder. Toute exception pendant `build()` laisse l'intégralité de son état inchangé et réutilisable. L'implémentation peut partager des données entre snapshots seulement si cela ne permet aucune mutation rétroactive.

Les classes Shape, Definition, Cell et Volume fonctionnent **sans Engine ni OpenGL**.
