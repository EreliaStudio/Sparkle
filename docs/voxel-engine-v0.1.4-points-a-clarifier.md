# Sparkle V0.1.4 — Questions d'architecture et précisions restantes

> **Document de travail — seules les options explicitement ouvertes restent à valider.** Ce fichier complète la [spécification d'architecture](voxel-engine-v0.1.4-specification.md) sur la branche `V0.1.4`, dans la PR #37. Il ne remplace pas les contrats déjà validés dans les chapitres thématiques. **Aucun développement C++ n'est autorisé** par cette liste.

## Mode de suivi

- **Q-ARCH** : arbitrage d'architecture ou de contrat public à soumettre explicitement au mainteneur, **une décision importante à la fois**.
- **Q-TECH** : contrat technique/numerique à rédiger et tester pendant la consolidation, sans demander un arbitrage de conception quand les contraintes approuvées suffisent ; remonter au mainteneur si un choix modifie l'API ou le comportement public.
- Chaque point contient le **déjà validé**, le **reste à définir** et un **critère de clôture**. Quand un point est résolu, intégrer sa décision dans les documents d'architecture, diagrammes et tests correspondants, puis le retirer de cette liste.
- Priorités : **P0** = choix potentiellement bloquant avant implémentation ; **P1** = précision nécessaire à une implémentation vérifiable.
- Les numéros `Q-ARCH-` et `Q-TECH-` sont nouveaux : ils évitent toute confusion avec les anciens identifiants **A1/A2/A4** des décisions déjà clôturées figurant dans les chapitres.

## 1. Arbitrages architecturaux encore ouverts

**Q-ARCH-009 validée (option A) :** fond RGB configurable, noir par défaut ; sortie d'EngineView toujours opaque (alpha=1), y compris avec une Skybox.

**Q-ARCH-010 close — Skybox dédiée validée.** Le besoin exprimé est une classe publique **`spk::Skybox`** qui affiche **six `spk::Material` génériques** sur les **six faces intérieures d'un mesh cubique inversé**. La caméra observe le cube depuis l'intérieur. Le ciel suit la rotation de caméra, mais pas ses déplacements ; il est dessiné avant la scène opaque, sans écriture de profondeur, dans la passe opaque existante. Il ne produit ni éclairage ambiant, ni ombres, ni passe supplémentaire. Le rendu final reste opaque, soumis à la composition HDR et au tone mapping. Ces Materials conservent leurs sources Texture/EngineView, rôles et UV ordinaires, sans assemblage obligatoire en `Texture::Target::TextureCubeMap` GPU ni création d'une classe `spk::Cubemap` : ces deux propositions précédentes sont retirées.

**Q-ARCH-012 — décision révisée : Skybox mutable, Materials ordinaires.** Une `spk::Skybox` associe six `spk::Material` génériques aux faces intérieures de son cube inversé. Le jeu peut remplacer explicitement le Material d'une face sans modifier les autres. `spk::Material` reste immuable : la Skybox remplace une valeur, elle ne change ni ses rôles, ni ses UV ni ses sources. Les six associations sont capturées de manière **cohérente** à chaque snapshot ; les nouvelles captures de toutes les EngineView liées voient les modifications, les snapshots déjà publiés conservent leurs anciens Materials et leurs handles résolus. Synchronisation exacte en Q-TECH-007 ; enum des faces et signatures en Q-TECH-005. Les Materials conservent leurs API, sources Texture ou EngineView, rôles, UV et cycle de vie génériques. Aucun `SkyboxMaterial`, atlas imposé ou `TextureCubeMap` couleur n'est introduit.

**Q-ARCH-013 — option A validée : Skybox non éclairée (unlit).** Les six faces du cube inversé utilisent chacune le `spk::Material` générique affecté à cette face, avec son rôle `albedo`, sa source Texture ou EngineView et sa région UV. **Le shader Skybox ne reçoit ni `AmbientLight`, ni contributions Point/Spot/Directional, ni échantillonnage de shadow maps, ni atténuation ou calcul diffus/spéculaire** : la couleur de fond est indépendante des lumières présentes dans la scène. Il ne crée pas d'éclairage ambiant ou environnemental pour les objets. La valeur albedo est lue en couleur linéaire, écrite dans la cible HDR opaque (sans écrire la profondeur), puis reçoit **l'exposition configurable et le tone mapping unique de l'EngineView**, comme le reste de l'image, avec alpha final=1. Les autres rôles des Materials restent stockables et génériques mais ne sont pas consommés par le rendu Skybox V0.1.4 ; aucun type `SkyboxMaterial` ou flag d'éclairage n'est ajouté à `spk::Material`. Un Material sans rôle `albedo` utilisable pour une face de Skybox provoque une `spk::Exception` descriptive à la validation/capture, sans repli implicite. Les associations face→Material restent mutables selon Q-ARCH-012 ; le choix unlit est fixe pour toutes les Skybox et n'est pas une option par Material ou EngineView.

**Aucun arbitrage Q-ARCH nouveau n'est actuellement identifié.** Le travail restant est la consolidation des Q-TECH-002 à Q-TECH-007, détaillées ci-dessous. La prochaine décision technique utile concerne la **courbe de tone mapping HDR (Q-TECH-003)**, actuellement non définie.

Q-ARCH-001 à Q-ARCH-013 sont closes.

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

**Encore à préciser, sans réécrire les choix précédents :** le traitement des échantillons PCF Spot/Directional hors des UV de leur carte 2D et des références hors volume ; la valeur et la politique extrême de `minLightDistance` pour le classement ; les conversions explicites RGB/RGBA ; la valeur initiale et les validations aux limites de `shadowDistance`, les détails techniques des bornes orthographiques automatiques (profondeur des casters hors caméra, frustum tronqué/cas limite), résolution et précision de l'unique carte directionnelle ; valeur initiale numérique commune des shadow maps (1024×1024 seulement proposé), validation des tailles, limites GPU et format depth des Spot/Point/Directional ; la convention d'égalité exacte à `near`/`far`, les valeurs non finies et la précision numérique aux frontières ; les détails de `B_texel` et des saturations numériques ; tests de continuité PCF aux douze arêtes et huit coins ; valeurs par défaut des budgets de shadow map (X/Y/tolérance et hystérésis) non encore choisies. **Aucun CSM, aucune politique Tiled Forward+, aucun changement de taille de filtre implicite.**

**Clôture :** rendre tous les paramètres restants vérifiables dans §7.4 et §8.5 puis retirer Q-TECH-002 du suivi.

### Q-TECH-003 — WBOIT, tone mapping HDR et états OpenGL
**Priorité : P1** — [GPU, §7.8](voxel-engine-v0.1.4/09-pipeline-gpu-transparence.md)

Préciser valeurs de clear, blending individuel des MRT, pondération d'accumulation, formule de revealage et de composition, gestion du `R8`, restauration d'états GL et comportement des cas limites alpha/profondeur. **Tone mapping HDR validé (option A) :** opaque et accumulation WBOIT en `RGBA16F` linéaire jusqu'à la composition, **tone mapping unique automatique après mélange HDR**, exposition configurable par `EngineView` (défaut `1,0f`, valeur finie strictement positive), capturée dans le snapshot et indépendante des vues. Aucun auto-exposure, nouvelle passe ni correction des widgets UI. **Reste technique :** courbe fixe de tone mapping, encodage sRGB/gamma, format final de sortie et cas extrêmes. **Q-ARCH-009 close (option A) :** sortie toujours opaque (alpha=1), fond RGB configurable et noir par défaut ; Skybox optionnelle à six Materials génériques sur cube inversé (Q-ARCH-010/012 closes) ; Q-ARCH-011 close : Skybox partageable ; Q-ARCH-013 ouverte sur l'éclairage reçu. **Q-ARCH-008 close (option A) :** widgets et Materials utilisent uniquement l'image finale tone-mappée ; aucune source HDR alternative. La courbe de tone mapping HDR reste à choisir dans Q-TECH-003 ; l'encodage/décodage sRGB et les formats/filtrages des six Materials d'une Skybox restent des précisions techniques à consolider. **Une seule passe géométrique translucide**, sans ombres projetées par les surfaces translucides.

### Q-TECH-004 — Précision numérique du VoxelMesher et d'OcclusionCache
**Priorité : P1** — [Meshing, §3](voxel-engine-v0.1.4/03-meshing-occlusion.md)

Préciser tolérances de coplanarité, union/différence de polygones, triangulation déterministe, dégénérescences, interpolation UV, orientations/indices et propriété d'équivalence des résultats avec et sans cache. Prévoir des tests de géométrie reproductibles.

### Q-TECH-005 — Signatures C++23 et adaptation aux interfaces existantes
**Priorité : P1** — [Contrats, §8](voxel-engine-v0.1.4/10-contrats-et-validation.md)

Finaliser signatures de Builders, identifiants de Parts, factories d'assets, API de `spk::Skybox` à six Materials affectables par face, validation du rôle albedo nécessaire au rendu unlit et API des composants et des vues (liaison non propriétaire vers une Skybox, `nullptr` pour délier, Q-ARCH-011 close) (**dont le seul nom de l'accesseur et des setters de son `spk::AmbientLight` possédé ; propriété directe et absence de liaison déjà validées**), `CaptureContext`, abonnements RAII et exceptions. Vérifier l'intégration à `Mesh<T>`, `RenderSnapshot::Builder`, `Texture::Handle`, `GPUResourceCollection`, `Registry` et `VersionedTrait`, sans changer les contrats publics approuvés.

