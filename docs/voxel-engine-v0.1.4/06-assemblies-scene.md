# Sparkle V0.1.4 — Voxel::View, composants et scène

[Sommaire](../voxel-engine-v0.1.4-specification.md)

## 6. View, modèles et scène

### 6.1 Séparation entre Model et View

`spk::Voxel::Model` est une ressource immuable de géométrie produite par `spk::Voxel::Mesher::bake(Volume)`. Il contient un mesh opaque, un mesh translucide, un mesh de profondeur et les slots de matériaux nécessaires, avec leurs opacités. **Le Model ne contient ni palette ni transformation.** Les copies de Model partagent ses meshes via une donnée immuable.

`spk::Voxel::View` est l'unique objet voxel porteur d'une apparence et d'une hiérarchie animable. Elle remplace l'ancien concept `Model::View` et l'ancien `Voxel::Assembly`. Une View avec une seule Part et un seul Model n'est pas un type spécial : c'est exactement la même classe que pour un personnage composé.

### 6.2 Parts et animation

Une `View::Part` contient un `PartID` monotone, un nom, un parent facultatif, une transformation locale `spk::Matrix4x4` et **`std::vector<Model> models`**. Une Part peut porter zéro Model (point d'attache), un ou plusieurs Models. Les Models d'une même Part partagent sa pose. `View::part(id)` retourne seulement `const Part&` ; aucune modification directe de Models ou de palettes n'est possible.

Les modifications passent par `addPart(name,parent,models,transform)`, `addModel(id,model)`, `setModel(id,index,model)`, `removeModel(id,index)`, `setName`, `setLocalTransform`, `setParent` et `removePart`. La hiérarchie peut avoir plusieurs racines ; les cycles sont refusés. La suppression d'une Part retire également ses descendants. `roots()`, `children()`, `partIDs()` et `accumulatedTransform()` sont les accès de lecture.

### 6.3 Palette unique

`View::palette()` retourne `nullptr` si aucune palette n'a été liée. `View::bind(spk::Palette)` vérifie le contrat de palette voxel : chaque Material possède un albedo valide et tous les albedos partagent un atlas unique. Elle vérifie ensuite la présence de tous les slots requis par **tous les Models** de toutes les Parts, y compris les slots occultés. En cas d'incomplétude, `spk::Exception` décrit les noms manquants et la liaison précédente reste intacte.

Une View sans palette est **constructible mais pas prête pour le rendu**. On peut y ajouter librement des Models. Une fois la palette liée, l'ajout ou le remplacement de Models et l'insertion d'une autre View exigent que chaque Model soit compatible avec cette palette. Les opérations incompatibles échouent sans modifier la View. Supprimer une Part ou un Model ne modifie jamais la palette. **Aucune fusion automatique de palettes n'existe.** Une View ne possède qu'une seule palette et aucune Part n'en possède.

### 6.4 Exemples

```cpp
spk::Voxel::Mesher mesher;
spk::Voxel::Model swordModel = mesher.bake(swordVolume);
spk::Voxel::View sword(swordModel, swordPalette); // Une Part, un Model.

spk::Voxel::View character;
auto torso = character.addPart("torso", {}, {torsoModel, armorModel});
auto arm = character.addPart("arm", torso, {armModel, shoulderArmorModel});
auto hand = character.addPart("hand", arm); // Point d'attache vide.
character.bind(characterPalette);

character.setLocalTransform(arm, armRotation);
auto result = character.insertView(hand, sword, insertionTransform);
character.invalidate();
```

L'insertion ci-dessus n'utilise **que la palette de `character`**, jamais celle de `sword`, qui reste autonome. Tous les Models de la Part `arm` utilisent la même matrice cumulée.

### 6.5 Insertion par copie et expansion

`insertView(parent, source, attachmentTransform, anchorName)` crée une Part d'ancrage sans Model sous le parent de destination. Toutes les racines de la View source sont recopiées sous cet ancrage ; le parcours est déterministe, parents avant enfants. Chaque Part importée reçoit un PartID neuf. Le résultat `{anchor, partIDs}` fournit l'ancrage et la correspondance de tous les PartIDs source vers destination.

L'insertion copie les Models et matrices, **pas la palette source**. Elle refuse l'auto-insertion, les parents inconnus, les transforms non finis, les cycles ou hiérarchies sources invalides, les IDs épuisés et les Models incompatibles avec une palette déjà liée à la destination. La destination prépare un nouvel arbre, puis le publie atomiquement en cas de succès. Une View source vide produit seulement l'ancrage ; plusieurs racines restent prises en charge. Les Parts importées ne sont jamais liées vivantes à la View source.

### 6.6 Versions, ombres et composants de rendu

La copie et l'affectation d'une View reconstruisent un `VersionedTrait` indépendant en version 1, sans abonnements copiés. Les mutations ordinaires et les changements de palette n'appellent **pas** automatiquement `invalidate()` ; le jeu le déclenche lorsque nécessaire. Les anciennes captures restent indépendantes.

Pour chaque occurrence de Model d'une Part, le rendu calcule `M_world = M_entityWorld * M_view * M_parent1 * ... * M_part`. Aucun transform de View source n'est appliqué deux fois à une copie insérée. Les ombres s'appuient sur les meshes de profondeur existants et une capture explicite de leurs matrices ; aucun Contributor ou ShadowMode implicite.

Le futur `ViewRenderingComponent` emprunte une `spk::Voxel::View`, sans la posséder. Il remplace les deux concepts de composants `ModelRenderingComponent` et `AssemblyRenderingComponent`. Les layers sont gérés par le composant graphique suivant les conventions existantes. Le système de rendu capture les Models, transformations et la palette avant tout dessin GPU.

### 6.7 Indices de matériaux entre Models

`Model::materialSlots()` représente une table locale au Model, triée lexicographiquement. Les `materialIndex` des sommets des meshes opaque et translucide réutilisent **la même table locale**. Les Models distincts peuvent donc utiliser un même index pour des noms différents.

`View::materialIndices(partID, modelIndex)` résout cette table locale vers les indices de la palette commune (ordre déterministe de `spk::Palette::Materials`, lexicographique par nom). Elle nécessite une palette liée et renvoie un `std::vector<uint32_t>` : `remap[materialIndex]` fournit l'index de Material commun à la View. L'indice local porté par le sommet n'est jamais réécrit. La table sera fournie au futur pipeline GPU, par Model ou par bucket, sans rebake ni modification de géométrie. Une nouvelle palette peut modifier cette correspondance ; elle est alors recalculée pour la capture suivante.

L'opacité par slot demeure dans `Definition::Slot {name, opacity}` : un simple `vector<string>` la ferait disparaître, alors qu'elle sert à la classification opaque/translucide et au facteur `alphaTexture × slotOpacity`.
