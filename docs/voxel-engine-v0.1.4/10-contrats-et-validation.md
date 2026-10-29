# Sparkle V0.1.4 — Contrats et validation

[Sommaire de l'architecture](../voxel-engine-v0.1.4-specification.md)

## 8. Garanties transversales et frontières des interfaces

### 8.1 Propriété des données

| Ressource | Propriétaire / représentation | Relation vers les utilisateurs |
|---|---|---|
| `Shape`, `Definition`, `Volume`, `Model` | Données immuables, copies légères et partage interne | Réutilisables indépendamment du Builder d'origine |
| `OcclusionCache` | Propriété de l'application ; table protégée | Mesher lié par pointeur **non propriétaire** |
| `OcclusionResult` | Valeur immuable à données partagées | Survit à `OcclusionCache::clear()` si une copie existe |
| `Material` | Valeur immuable : handles statiques ou source EngineView empruntée | Aucune propriété d'une EngineView ; handles GPU capturés par les snapshots |
| `VoxelPalette` | Valeur immuable, association de Materials | Aucune ressource GPU propre à la palette |
| `AssetManager` | Possède les objets `T` du cache | `load<T>()` retourne `T&` emprunté ; `clear()` invalide |
| `Assembly` | Possède sa hiérarchie mutable ; partage Models/Palettes immuables | Les composants empruntent l'Assembly |
| `Engine` | Moteur logique dans `sparkle::core` | Ne possède ni EngineView ni RenderSnapshot |
| `RenderingEngine : Engine` | Moteur logique + graphiques dans `sparkle::sparkle` | Possède les EngineView et le slot du snapshot moteur |
| `EngineView` | Vue possédée par RenderingEngine | Caméra, layers, framebuffer logique, `Light::Channel` ; instances GL par surface |
| `EngineView::Contributor` | Type imbriqué hérité par RenderingComponent/Behaviour/Entity/System | Contributions aux vues admises par layers via `CaptureContext`, sans possession de vue ni logique d’ombre |
| `ShadowMap` | Abstraction graphique de passe logique d’ombre | Expose Contributor et RenderingSystem imbriqués ; une seule RenderPass par Builder de vue ; clé statique commune envisageable mais non imposée |
| `ShadowMap::Mesh` | Classe de profondeur dérivée de `spk::Mesh<spk::Vector3>` et factory de conversion position-seule | Conversion en nouveau contenu immuable ; copies légères par partage de `Mesh::Content` |
| `ShadowMap::MeshInstance` | Entrée de collection du MeshFilter | Mesh immuable en copie légère et `Matrix4x4` locale détenue par valeur dans le repère de la collection ; `baseTransform` distinct dans MeshFilter |
| `ShadowMap::Contributor` | Composant graphique attachable aux Entity3D | `ShadowMode::None`, layers vides, collection vide et base identité initialement ; **aucune propagation** ; `bind(mesh, matrix)` / `bind(collection, baseMatrix)` remplacent intégralement les données ; découvert par Registry |
| `ShadowMap::RenderingSystem` | RenderingSystem spécialisé | Observe les casters, gère les caches statiques/finals et publie les résultats complets par surface |
| `Light::Channel` / `Prioritizer` | Contexte d'éclairage d'une EngineView | Classement des scores propres à chaque lumière, publication de la sélection immuable |
| `Light::ShadowResults` / `ShadowResult` | Slot par surface et résultat immuable par lumière | Gestion des générations individuelles, des matrices cohérentes et de la fermeture RAII |
| `Lighting::System` | Système Forward | AcquireLatest à l'exécution, sélection des cartes éligibles par surface |
| `CameraComponent` | Component3D graphique lié à une Entity | Accès à ses vues par `camera.view(name)` contrôlé |
| `Window::Surface` / `GPUResourceCollection` | Propriété de l'Application, propre à la fenêtre | Instance OpenGL par State ; notification de destruction permettant de nettoyer les slots d'ombres |
| `EngineWidget` | Widget avec rôles `Update`, `Render`, `Drawing` | `bind(RenderingEngine*)`, non propriétaire ; aucune classe d'affichage secondaire |
| Slot moteur | ThreadSafeSlot de RenderingEngine | Snapshot partagé entre fenêtres via `shared_ptr<const RenderSnapshot>` |
| Slot de fenêtre | ThreadSafeSlot de l'Application par fenêtre | Snapshot UI, capture du snapshot moteur et Drawing |
| Material à source de vue | Material immuable, référence logique empruntée (API à préciser) | Ne maintient pas la vue en vie ; capture de handles uniquement dans les snapshots |
| `RenderSnapshot` et commandes | Conservent les handles de ressources graphiques nécessaires | Aucun accès aux transforms mutables après publication |

### 8.2 Règles d'erreur et de publication

Les opérations publiques invalides, les incompatibilités de slots, les violations des invariants géométriques, les cycles d'assets et les appels de liaison interdits déclenchent des **`spk::Exception` descriptives**. Les opérations de Builder garantissant un état inchangé après échec doivent préserver cette propriété même lors d'une exception technique ; ne pas transformer systématiquement les exceptions d'allocation en `spk::Exception` si cela détruit leur cause.

Aucune Shape, Volume, Model, VoxelPalette ou entrée AssetManager **partiellement construite** ne doit être publiée. Les avertissements d'UV hors `[0,1]` sont, au contraire, **non bloquants** et émis au niveau de la géométrie source, sans répétition par pixel ou par frame.

### 8.3 Niveau de précision des exemples

Les **responsabilités, données, règles de propriété, classifications, algorithmes et comportements décrits ci-dessus** constituent l'architecture à produire. Les exemples illustrent des interfaces possibles, sans obliger à employer un nom secondaire, un emplacement précis de classe, un conteneur particulier, un algorithme de triangulation spécifique ou une nouvelle abstraction de façade lorsque Sparkle dispose déjà du mécanisme nécessaire.

En particulier, l'intégration finale doit s'appuyer sur les vrais `spk::Mesh<TVertex>`, `ShaderStorageBuffer`, `Texture::Handle`, `Registry`, `RenderingSystem`, `RenderSnapshot` et `RenderCommand` disponibles dans la bibliothèque. `EngineView`, `EngineView::Contributor`, les sous-snapshots par vue, les commandes OpenGL typées, la source Material fondée sur une vue et l'héritage `RenderingEngine : Engine` sont des **capacités à introduire ou refactoriser**. `ThreadSafeSlot`, `GPUResourceCollection` et `Framebuffer` existent déjà ; `EngineWidget` existe mais doit adopter les trois rôles configurables. Les détails numériques du clipping, les signatures définitives des Builders et des bindings de textures nommées, les types d'identifiants de Part et les bindings GPU exacts doivent respecter les contrats décrits sans ajouter de comportements implicites.

Le présent texte est **la description de l'architecture souhaitée pour Sparkle V0.1.4**. Il ne constitue pas une autorisation de développement, de fusion ou de modification du code existant.

Les contrats et paramètres restant explicitement non spécifiés doivent être consolidés sans inventer de décisions publiques nouvelles.

### 8.4 Inventaire des responsabilités d'implémentation

Cet inventaire résume les types et adaptations décrits dans les sections précédentes ; il ne crée pas d'API supplémentaire. Les noms marqués **indicatifs** devront être consolidés dans l'interface C++ avant implémentation.

| Classe ou ensemble | Responsabilité à implémenter ou adapter | Référence |
| --- | --- | --- |
| `spk::Engine` | Rester headless ; destructeur polymorphique ; mise à jour de la simulation indépendante des snapshots | §7.1 |
| `spk::RenderingEngine` | Hériter d'`Engine`, posséder les vues, produire et publier le snapshot moteur | §7.1–7.2 |
| `spk::EngineView` / `CaptureContext` | Caméra, layers, résolution, ordre optionnel, politique de sortie, Builder et sous-snapshot | §7.1–7.2 |
| `EngineView::Contributor` / `RenderingComponent`, `RenderingBehaviour`, `RenderingEntity`, `RenderingSystem` | Contributions visuelles filtrées, sans logique d'ombres | §7.2 |
| `spk::EngineWidget` | Liaison non propriétaire ; rôles `Update`, `Render`, `Drawing` et retour explicite à la présentation UI | §7.1–7.2 |
| `spk::RenderSnapshot` / `RenderPass` / `RenderCommand` | Sous-snapshots immuables, délégation ordonnée, commandes de GPU typées réentrantes | §7.1–7.2 |
| `ExecuteViewSnapshotCommand` (**nom indicatif**) | Capturer par valeur le sous-snapshot d'une vue et l'exécuter intégralement dans l'ordre du snapshot moteur | §7.2 |
| `BlendRenderCommand`, `DepthRenderCommand`, `CullFaceRenderCommand`, `ColorMaskRenderCommand`, `FramebufferRenderCommand` | Configurer explicitement les états et cibles GL, y compris blending MRT distinct et depth sans écriture pour WBOIT | §7.2, §7.8 |
| Commandes `Viewport` / `Scissor` (**noms indicatifs**) | Appliquer les états de cible et la résolution d'une vue et restaurer l'UI par le snapshot de fenêtre | §7.2 |
| `CameraComponent`, `Light::Contributor`, `Light::Point`, `Light::Spot`, `Light::Directional` | Caméra et composants 3D lumineux ; volumes pré-calculés, tests géométriques et priorités propres | §7.4 |
| `ShadowMap::Mesh` / `MeshInstance` / `ShadowMap::Contributor` / `ShadowMap::RenderingSystem` | Lier les meshes de profondeur et les transforms ; notifier les invalidations et reconstruire les caches | §7.3, §7.8 |
| `Light::Channel` / `Prioritizer` / `ShadowResults` / `ShadowResult` / `Lighting::System` | Sélection par score, slots par surface, publications immuables et consommation à l'exécution | §7.3–7.4 |
| `spk::Texture` / `spk::Framebuffer` | Cubemaps de profondeur à six faces ; sélection explicite d'une face depth ; formats et attachments WBOIT | §7.4, §7.8 |
| `spk::GPUResource` / `GPUResourceCollection` / `Window::Surface` | Instances par surface, handles immuables, récupération sur le bon contexte et notification de destruction | §7.3–7.4, §8.1 |
| `spk::Voxel::Shape`, `Definition`, `Cell`, `Volume` | Géométrie, slots et volume dense immuables | §2 |
| `spk::Voxel::VoxelMesher`, `OcclusionCache`, `Model` | Bake complet, clipping/occlusion déterministes, deux meshes visuels au maximum et un mesh de profondeur distinct | §3 |
| `spk::Material`, `SpriteSheet`, `VoxelPalette` | Association matériaux/slots, atlas, alpha et sources de texture de vue | §4, §7.1 |
| `spk::AssetManager` / `AssetFactory<T>` | Identités de chargement, factories, cycle de vie et détection de cycles | §5 |
| `spk::Voxel::Assembly`, `ModelRenderingComponent`, `AssemblyRenderingComponent` | Composition articulée, liaison graphique, layers et transforms capturées | §6 |
| `VoxelRenderingSystem`, SSBO et `DrawVoxelBucketCommand` (**nom indicatif**) | Buckets instanciés, rendu voxel opaque et WBOIT ; jamais de commandes de shadow maps | §7.5–7.8 |
| Pipeline Forward classique et WBOIT | Top-N global par EngineView, Point/Spot communs, Directional hors quota, sélection partagée opaque/WBOIT, MRT et composition | §7.8 |

**Contrat d'implémentation :** les fonctions décrites doivent respecter les invariants déjà validés ; toute interface non spécifiée doit être consolidée avant le développement.


### 8.5 Scénarios d'acceptation dérivés des décisions validées

Ces scénarios transcrivent les comportements déjà définis dans ce document en résultats vérifiables. Ils ne constituent ni de nouvelles options de conception ni une obligation de modifier immédiatement le code. Les futurs tests devront utiliser les interfaces effectivement consolidées.

**Géométrie, volumes et meshing**

- Construire des Shapes valides (y compris vides), avec polygones plans convexes, winding CCW et slots nommés ; vérifier qu'une opération invalide laisse le Builder inchangé. Un `build()` échoué permet une nouvelle tentative, tandis qu'un build réussi consomme le Builder. Les UV hors de `[0,1]` produisent un Warning non bloquant lors de la construction, sans répétition par image.
- Vérifier les huit orientations des Cells, y compris le flip vertical, avec winding, normales et UV corrects. Vérifier le stockage dense des Volumes, l'index Air réservé, la lecture par valeur et l'absence de modification d'un ancien snapshot par le Builder issu de ce Volume.
- Vérifier le clipping d'occultation partielle, l'union de polygones opaques frontaux se chevauchant, l'absence d'occlusion par une face arrière, translucide ou intérieure, et la conservation des UV/normales interpolées après triangulation. Tester les six voisins, les cas extérieurs `Empty`, `Occupied`, `Unknown`, et l'émission de tous les `innerShell` dès qu'au moins une direction n'est pas complètement fermée.
- Avec et sans `OcclusionCache`, obtenir les mêmes fragments visibles. Contrôler l'asymétrie A vers B / B vers A, la distinction entre absence et résultat vide, les copies qui survivent à `clear()`, la sécurité des lectures concurrentes et l'absence de réutilisation erronée d'identité après destruction d'une Shape.
- Vérifier que chaque Model garde jusqu'à deux meshes visuels, un mesh d'ombre de positions/indices et **tous** ses slots déclarés par les Definitions utilisées, même totalement occultés. Rejeter les collisions de noms avec opacités distinctes ; conserver un ordre lexicographique stable des indices. Rechanger de Palette ne provoque pas de rebake.

**Assets, Materials, Palettes et Assemblies**

- Deux `AssetManager::load<T>(id, config)` avec type, identifiant et JSON structurel identiques retournent la même référence, indépendamment de l'ordre textuel des clés JSON ; une configuration 16×16 et une 8×8 donnent deux assets distincts. L'absence de configuration équivaut à un **objet JSON vide**, et non à `Null`.
- Tester la construction d'assets non copiables/non déplaçables, le chargement imbriqué, les cycles directs/indirects (erreur descriptive), la possibilité de réessayer après échec, les dépendances déjà chargées qui restent disponibles, la stabilité d'adresse et `clear()` invalidant les emprunts. Aucun accès concurrent non synchronisé n'est présumé sûr.
- Vérifier `SpriteSheet::sprite(coord)`, les UV normalisés, le rôle `albedo` requis par la palette voxel, la possibilité de slots supplémentaires et le rejet d'une association Model/Palette qui demande plusieurs atlas différents **dans la même passe**. Les meshes opaques et translucides peuvent employer deux atlas distincts.
- Vérifier que l'alpha final est `alphaTexture × slotOpacity`, que l'alpha nul écarte le fragment et que l'alpha d'une texture ne change jamais la classification de meshing. Une Palette ou un Assembly copié reste indépendant de l'original pour ses mutations ; le contenu immuable des Models/Materials demeure partageable.

**EngineView, snapshots, layers et multi-fenêtres**

- Créer les vues `surveillance` d'ordre 0 et `main` d'ordre 1 : chacune exécute **son pipeline complet** dans son sous-snapshot avant la suivante. Une vue sans ordre explicite vient après toutes les vues ordonnées, dans son ordre de création. Rejeter les noms et priorités explicites dupliqués.
- Deux surfaces A/B exécutent des snapshots moteurs immuables, éventuellement différents, avec des instances OpenGL et des textures de sortie propres. Une vue produite plus tôt dans **le même snapshot sur B** peut être utilisée par une vue suivante sur B ; une texture seulement produite sur A ne compte pas comme image disponible sur B.
- Après détachement explicite de la caméra, aucune commande de nouveau rendu 3D n'est créée pour cette vue : `KeepLastFrame` conserve l'image locale valide si elle existe, `Invalid` lève une exception à l'utilisation, et `DefaultTexture` emploie la texture configurée ou lève une exception si elle est absente/invalide. Le mode initial est `Invalid`. Un resize explicite ne recycle pas les pixels de l'ancienne cible dans la nouvelle.
- Vérifier que `removeView(name)` supprime la vue de la recherche et des captures futures sans invalider les handles déjà capturés par d'anciens snapshots ; les emprunts C++ vers la vue détruite restent sous la responsabilité de l'appelant. Une construction de snapshot échouée laisse en place la précédente publication.
- Filtre de layers non configuré : tout admettre, même sans layer. `setLayers({})` : n'admettre **aucun** Contributor/composant, y compris sans layer. Avec `{"layerA","layerB"}`, admettre `{"layerA"}` et `{"layerB","layerC"}`, rejeter `{"layerC"}` et une liste vide. Repasser à l'état non configuré doit restaurer l'admission aux captures suivantes et invalider les résultats d'ombres concernés sans changer les anciens snapshots.
- Une commande d'exécution du snapshot moteur dans une fenêtre est immédiatement suivie de la transition explicite aux états GPU de présentation avant dessin des widgets UI. Tester les rôles `Update`, `Render`, `Drawing` indépendamment et combinés, sans supposer une déduplication automatique.

**Ombres, cubemaps, sélection et budgets**

- **Slots d'ombres par surface.** Deux fenêtres sur la même vue partagent le `Light::Prioritizer`, mais disposent de slots `Light::ShadowResult` et de GPUResource natives distincts. Les publications incrémentales remplacent une seule entrée de lumière et gardent les autres générations intactes.
- **Consommation dans la même image.** Reconstruire A en génération 12 dans la passe d'ombres, puis observer cette nouvelle version pendant `Lighting::System::execute()` grâce à `acquireLatest()`, sans changer le `RenderSnapshot`. Une reconstruction de B en parallèle ne doit ni perdre la mise à jour A ni régresser en génération.
- **Conservation des anciennes générations.** Invalider la carte après mouvement de la lumière ou d'un caster ; différer la reconstruction faute de budget : l'ancienne carte complète reste utilisable avec **ses matrices d'origine** si la lumière reste sélectionnée et `castShadows` est actif. Elle ne l'est plus après exclusion, suppression, désactivation, ou fermeture de sa surface ; une lumière jamais reconstruite éclaire sans ombre.
- **Atomicité cubemap.** Pour une PointLight, publier les six faces comme une seule carte cohérente. Construire de **nouvelles ressources GPU logiques** sans réécrire l'ancienne carte, dont les références peuvent subsister ; laisser `GPUResourceCollection` assurer le recyclage.
- **Fermeture des fenêtres.** La notification `Window::Surface::subscribeToDestruction()` retire du dictionnaire `ShadowResults` le slot de cette surface. Recréer une fenêtre avec le même nom ne doit pas réutiliser une ancienne publication.
- **Culling préalable au score.** Un `Point` rejette sa sphère contre les six plans du frustum ; un `Spot` commence par une sphère englobante puis son cône fini. Le rayon et la géométrie locale sont pré-calculés lors de la création et au changement de leurs paramètres. Les deux types retournent score zéro en cas de rejet, sinon un score comparable, et le `Light::Prioritizer` compare uniquement ces scores. Le seuil `E_min` reste une seule constante de Sparkle, valeur numérique à fixer.

- **A2, contrat public :** aucune déclaration C++ de `ShadowPropagation`, aucune surcharge à deux arguments de `setShadowMode`, aucun parcours récursif des modes d'ombres, aucun override d'ancêtre et aucun traitement des relations de collecte graphique pour sélectionner le mode. Les callbacks de modification d'un mode n'invalident que les caches dépendant de ce Contributor ; le lien parent/enfant continue uniquement d'affecter les matrices monde et leur suivi d'invalidation.

- Un `ShadowMap::Contributor` et un `VoxelRenderingSystem` attachés à la même Entity fournissent respectivement géométrie d'ombre et rendu visible, sans commande d'ombre générée par le système voxel. Plusieurs sources et plusieurs lumières dans une vue produisent **une seule RenderPass d'ombres par vue**, mais une paire statique/finale indépendante pour chaque lumière.
- Tester la factory de `ShadowMap::Mesh` depuis plusieurs types de vertices : extraction des positions, conservation de l'ordre des indices, winding, triangles et sommets partagés ; rejeter indice invalide, index count non multiple de trois et position non finie ; accepter un mesh source vide. Aucun contexte GL n'est requis pendant la conversion et le mesh source reste inchangé.
- Tester un `Voxel::Model` après bake : deux meshes visuels au maximum, plus le mesh de profondeur position-seule issu des seuls triangles opaques ; ni palette ni Contributor automatique. Après `bind(shadowMesh)` ou `bind(shadowMesh, matrix)`, destruction des wrappers et du Model original : les meshes des `MeshInstance` conservent leurs données. Après `bind(collection, baseMatrix)`, `bind(collection)`, `bind(collection vide)` ou `unbind()` : remplacement intégral de la collection **et** de la matrice commune, notification unique et invalidation des caches concernés ; pas de fallback visuel ; anciens snapshots et leurs handles/matrices inchangés. Sans instances ou avec seulement des meshes vides, aucun triangle d'ombre n'est soumis et aucune exception n'est produite.
- Valeurs initiales : Contributor facultatif, `ShadowMode::None`, aucun layer, collection vide et matrice commune identité. Avec une vue sans filtre, admission par layers mais aucune projection ; avec une vue à filtre explicite, Contributor sans layer exclu. Ajouter une instance sans sélectionner explicitement `Static` ou `Dynamic` sur **ce même Contributor** ne crée aucune ombre. Aucun parent Entity ou regroupement graphique ne peut changer ce mode.
- `bind(mesh, matrix)` remplace une collection antérieure de plusieurs entrées par une **seule** instance ; `bind(mesh)` utilise une matrice identité ; `bind(collection)` préserve exactement le nombre et l'ordre des instances, y compris un même mesh utilisé plusieurs fois avec différentes matrices, **et remet la matrice commune à l'identité**. `bind(collection, baseMatrix)` conserve les instances et applique la même `baseMatrix` à toutes. `bind(collection vide)`/`unbind()` efface tout et remet la matrice commune à l'identité. Ni add/remove/ID ni fusion CPU des meshes.
- Une `Voxel::Assembly` avec Parent/Enfant articulés est rendue par un Contributor et plusieurs `MeshInstance` dont les matrices locales contiennent **toute la chaîne de Parts depuis le repère de l'Assembly**, sans inclure sa matrice commune ; vérifier `M_entityWorld * M_assembly * M_partAccumulated`, sur des transformations non triviales (rotation et translation de l'Assembly), au changement de rotation d'un parent, au changement de pose d'un enfant et au déplacement de l'Entity. Vérifier l'absence de double application de la matrice d'Entity ou d'Assembly. Une nouvelle pose **ou un déplacement de l'Assembly relativement à l'Entity** nécessite un rebind explicite ; un déplacement global de l'Entity ne rebind ni la collection ni la matrice d'Assembly mais invalide les cartes via Transform3D. Les meshes immuables et leurs indices ne changent jamais.
- Un `bind(collection)` réussi ne diffuse qu'**un** événement même avec plusieurs changements de meshes/matrices ; un échec de validation (matrice commune ou matrice enfant invalide) laisse la collection, la matrice commune et le compteur d'invalidation inchangés. Un `bind()` sur Contributor purement `Dynamic` invalide **seulement la finale** ; sur `Static`, statique et finale, sans changer le comportement distinct de `setShadowMode`. Répéter un bind identique émet un événement selon la politique choisie (pas de comparaison profonde). Les snapshots existants gardent anciennes matrices et handles ; la capture suivante utilise uniquement les nouvelles instances, sans jamais accéder à l'Assembly pendant `execute()`.
- Changer **seulement** la matrice commune par `bind(collection, nouvelleBaseMatrix)` doit repositionner toutes les instances sans modifier leurs matrices locales ou le contenu des meshes, avec une seule notification. Les captures précédentes gardent les anciennes matrices ; un `bind(collection)` ultérieur remet la matrice commune à identité. Vérifier avec plusieurs Parts différentes et une transformation d'Entity non identité.
- Deux instances de même mesh avec matrices différentes produisent deux silhouettes dans une carte de lumière (si toutes deux pertinentes) sans admission en double de la **même instance**. Les paramètres **`ShadowMode` et layers** restent communs au Contributor entier, non configurés par entrée ; `Dynamic` et `Static` utilisent leurs caches respectifs, `None` aucun dessin.
- `None` ne projette pas ; `Static` contribue au cache statique ; `Dynamic` à la finale. Tester un parent `None` et un enfant `Dynamic` : **l'enfant projette** ; parent `Dynamic`, enfant `None` : **l'enfant ne projette pas** ; une Entity seule, un reparenting ou un regroupement graphique ne créent aucune propagation de mode. Modifier le parent ne change jamais le mode des Contributors descendants. Le Registry ne fait aucune déduplication basée sur les chemins hiérarchiques : chaque Contributor admis est parcouru exactement une fois et chaque `MeshInstance` pertinente est dessinée une fois par cible. `setShadowMode(mode)` avec valeur inchangée : aucune notification ; changement de mode : notification RAII et invalidation statique + finale pour les lumières/vues concernées.
- Premier rendu d'une lumière : construire statique puis finale. Sans modification : aucune reconstruction. Changement d'un caster dynamique : ne reconstruire que la finale, après copie de la profondeur statique. Changement statique, lumière ou projection : reconstruire les deux. Deux surfaces conservent des validités et générations distinctes.
- Une PointLight nouvellement créée ne projette pas d'ombre par défaut. Activée et retenue, elle possède **deux cubemaps de profondeur à six faces**, chacune carrée et attachable séparément à un framebuffer depth-only complet. Tester les six projections à 90°, la copie de chaque face de statique vers finale, les casters admissibles dans la portée, le sampling omnidirectionnel et l'absence de contamination entre faces/surfaces.
- Budget souple : pour `X=8`, `Y=12`, `epsilon=0,10`, et `P_X=15`, accepter parmi les suppléments les scores `14,8`, `14,5`, `14`, `13,7`, mais exclure `13`. Comparer **toujours** à 15, jamais au dernier supplément. Vérifier également `X=0`, `X=Y`, `n<X`, `epsilon=0`, score nul, ex æquo et paramètres invalides ; les lumières exclues continuent d'éclairer sans ombre.
- Hystérésis et admissibilité : pour `X=2`, `Y=3`, `epsilon=0,10`, `h=0,20`, avec retenues de scores `[20,10,9,5]`, retirer immédiatement la troisième si elle descend à `8,8` (seuil 9), puis admettre une nouvelle supplémentaire à `9,8` dans la place libre. Une ancienne supplémentaire à `9,2` résiste à une nouvelle à `9,7` si `h=0,20`, mais pas si `h=0,05`. Les X premières sont toujours garanties. Avec `h=0,10`, `10,5` ne remplace pas `10` ; `11,1` le remplace si ces deux lumières sont des supplémentaires admissibles.
- Budget GPU de reconstructions : avec limite 1 par image/surface et deux paires invalides de scores 20 et 5, la demande 20 gagne, **même si sa vue est exécutée plus tard**. Si elle est réinvalidée à chaque image, la demande 5 peut rester indéfiniment sans ombre ni bonus d'attente ; lorsque la carte 20 reste valide, 5 devient reconstructible. Une reconstruction de six faces, statique et finale si nécessaire, ne compte qu'**une** unité ; une paire valide coûte zéro. Tester limite zéro, égalités stables, deux vues partageant la limite d'une surface et deux surfaces aux limites indépendantes.
- Sortie de sélection PointLight : retirer immédiatement sa paire des caches actifs **de cette vue**, pas des autres vues. Un ancien snapshot conserve ses `GPUResource::Handle` jusqu'à libération, puis les instances natives peuvent rejoindre le pool générique de la surface. Une réadmission utilise **de nouveaux State/Handle invalides**, même si une instance native compatible est recyclée ; la nouvelle profondeur n'est utilisable qu'après reconstruction dans le budget. Une PointLight toujours sélectionnée mais simplement différée par le budget garde sa paire active invalide.
- Vérifier la récupération des ressources sur le contexte OpenGL courant correct, notamment au tick sans nouveau rendu et après alternance de surfaces ; ne pas prendre la mise en pool pour une libération immédiate de VRAM. Aucun cache dormant ShadowMap, aucune file d'éviction supplémentaire.

