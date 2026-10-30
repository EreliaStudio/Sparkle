# Sparkle V0.1.4 — Questions d'architecture et précisions restantes

> **Document de travail — seules les options explicitement ouvertes restent à valider.** Ce fichier complète la [spécification d'architecture](voxel-engine-v0.1.4-specification.md) sur la branche `V0.1.4`, dans la PR #37. Il ne remplace pas les contrats déjà validés dans les chapitres thématiques. **Aucun développement C++ n'est autorisé** par cette liste.

## Mode de suivi

- **Q-ARCH** : arbitrage d'architecture ou de contrat public à soumettre explicitement au mainteneur, **une décision importante à la fois**.
- **Q-TECH** : contrat technique/numerique à rédiger et tester pendant la consolidation, sans demander un arbitrage de conception quand les contraintes approuvées suffisent ; remonter au mainteneur si un choix modifie l'API ou le comportement public.
- Chaque point contient le **déjà validé**, le **reste à définir** et un **critère de clôture**. Quand un point est résolu, intégrer sa décision dans les documents d'architecture, diagrammes et tests correspondants, puis le retirer de cette liste.
- Priorités : **P0** = choix potentiellement bloquant avant implémentation ; **P1** = précision nécessaire à une implémentation vérifiable.
- Les numéros `Q-ARCH-` et `Q-TECH-` sont nouveaux : ils évitent toute confusion avec les anciens identifiants **A1/A2/A4** des décisions déjà clôturées figurant dans les chapitres.

## 1. Arbitrages architecturaux encore ouverts

**Aucun.** Les décisions Q-ARCH-001 à Q-ARCH-013 sont closes, y compris la révision de Skybox en **composant de rendu attachable à une Entity3D**, sans liaison particulière à `EngineView`. Les décisions validées figurent dans les chapitres normatifs et dans le rappel de la section 3 ; elles ne doivent pas être reproposées. Six groupes de consolidation technique (Q-TECH-002 à Q-TECH-007) demeurent ouverts.

## 2. Précisions techniques à consolider

### Q-TECH-002 — Paramètres lumineux, projections et cas limites restants
**Priorité : P1** — [Lumières et ombres, §7.4](voxel-engine-v0.1.4/08-lumieres-et-ombres.md) · [Scénarios de validation, §8.5](voxel-engine-v0.1.4/10-contrats-et-validation.md)

**Décisions déjà validées — ne pas rouvrir :**
- Point/Spot/Directional : couleur `spk::RGB` linéaire, canaux bornés à `[0,1]`, intensité scalaire `float` finie `>=0` sans plafond artificiel ; `spk::Color` devient `spk::RGBA`, avec `using Color = RGBA;`. Point/Spot ont une portée `range` indépendante de l'intensité, Spot un seul angle, Directional n'a pas de portée et utilise l'orientation de `Transform3D`.
- Point/Spot : atténuation radiale cubique Smoothstep `1-3x²+2x³`, `x=d/range` dans `[0,1]`, nulle à ou au-delà de `range`, ou pour `range=0`. `d_floor`, l'inverse du carré et `E_min` ne sont **plus** applicables. Spot multiplie par le Smoothstep angulaire en cosinus, de l'axe au bord du cône ; Directional contribue `RGB×intensity` sans décroissance de distance.
- Ambiance par EngineView (décision A validée) : chaque vue **possède directement un unique `spk::AmbientLight`**, accessible et configurable par référence, sans pointeur externe ni liaison `bind()`. `spk::RGB` linéaire `[0,1]` (défaut blanc) et intensité finie `>=0` (défaut `0`, contribution nulle). `RGB×intensity` capturé par valeur et appliqué à l'albedo opaque/WBOIT. Pas de `Light::Contributor`, Registry, Transform3D, Top-N, ombres ou lumière indirecte. Pas de collection d'ambiances dans la scène.
- Top-N Point/Spot : `score = intensity × range² × priorityWeight / max(distanceCamera,minLightDistance)` après culling ; RGB exclu du score ; les Directional actives restent hors Top-N.
- Projections d'ombres Point/Spot : `near=range×0,001`, `far=range` ; six perspectives 90° pour Point. `range=0` interdit la construction d'une carte ; une projection non représentable déclenche `spk::Exception`, sans clamp caché. Récepteur avant `near` : échantillon non ombré.
- Biais automatique appliqué au **test** (pas à la génération de carte), avec `constantFactor`/`slopeFactor` publics (défaut 1), `max(B_texel,ULP(z)) × (constantFactor + slopeFactor×sinθ)`, dépendant du format, de la résolution, de la normale géométrique et de la projection historique.
- Directional (option A validée) : **une seule carte d'ombres orthographique par lumière et EngineView**, avec zone automatiquement définie depuis le frustum capturé de la caméra, **plafonnée par `shadowDistance` configurable sur chaque `Light::Directional`** en `U` ; plan lointain effectif `min(cameraFar, shadowDistance)` ou `shadowDistance` si `cameraFar` est infini. Le paramètre ne limite pas l'éclairage de la Directional, uniquement les ombres. Pas de saisie manuelle du volume ni CSM ; conserver les casters pertinents hors frustum caméra et les paramètres historiques des générations.
- Résolution des shadow maps (option A validée) : même résolution standard initiale pour Point, Spot et Directional, **paramètre individuel par lumière** (`shadowResolution`, nom indicatif), longueur d'un côté carré en texels ; deux cartes 2D statique/finale pour Spot et Directional, deux cubemaps à six faces carrées pour Point. Reconfigurer invalide les cartes de cette lumière, crée de nouvelles ressources lors de la reconstruction et préserve les résolutions historiques des anciennes cartes encore admissibles. Aucune adaptation automatique à la vue ou à la distance ; aucun clamp GPU implicite.
- Point/Spot/Directional PCF : **filtre `3×3` fixe (9 positions) pour tous les types**, moyenne des résultats de comparaison, aucune variante publique ; Spot et Directional utilisent chacun les neuf UV décalés à ±1 texel dans leur carte de profondeur 2D, avec projection et biais historiques respectifs. Point conserve son filtre manuel inter-faces : `nearest`, noyau aligné UV sur la face centrale à ±1 texel ; chaque tap reprojeté en 3D vers sa face réelle, comparaison, référence et biais historiques propres à cette face ; départage `X > Y > Z` aux égalités, sans mélange des noyaux. Au centre exact `P=L`, visibilité 1 et aucune texture lue ; normalisation robuste des vecteurs non nuls ; tap avant `near` = visible sans accès profondeur.

**Encore à préciser, sans réécrire les choix précédents :** le traitement des échantillons PCF Spot/Directional hors UV et hors volume ; la valeur initiale et la politique extrême de `minLightDistance` ; les conversions explicites RGB/RGBA ; la valeur par défaut et les bornes de `shadowDistance`, ainsi que la projection orthographique couvrant les casters pertinents hors frustum caméra ; la validation des tailles de shadow maps et des capacités GPU **pour le format D32F et la résolution initiale déjà fixée à 1024×1024** ; les égalités exactes à `near`/`far`, valeurs non finies, précision aux frontières et détails de `B_texel` ; les tests PCF sur les arêtes et coins. **Le budget Point X=2/Y=4, la tolérance de 10 %, l'hystérésis de 10 % par vue et le budget de 1 reconstruction complète par image/surface sont déjà validés** et ne sont plus des questions. Aucun CSM, Tiled Forward+ ni raffinement progressif.

**Clôture :** rendre tous les paramètres restants vérifiables dans §7.4 et §8.5 puis retirer Q-TECH-002 du suivi.

### Q-TECH-003 — WBOIT, tone mapping HDR et états OpenGL
**Priorité : P1** — [GPU, §7.8](voxel-engine-v0.1.4/09-pipeline-gpu-transparence.md)

**Décision partielle Q-TECH-003 — Reinhard classique, option B validée.** La dernière passe système `sparkle.engineview.composition` mélange d'abord les contributions opaques, WBOIT et fond en **RGB linéaire HDR**. Elle applique ensuite **une seule fois** l'exposition positive `E` capturée dans l'EngineView, puis le tone mapping de **Reinhard classique par canal** :

```text
C_nonnegative = max(C_HDR, 0)          // séparément pour R, G et B
x = C_nonnegative * E                 // E = 1.0 par défaut, strictement positif et fini
C_reinhard = x / (1 + x)
alpha_final = 1
```

La conversion est **par composante**, sans calcul de luminance, normalisation de couleur, white point, courbe ACES, paramètre de courbe configurable ou exposition automatique. Les valeurs HDR positives ne sont pas écrêtées avant cette étape ; une valeur négative est ramenée à zéro **à l'entrée du tone mapping seulement**. L'implémentation peut utiliser une forme numériquement équivalente et stable pour ne pas produire `inf/inf` en cas de dépassement de `C_nonnegative * E` ; l'extrémité lumineuse doit tendre vers 1, sans NaN. À `E=1`, `C_HDR` 0→0, 0,5→1/3, 1→1/2, 2→2/3, 4→4/5, 9→9/10 ; à `E=2`, `C_HDR=2`→4/5. Les valeurs RGB HDR non finies ne sont pas des valeurs de couleur valides ; leur diagnostic GPU demeure dans les cas limites de Q-TECH-003. Cette étape inclut le fond RGB ou la Skybox unlit, mais non les widgets UI ultérieurs. **Le format final `GL_SRGB8_ALPHA8` et l'encodage sRGB unique après Reinhard sont déjà validés : seule la composition WBOIT numérique, les états GPU et les cas limites restent à consolider dans Q-TECH-003.**

Préciser valeurs de clear, blending individuel des MRT, pondération d'accumulation, formule de revealage et de composition, gestion du `R8`, restauration d'états GL et comportement des cas limites alpha/profondeur. **Tone mapping HDR validé (option A) :** opaque et accumulation WBOIT en `RGBA16F` linéaire jusqu'à la composition, **tone mapping unique automatique après mélange HDR**, exposition configurable par `EngineView` (défaut `1,0f`, valeur finie strictement positive), capturée dans le snapshot et indépendante des vues. Aucun auto-exposure, nouvelle passe ni correction des widgets UI. **Choix Reinhard classique validé (B) :** `x=max(C_HDR,0)*E`, `C_final=x/(1+x)` par canal après composition HDR, sans normalisation par luminance, alpha final=1. **Format final validé :** `GL_SRGB8_ALPHA8`, écriture sRGB unique après Reinhard, lecture linéarisée pour les Materials. **Reste technique :** gestion effective de `GL_FRAMEBUFFER_SRGB` selon les targets/widgets, rendu WBOIT, états de blend, diagnostics de valeurs non finies et autres cas limites. **Q-ARCH-009 close (option A) :** sortie toujours opaque (alpha=1), fond RGB configurable et noir par défaut ; Skybox optionnelle à six Materials génériques sur cube inversé (Q-ARCH-010/012 closes) ; Q-ARCH-011 close : Skybox partageable ; Q-ARCH-013 close (A) : Skybox non éclairée, seule la source `albedo` est utilisée, puis exposition/Reinhard par EngineView. **Q-ARCH-008 close (option A) :** widgets et Materials utilisent uniquement l'image finale tone-mappée ; aucune source HDR alternative. La courbe Reinhard classique est validée dans Q-TECH-003 ; l'encodage/décodage sRGB et les formats/filtrages des six Materials d'une Skybox restent des précisions techniques à consolider. **Une seule passe géométrique translucide**, sans ombres projetées par les surfaces translucides.

### Q-TECH-004 — Précision numérique du VoxelMesher et d'OcclusionCache
**Priorité : P1** — [Meshing, §3](voxel-engine-v0.1.4/03-meshing-occlusion.md)

La **tolérance de clipping du Mesher est déjà fixée à `1e-5` par défaut et configurable**, conformément au chapitre 03. Consolider ses règles d'application et la précision restante : coplanarité, union/différence de polygones, triangulation déterministe, dégénérescences, interpolation UV, orientations/indices et équivalence des résultats avec et sans cache. Prévoir des tests reproductibles ; ne pas rouvrir la valeur initiale.

### Q-TECH-005 — Signatures C++23 et adaptation aux interfaces existantes
**Priorité : P1** — [Contrats, §8](voxel-engine-v0.1.4/10-contrats-et-validation.md)

Finaliser signatures de Builders, identifiants de Parts, factories d'assets et API de **`spk::Skybox` comme composant ordinaire de `Entity3D` / `RenderingComponent3D`**, avec six Materials remplaçables et rôle albedo obligatoire (aucun `setSkybox()` dans EngineView) ; consolider les signatures et conventions de transformation/capture dans les interfaces de composants existantes **sans règle spéciale de nombre ou de sélection de Skybox**. Préciser l'accesseur et les setters de l'`spk::AmbientLight` possédé par EngineView, ainsi que `targetFPS` (défaut 60), `CaptureContext`, abonnements RAII et exceptions. Vérifier l'intégration à `Mesh<T>`, `RenderSnapshot::Builder`, `Texture::Handle`, `GPUResourceCollection`, `Registry` et `VersionedTrait`, sans changer les contrats publics approuvés.

### Q-TECH-006 — Formats CPU/GPU, limites et bindings
**Priorité : P1** — [GPU, §7.5–7.8](voxel-engine-v0.1.4/09-pipeline-gpu-transparence.md)

Décrire les structures d'instances, matériaux et lumières avec types sémantiques `Vector2`/`Vector3`, padding `SPK_PADDING(n)` minimal et tests de layout (`sizeof`, `alignof`, `offsetof`), bindings, limites OpenGL, mesh cubique inversé, UV et bindings Material des six faces de Skybox, shader unlit propre au dessin du ciel sans dépendance aux buffers de lumière, matrice modèle unique (pas de matrice normale) et échelles homogènes positives, régénération des buffers par snapshot, allocation des formats `RGBA16F`/`R8`, sortie `SRGB8_ALPHA8` et depth cubemap `D32F`, tests de capacité et validation de commandes réentrantes multi-surface.

### Q-TECH-007 — Synchronisation, fermeture de surface et durée de vie GPU
**Priorité : P1** — [Vues, §7.1–7.2](voxel-engine-v0.1.4/07-vues-et-passes.md) · [Ombres, §7.3](voxel-engine-v0.1.4/08-lumieres-et-ombres.md)

Consolider la capture cohérente des six Materials d'un **composant Skybox attaché à une Entity3D**, sans lecture du composant ni de l'Entity mutable dans `execute()` (Q-ARCH-012). Définir les verrous de fusion incrémentale des `ShadowResult` par surface, l'ordre de désabonnement `ContractProvider`, la notification de fermeture, le maintien des canaux nécessaires aux snapshots en vol, les sections critiques des captures concurrentes et le contexte GL valide pour `GPUResourceCollection::reclaimReleased()` même sans dessin.

## 3. Décisions validées à ne pas rouvrir

Ce rappel sert seulement à **exclure les faux points ouverts** :

- **AmbientLight :** chaque EngineView possède directement un objet ambiant unique, accessible par référence, RGB blanc et intensité nulle par défaut, capture immuable ; aucun pointeur externe, attachement ni liste de lumières ambiantes, aucune contribution aux budgets des autres lumières.
- **Éclairage Forward classique**, sans Tiled Forward+, sans listes GPU par tile ni politique de saturation ; sélection globale `Light::Prioritizer` **Top-N commun Point/Spot**, Directional actives hors quota ; sélection immuable commune aux consommateurs.
- **Sous-snapshots par EngineView**, intégralement exécutés dans l'ordre choisi par le jeu ; aucun ordonnanceur global caché ni dépendance automatique entre vues ; resize explicitement piloté par le jeu.
- **Ombres :** producteur `ShadowMap::RenderingSystem`, consommateur `Lighting::System` ; `ShadowMode` de chaque Contributor indépendant de sa hiérarchie, pas de propagation ; aucune shadow map générée par `VoxelRenderingSystem`. **Option A validée : ombres entièrement géométriques**, chaque triangle d'ombre admis est opaque sans échantillonner d'UV, albedo ou alpha. Les textures trouées ne découpent pas les ombres ; un mesh d'ombre explicitement ajouré est nécessaire. Pas de catégorie cutout ni d'invalidation liée au seul changement de palette/texture.
- **PointLight :** budget souple X/Y/tolérance/hystérésis pour retenir des ombres ; budget commun de reconstruction par image et surface, priorité stricte des scores, sans bonus d'attente ; sortie de sélection = éviction immédiate des **caches logiques actifs**, récupération GPU par l'infrastructure générique.
- **Publication d'ombres :** cubemap de six faces atomique, publication incrémentale par lumière, anciens résultats complets conservés sans régression si encore admissibles ; aucune file d'éviction ou recyclage spécialisé ShadowMap.
- **Voxel :** Volume/Model/Material/VoxelPalette immuables après construction, meshing synchrone ; un atlas par mesh/passe par association Model/Palette ; Assembly animée explicitement ; casters d'ombres définis par le jeu et non inférés du système voxel.
- **Q-ARCH-008 :** une sortie finale tone-mappée par EngineView, utilisée par les widgets et les Materials ; aucune sortie HDR supplémentaire ou choix public d'étape de rendu.
- **Q-ARCH-009 :** fond RGB noir par défaut et sortie finale alpha=1, même avec une Skybox.
- **Q-ARCH-010 :** `spk::Skybox` dédiée au dessin de six Materials ordinaires sur un cube inversé, sans nécessité de TextureCubeMap GPU.
- **Q-ARCH-011 (révisée) :** `spk::Skybox` est un **composant graphique attachable à une Entity3D** et découvert comme les autres RenderingComponents selon activation, hiérarchie et layers ; ce n'est pas une Entity. Il contribue à la RenderPass Skybox avant opaque sans depth write, sans liaison `EngineView::setSkybox()` ni logique de sélection/unicité. Les snapshots conservent leurs ressources après la destruction du composant ou de son Entity.
- **Q-ARCH-012 :** Skybox mutable dans ses associations face→Material, `Material` lui-même immuable ; snapshots cohérents. Q-ARCH-013 a validé le shader unlit sans éclairage de scène.
- **Q-ARCH-013 :** les six Materials génériques de Skybox sont rendus sans éclairage : seul `albedo` et ses UV sont exploités, sans AmbientLight, Point, Spot, Directional ni shadow maps ; exposition et tone mapping restent communs à la vue, alpha final=1. Aucun mode éclairé alternatif.
- **Q-TECH-003 (courbe validée) :** Reinhard classique RGB par canal `x/(1+x)` avec `x=max(0,C_HDR)*E`, après mélange HDR opaque/WBOIT et fond, alpha final=1. Sortie unique `GL_SRGB8_ALPHA8` validée ; seules les conventions d'état GL restent à fixer.
- **Q-TECH-002 validé partiellement :** `D32F` et résolution initiale `1024×1024` **fixés** pour Point/Spot/Directional, réglables par lumière ; sous-sélection d'ombres Point par EngineView `X=2`, `Y=4`, tolérance `10 %` et hystérésis des suppléments `10 %`, configurables indépendamment ; budget de reconstruction Point `1` par image et par surface. Reconstruction complète à taille fixe, sans raffinement progressif ni ordonnanceur FPS adaptatif. Restent les limites GPU et les conventions numériques.
- **Q-TECH-006 validé partiellement :** vrai `Vector2` pour les UV, vrai `Vector3` pour les données 3D, padding seulement si nécessaire, transformation normale par modèle seul pour échelle homogène positive.
- **FPS :** cible 60 configurable sur EngineView, sans limiteur ni redessin supplémentaire pour raffinement.
- **Transparence et affichage HDR :** WBOIT avec une passe géométrique MRT et depth opaque partagé ; les surfaces translucides ne projettent pas d'ombres. Opaque et accumulation WBOIT en `RGBA16F` HDR avant composition, puis tone mapping unique avec exposition réglable sur chaque `EngineView`, sans auto-exposure ni traitement des widgets UI.

## 4. Procédure de clôture

1. Prioriser une **seule Q-ARCH** significative, exposer les options et obtenir le choix explicite du mainteneur.
2. Inscrire le contrat adopté **dans le ou les chapitres normatifs**, y compris diagrammes, séquences et tests ; ne pas conserver une alternative comme si elle était encore admise.
3. Supprimer ici la question résolue ; ne conserver que les arbitrages et précisions réellement ouverts.
4. Pour chaque `Q-TECH`, ajouter le contrat vérifiable dans sa section avant l'implémentation et signaler toute découverte nécessitant une nouvelle validation architecturale.