### Q-TECH-006 — Formats CPU/GPU, limites et bindings
**Priorité : P1** — [GPU, §7.5–7.8](voxel-engine-v0.1.4/09-pipeline-gpu-transparence.md)

Décrire les structures d'instances, matériaux et lumières, alignements/tailles SSBO, bindings, limites OpenGL, mesh cubique inversé et UV et bindings Material des six faces de Skybox, shader unlit propre au dessin du ciel sans dépendance aux buffers de lumière, régénération des buffers par snapshot, allocation des formats `RGBA16F`/`R8` et depth cubemap, tests de capacité et validation de commandes réentrantes multi-surface.

### Q-TECH-007 — Synchronisation, fermeture de surface et durée de vie GPU
**Priorité : P1** — [Vues, §7.1–7.2](voxel-engine-v0.1.4/07-vues-et-passes.md) · [Ombres, §7.3](voxel-engine-v0.1.4/08-lumieres-et-ombres.md)

Consolider les règles de capture des six Materials de Skybox depuis une référence non propriétaire et garantir une lecture cohérente lors des remplacements par face (Q-ARCH-012) ; le jeu délie les vues avant destruction de leur Skybox (Q-ARCH-011). Définir les verrous de fusion incrémentale des `ShadowResult` par surface, l'ordre de désabonnement `ContractProvider`, la notification de fermeture, le maintien des canaux nécessaires aux snapshots en vol, les sections critiques lors des captures concurrentes, et le contexte GL valide pour `GPUResourceCollection::reclaimReleased()` même en absence de dessin.

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
- **Q-ARCH-011 :** Skybox autonome, possédée par le jeu et partageable, empruntée explicitement par les EngineView ; le jeu doit délier toutes les vues avant destruction. Snapshots en vol indépendants de l'objet Skybox. Q-ARCH-012 permet de remplacer explicitement ses Materials de faces.
- **Q-ARCH-012 :** Skybox mutable dans ses associations face→Material, `Material` lui-même immuable ; snapshots cohérents. Q-ARCH-013 a validé le shader unlit sans éclairage de scène.
- **Q-ARCH-013 :** les six Materials génériques de Skybox sont rendus sans éclairage : seul `albedo` et ses UV sont exploités, sans AmbientLight, Point, Spot, Directional ni shadow maps ; exposition et tone mapping restent communs à la vue, alpha final=1. Aucun mode éclairé alternatif.
- **Transparence et affichage HDR :** WBOIT avec une passe géométrique MRT et depth opaque partagé ; les surfaces translucides ne projettent pas d'ombres. Opaque et accumulation WBOIT en `RGBA16F` HDR avant composition, puis tone mapping unique avec exposition réglable sur chaque `EngineView`, sans auto-exposure ni traitement des widgets UI.

## 4. Procédure de clôture

1. Prioriser une **seule Q-ARCH** significative, exposer les options et obtenir le choix explicite du mainteneur.
2. Inscrire le contrat adopté **dans le ou les chapitres normatifs**, y compris diagrammes, séquences et tests ; ne pas conserver une alternative comme si elle était encore admise.
3. Supprimer ici la question résolue ; ne conserver que les arbitrages et précisions réellement ouverts.
4. Pour chaque `Q-TECH`, ajouter le contrat vérifiable dans sa section avant l'implémentation et signaler toute découverte nécessitant une nouvelle validation architecturale.