**Éclairage Forward classique et WBOIT**

- Une EngineView sélectionne un Top-N commun Point/Spot, avec N/h configurables, et toutes les Directional actives sans consommer N ; même sélection immuable en entrée de Lighting et ShadowMap.
- Vérifier les invalidations Registry/lumières/caméra, la sérialisation de reconstruction du cache sous mutex et l'absence de recalcul redondant pour plusieurs consommateurs ; des EngineView distinctes peuvent avoir des sélections différentes.
- Vérifier que les shaders opaque et WBOIT utilisent la même liste globale capturée sans culling GPU par tile, et que l'éligibilité aux ombres est un sous-ensemble des lumières de cette liste.
- Une lumière sélectionnée sans carte exploitable éclaire sans ombre ; toutes les ressources GPU de shadow maps restent indépendantes par surface et les générations ne régressent pas.

- Une seule passe géométrique translucide écrit simultanément l'accumulation `RGBA16F` et la révélation `R8` avec blending indépendant, puis la composition s'effectue séparément. La passe translucent utilise le **même depth attachment** que l'opaque, avec test actif et écritures désactivées. Tester les cas de précision `R8`, les mélanges translucides et l'absence de second dessin de la même géométrie.
- Une surface translucide peut recevoir les ombres des objets opaques mais ne produit elle-même aucune commande de projection d'ombres. Les contextes dépourvus des capacités MRT/blending requises sont rejetés explicitement, sans fallback silencieux.
