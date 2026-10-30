# Sparkle V0.1.4 — Architecture cible du moteur voxel

> **Spécification d'architecture, préalable à l'implémentation.** Ce document décrit le comportement et les responsabilités attendus de la bibliothèque, indépendamment de l'ordre dans lequel les décisions ont été prises. Les exemples C++ représentent l'intention de l'API ; une signature illustrative ne constitue pas, à elle seule, une interface définitive. Aucune implémentation de ces nouveaux modules n'est présumée existante.
>
> **Objectif :** offrir au sein de Sparkle une infrastructure voxel publique, générique, utilisable pour des objets, personnages articulés et volumes de cellules, sans introduire de dépendance à un jeu, à un terrain, à Erelia ou à un catalogue global.

Ce fichier est le **sommaire normatif** des huit chapitres de Sparkle V0.1.4. Les chapitres sont répartis par domaine sans supprimer la géométrie voxel, les matériaux, les assets, les Assemblies ou les systèmes de rendu.

## Chapitres

| Chapitre | Spécification |
| --- | --- |
| 1 | [Vue d'ensemble](voxel-engine-v0.1.4/01-vue-ensemble.md) |
| 2 | [Modèle de données voxel](voxel-engine-v0.1.4/02-modele-voxel.md) |
| 3 | [Meshing, visibilité et occlusion](voxel-engine-v0.1.4/03-meshing-occlusion.md) |
| 4 | [Matériaux, textures et palettes](voxel-engine-v0.1.4/04-materiaux-textures-palettes.md) |
| 5 | [AssetManager générique](voxel-engine-v0.1.4/05-asset-manager.md) |
| 6 | [Assembly, composants et scène](voxel-engine-v0.1.4/06-assemblies-scene.md) |
| 7.1–7.2 | [RenderingEngine, EngineView et passes](voxel-engine-v0.1.4/07-vues-et-passes.md) |
| 7.3–7.4 | [Lumières et ombres](voxel-engine-v0.1.4/08-lumieres-et-ombres.md) |
| 7.5–7.8 | [GPU, Forward et WBOIT](voxel-engine-v0.1.4/09-pipeline-gpu-transparence.md) |
| 8 | [Contrats et validation](voxel-engine-v0.1.4/10-contrats-et-validation.md) |

## État consolidé de l'éclairage (Q-TECH-002)

Les arbitrages suivants sont **validés** et décrits de façon normative dans le [chapitre Lumières et ombres](voxel-engine-v0.1.4/08-lumieres-et-ombres.md) et ses [tests](voxel-engine-v0.1.4/10-contrats-et-validation.md) :

- **API lumineuse :** Point, Spot et Directional ont une couleur `spk::RGB` linéaire à trois canaux bornés `[0,1]` et une intensité scalaire finie `>=0` ; Point/Spot ont une portée `range` directement définie en `U`, Spot possède un seul angle, Directional aucune portée. L'ancienne API `spk::Color` (RGBA) conserve son alias lors du renommage `spk::RGBA`.
- **Tone mapping HDR par EngineView (option A validée) :** les contributions d'éclairage restent linéaires HDR ; opaque et WBOIT sont combinés **avant** un tone mapping automatique unique de composition finale, avec exposition propre à chaque vue (`1,0f` par défaut, finie et strictement positive). Pas d'exposition automatique, de saturation précoce, de passe supplémentaire ni d'effet sur les widgets UI. **Q-TECH-003 option B validée :** Reinhard classique `x/(1+x)` par canal RGB, avec `x=max(C_HDR,0)*E` après composition HDR ; la texture finale `GL_SRGB8_ALPHA8` est désormais validée, seuls les détails d'état GL/sRGB restant à préciser ; **Q-ARCH-008 fixe la réutilisation exclusive de la texture finale tone-mappée** comme source de Material, sans autre sortie HDR.
- **Ambiance par EngineView (décision A validée) :** chaque vue **possède directement un unique `spk::AmbientLight`** accessible par référence, sans attachement ni pointeur externe. Il contient `spk::RGB color` (initialement blanc) et `float intensity` (initialement `0`), d'où une contribution ambiante nulle par défaut. Cette contribution est capturée par valeur, modulée par l'albedo et ajoutée uniformément au Forward opaque/WBOIT, sans ombres ni illumination globale. Pas de quatrième `Light::Contributor`, Registry, `Component3D`, Top-N ou collection d'AmbientLight ; les seuls noms précis des accesseurs/setters restent à consolider.
- **Shading :** Point et Spot utilisent `A(x)=1−3x²+2x³` (`x=d/range` borné à `[0,1]`), extinction à `range`, sans inverse du carré ni `d_floor` ; Spot multiplie par un Smoothstep angulaire en cosinus. Directional utilise `RGB×intensity` sans atténuation radiale.
- **Sélection :** score Point/Spot `intensity×range²×priorityWeight/max(distanceCamera,minLightDistance)`, calculé après culling ; sélection Forward classique, sans Tiled Forward+.
- **Ombres :** Point/Spot `near=0,001×range`, `far=range` ; Point six faces à 90° ; **PCF 3×3 fixe pour Point, Spot et Directional** (neuf positions, moyenne des comparaisons) : Spot/Directional utilisent leurs cartes 2D avec offsets ±1 texel, Point utilise une cubemap avec reprojection inter-faces (±1 texel sur la face centrale), règle d'égalité X > Y > Z ; biais automatique par échantillon fondé sur la projection historique ; centre exact non ombré, tap avant near non ombré. Cartes historiques géométriquement cohérentes ; RGB/intensity courants.
- **Casters géométriques uniquement (option A validée) :** les `ShadowMap::Mesh` représentent des triangles de profondeur entièrement opaques sans UV, texture ou test alpha. Une texture albedo trouée ne découpe pas sa shadow map ; l'auteur peut fournir explicitement un mesh d'ombre dont la géométrie correspond aux ouvertures. Les polygones de slots translucides ne projettent pas d'ombres, et changer une palette seule ne reconstruit pas les cartes.
- **Résolution des ombres :** Point, Spot et Directional utilisent **1024×1024 en D32F (`GL_DEPTH_COMPONENT32F`) par défaut**, configurable individuellement par lumière (`shadowResolution`, nom indicatif). Les cartes statique/finale sont de même taille, avec six faces identiques pour Point. Toute modification explicite invalide les cartes concernées sans modifier les anciens snapshots ; aucune adaptation automatique de résolution.
- **Directional :** une seule shadow map orthographique par lumière et par vue ; couverture automatique à partir du frustum de caméra, **plafonnée par `shadowDistance` configurable sur la lumière**, en unités `U` (far infini accepté grâce au plafond fini). Ce paramètre limite uniquement les ombres, jamais l'éclairage ; aucune saisie manuelle des bornes, aucun CSM. La valeur initiale de `shadowDistance` et les détails des bornes de profondeur et casters hors frustum restent à préciser.
- **Statut :** les valeurs et formats de profondeur validés ne sont plus à arbitrer. Restent dans [Q-TECH-002](voxel-engine-v0.1.4-points-a-clarifier.md) les limites de taille et de capacités GPU, les cas numériques aux bornes, les conventions PCF et les détails de couverture des Directional. Aucune implémentation n'est autorisée.

## Questions à résoudre avant l'implémentation

Les **treize arbitrages architecturaux Q-ARCH-001 à Q-ARCH-013 sont clos**, ainsi que **Q-TECH-001** (Top-N et transaction synchronisée des `Light::Channel`), et figurent dans les chapitres normatifs. Le [document des précisions encore ouvertes](voxel-engine-v0.1.4-points-a-clarifier.md) ne conserve **aucune Q-ARCH ouverte** et garde **six Q-TECH** à consolider avant l'implémentation, sans annuler ni rouvrir les contrats validés. Toute précision susceptible de changer un comportement public doit être soumise explicitement au mainteneur avant rédaction de ce nouveau choix.

## Pondération WBOIT — Q-TECH-003 partiellement résolue

**Option B validée :** le Forward translucide utilise la profondeur fenêtre `z` et le poids borné fixe `w=1+7×(1-clamp(z,0,1))`, sans paramétrage public. Accumulation `RGBA16F` additive (`Σ(C*a*w,a*w)`), reveal `R8` multiplicatif (`Π(1-a)`), depth opaque partagé en `GL_LEQUAL` sans écriture translucide, puis mélange HDR `opaque*T + accum.rgb/accum.a*(1-T)` avant exposition/Reinhard ; voir [§7.8](voxel-engine-v0.1.4/09-pipeline-gpu-transparence.md) et [tests §8.5](voxel-engine-v0.1.4/10-contrats-et-validation.md). Les tests de saturation/quantification GPU et le transfert `GL_FRAMEBUFFER_SRGB` aux widgets restent ouverts et conservent **Q-TECH-003** dans le suivi.

## Courbe de tone mapping — Q-TECH-003 partiellement résolue

**Reinhard classique (option B).** Après la composition HDR opaque/WBOIT, chaque `EngineView` applique par canal `C_final = x/(1+x)` avec `x=max(C_HDR,0)*E`, `E` exposition capturée et strictement positive (1 par défaut). Aucun ajustement de luminance, white point, ACES ni courbe alternative ; alpha final 1. Le fond RGB, la Skybox unlit et la scène utilisent tous la même transformation ; les widgets UI sont exclus. Le format de sortie `GL_SRGB8_ALPHA8` est validé ; les états GL et la lecture dans les widgets/Materials doivent éviter les doubles conversions gamma (Q-TECH-003).

## Précisions techniques validées lors de la revue

- **Shadow maps :** Point/Spot/Directional `D32F`, résolution initiale `1024×1024` configurable par lumière. Par EngineView, ombres Point : cible `2`, maximum `4`, tolérance `10 %` et hystérésis des lumières supplémentaires `10 %`, chacune configurable ; indépendantes du Top-N d'éclairage `8` et du budget de reconstruction fixé à `1` par image et par surface par défaut.
- **Pas de raffinement progressif :** reconstruction complète à la résolution configurée lorsque nécessaire et autorisée ; ancienne carte complète admissible conservée en attendant la suivante. Pas de scheduling GPU adaptatif ni de nouveau rendu d'une scène immobile pour affiner les ombres.
- **Sortie :** texture unique `GL_SRGB8_ALPHA8` après Reinhard et encodage sRGB, avec alpha toujours à 1.
- **Données GPU :** échelles rendues homogènes strictement positives, SSBO d'instances avec matrice modèle seule, normales transformées avec `w=0` ; `spk::Vector2` pour UV, `spk::Vector3` pour données tridimensionnelles et RGB, padding explicite minimal vérifié par assertions.
- **Bindings SSBO (Q-TECH-006, option B) :** indices choisis par chaque pipeline, sans réservation globale par Sparkle ; chaque commande de dessin active son programme et tous les SSBO nécessaires sur sa surface **avant chaque draw**. Les handles existants conservent un indice fixe dans leur State : partage possible entre programmes à indice compatible, jamais de rebinding implicite. Voir [§7.5–7.7](voxel-engine-v0.1.4/09-pipeline-gpu-transparence.md) et [tests §8.5](voxel-engine-v0.1.4/10-contrats-et-validation.md).
- **FPS :** cible de 60 configurable par EngineView ; ne constitue ni limiteur de FPS ni contrôleur de raffinement en V0.1.4.

## Invariants de lecture

- **Q-ARCH-009 — fond opaque :** couleur RGB configurable, noire par défaut, alpha final toujours égal à 1.
- **Q-ARCH-010/011/012 — Skybox et Materials :** `spk::Skybox` est **un composant de rendu 3D attachable à toute `Entity3D`**, notamment une caméra, et non une Entity autonome. Il participe au rendu ordinaire par activation/layers et par capture des Contributors, sans `EngineView::setSkybox()`. Ses **six `Material` génériques** sont affectés aux faces intérieures d'un cube inversé ; **tous doivent employer la même source de texture `albedo`**, mais chacun peut utiliser sa propre région UV. Les affectations sont remplaçables par face, et l'égalité de la source est validée à la capture. **Aucune synchronisation propre à Skybox** : ses modifications suivent les règles CPU des RenderingComponents, coordonnées par l'application. La passe système Skybox précède opaque, sans depth test/write, et les snapshots ne lisent aucun composant mutable au rendu.
- **Q-ARCH-008 — sortie unique :** la sortie finale de toute EngineView, après exposition et tone mapping, sert à l'affichage et aux Materials d'autres vues ; les attachments HDR sont internes. Les règles existantes d'ordre des vues, disponibilité par surface/version et fallback restent impératives.

- **Q-ARCH-013 — Skybox non éclairée (option A) :** les Materials restent génériques et peuvent contenir d'autres rôles, mais le renderer Skybox ne consomme que `albedo` et ses UV, sans AmbientLight, Point/Spot/Directional ni shadow maps. Fond HDR, exposition et tone mapping identiques à la vue, sortie opaque. Aucune option de lighting ajoutée au Material ou à la Skybox.
- **Q-ARCH-011 — révision : composant de hiérarchie :** `Skybox` n'est ni une ressource autonome liée à la vue ni une Entity. Elle suit les règles normales des composants 3D attachés aux Entities : découverte, activation, layers et ordre de capture de la hiérarchie. Retirer le composant ou son Entity empêche ses contributions futures ; les snapshots déjà capturés restent valides.
- **Éclairage :** Forward classique avec Top-N global Point/Spot par EngineView, N et hystérésis configurables ; toutes les Directional actives hors quota. `Light::Selection` partagée par Lighting/ShadowMap et par opaque/WBOIT ; aucune grille de culling de lumières par tile.
- **Moteur :** `RenderingEngine : Engine`, Engine logique headless, EngineView appartenant au moteur graphique, snapshots immuables, instances OpenGL distinctes sur chaque surface.
- **Ombres :** `ShadowMap::RenderingSystem` produit ; `Lighting::System` consomme via `Light::Channel` et `ShadowResults` par surface. **Q-TECH-007 validée partiellement : flux GPU unique par `Window::Surface` et publication séquentielle** via `acquireLatest()` → copie de la valeur immuable → fusion → `publish()`, sans mutex de fusion dédié. L'admission reste protégée contre les révocations CPU par le mutex Channel existant. `GPUResourceCollection` gère la durée de vie GPU.
- **Données :** Shape, Definition, Volume, Model, Material et VoxelPalette immuables ; Assemblies mutables et composants de rendu emprunteurs suivant les contrats.
- **Portée :** cette spécification ne constitue pas une autorisation d'implémenter. Aucun comportement public manquant n'est considéré comme validé sans décision du mainteneur.
