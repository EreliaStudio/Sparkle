# Sparkle V0.1.4 — Points restant à clarifier

> **Document de travail : uniquement les décisions non résolues.** La [spécification d'architecture](voxel-engine-v0.1.4-specification.md) porte les choix déjà validés, y compris ceux des lumières, des ombres, des générations, des budgets et du filtrage géométrique. Une décision intégrée à la spécification est retirée de cette liste ; elle ne constitue plus une question à reposer.
>
> Les arbitrages A4–A7 ci-dessous concernent le comportement public. Les points T1–T7 concernent surtout la consolidation technique avant l'implémentation ; ils ne demandent pas systématiquement une décision au mainteneur.

## 1. Arbitrages architecturaux encore ouverts

### A4 — Saturation des listes de lumières du Tiled Forward+

Déterminer le comportement observable lorsqu'une tile possède plus de `Point` et `Spot` admissibles que la capacité des listes GPU :

- Capacité bornée avec indicateur d'overflow et chemin de traitement correct de la tile ; ou
- Stockage compact dimensionné selon le culling, avec politique explicite si la mémoire GPU est saturée.

Dans les deux cas, ne jamais perdre silencieusement une lumière, ni traiter le WBOIT différemment de l'opaque. Le choix ne remet pas en cause la grille **2D**, le culling conservateur sur toute la profondeur utile, les mêmes listes pour opaque/WBOIT ou les directionnelles globales.

### A5 — Texture d'une EngineView comme source de Material voxel

- Définir l'interface minimale de liaison **non propriétaire** d'un Material vers une `EngineView` dont la sortie peut changer de résolution.
- Définir une identité stable et la compatibilité avec la contrainte d'un atlas `albedo` par passe / association `Model`–`Palette`.
- Déterminer si la texture de vue est admissible comme `albedo` voxel et la politique observable pour les cycles entre vues.

Les règles de `DetachedOutputPolicy`, de validation CPU/GPU et de textures distinctes par surface sont déjà fixées dans la spécification.

### A6 — Composition d'une Voxel::Assembly dans une autre

Décider si attacher l'Assembly B à l'Assembly A signifie importer/copier ses Parts dans la hiérarchie de A ou conserver B comme sous-Assembly distincte. Définir alors possession, identité des Parts, matrices, animations, modification/détachement et prévention des cycles.

### A7 — Document JSON public d'un Material multi-rôles

Définir un exemple complet de Material avec au moins deux rôles nommés, en précisant :

- La séparation entre identité/configuration de l'asset et coordonnées ou sélection UV spécifiques à chaque rôle ;
- L'équivalence entre JSON, `Material::Builder` et éventuelle `AssetFactory<Material>` ;
- La validation des rôles dupliqués et des sources ou champs incompatibles.

## 2. Précisions techniques restantes avant implémentation

### T1 — Passes de rendu et frontière EngineWidget/UI

- Fixer les noms, clés numériques et plages réservées des `RenderPass` standard, la place des passes personnalisées et le traitement des collisions.
- Préserver l'ordre des vues (ordonnées puis non ordonnées) sans sentinelle numérique qui puisse entrer en conflit.
- Placer la restauration explicite des états GPU de présentation immédiatement après chaque délégation moteur et avant les commandes UI.
- Définir si la passe d'ombres est vide ou absente lorsqu'aucune carte ne nécessite de travail.
- Définir l'API de remise à « non configuré » du filtre de layers d'une `EngineView`, distincte de `setLayers({})`.

### T2 — Publication, invalidation et synchronisation des résultats d'ombres

- Définir la structure C++ précise des entrées d'un `Light::ShadowResult`, de leurs identifiants et générations, et de la liaison à durée de vie sûre entre le `CaptureContext`, le Channel et les `RenderCommand` en vol.
- Préciser l'atomicité de la fusion/publication incrémentale d'un slot par surface, avec concurrence possible des demandes : aucune perte de mise à jour d'une autre lumière et aucune régression d'une génération individuelle.
- Spécifier les notifications nécessaires sur `Light::Contributor`, `ShadowMap::Contributor`, Transform3D, Registry et layers, sans invalider par inadvertance la carte **encore exploitable** pendant sa reconstruction.
- Préciser le déclenchement unique de `Window::Surface::subscribeToDestruction()`, le thread d'émission, les contrats RAII et l'effacement synchronisé du dictionnaire, y compris recréation d'une fenêtre de même nom.
- Définir l'état GPU « carte finale complètement produite » propre à une surface, sans confondre activation d'une ressource et profondeur disponible.
- Réunir avant attribution les demandes de reconstructions ponctuelles de toutes les vues dessinées dans la même image/surface ; garantir le budget commun, le score brut décroissant et l'absence de double comptage en cas de sous-snapshot réexécuté.
- Vérifier `RenderRuntime::tickOnce()` : rendre le **bon contexte** courant avant `GPUResourceCollection::reclaimReleased()`, même sans nouveau snapshot et avec plusieurs surfaces.

**Contrats fixés, à ne pas réarbitrer :** un Channel par EngineView ; un slot de ShadowResult par surface ; souscription à la destruction des surfaces ; `ShadowMap` producteur indépendant de `Lighting` consommateur ; lecture par `acquireLatest()` pendant `execute()` ; publication disponible dans la même image ; ancienne carte complète et anciennes matrices conservées sous sélection/activation ; nouvelles ressources GPU logiques pour reconstruire ; publication même si obsolète sans régression de génération ; fusion incrémentale par lumière ; cubemap six faces atomique ; aucune gestion des ombres dans `Window::State`.

### T3 — Valeurs, unités et qualité des lumières et des ombres

- Fixer **la valeur numérique unique** de la constante Sparkle `E_min`, seuil d'éclairage minimal, et la loi exacte d'atténuation/les unités cohérentes d'intensité. La relation `R = sqrt(I/E_min)` suppose une atténuation inverse du carré de la distance.
- Fixer `minLightDistance`, les validations et les limites de portée calculées ; déterminer les paramètres géométriques exacts du cône Spot, du bounding sphere et les tolérances de tests conservateurs (sphère–plans puis cône–plans), y compris frustums orthographiques.
- Fixer les valeurs initiales et les frontières de configuration du Top-N commun Point/Spot et des paramètres d'ombres ponctuelles `targetShadowCount`, `maxShadowCount`, `shadowPriorityTolerance`, `shadowSelectionHysteresis` et `maxPointShadowRebuildsPerImage`.
- Fixer la couverture et les matrices de projection d'une directionnelle avec **une seule carte sans cascades**, les plans near/far, le cône Spot et les conventions des six faces de Point.
- Fixer la résolution, le biais, la comparaison de profondeur, le PCF/filtrage, les coutures des cubemaps et les diagnostics pour cartes absentes/différées.

**Contrats fixés :** chaque type calcule son score et `Light::Prioritizer` ne fait que comparer ; `Point` teste sa sphère ; `Spot` teste d'abord sa sphère englobante puis son cône fini ; un échec d'intersection donne un score zéro ; la géométrie locale est calculée à la création et recalculée seulement lorsque ses paramètres changent ; le poids de priorité ne modifie pas la portée.

### T4 — Composition WBOIT et états OpenGL

- Fixer les clear values des attachments accumulation/revealage, la fonction de poids, le blending par attachment et la formule de composition.
- Vérifier le depth commun en lecture seule pendant WBOIT, les transitions d'état, la complétude des framebuffers, les bornes numériques et les tests GPU pertinents.

Les formats `RGBA16F` et `R8`, le MRT géométrique unique et la composition séparée sont déjà choisis.

### T5 — Synchronisation de la capture et réentrance des commandes

- Définir la synchronisation entre mutation des Entities/Assemblies/lumières/vues et `refreshRenderSnapshot()`, ainsi que la sécurité des emprunts durant la capture.
- Vérifier des captures CPU distinctes des fenêtres A/B sans déduplication, la durée de vie des Channels après suppression de vue et l'exécution sans lecture d'objets métier mutables.
- Préserver les événements et le `DeviceContext` lors du passage de `RenderingEngine` adaptateur à `RenderingEngine : Engine`.
- Auditer les commandes GPU existantes, les caches UBO et le Profiler pour les sous-snapshots imbriqués.

### T6 — Meshing et OcclusionCache : précision numérique

- Spécifier les tolérances géométriques cohérentes de coplanarité, d'occlusion polygonale, de fermeture et de dégénérescence.
- Définir la triangulation déterministe des polygones convexes et l'interpolation UV après clipping.
- Fixer les clés directionnelles de `OcclusionCache` et les tests d'équivalence avec/sans cache.
- Couvrir rotations, flip et winding, `innerShell`, extérieur `Empty/Occupied/Unknown` et recouvrements partiels.

### T7 — Ajustement aux interfaces Sparkle C++23 et GPU existantes

- Consolider les Builders `Voxel`, Material, Palette et les identités/transformations d'Assembly.
- Finaliser la construction des assets non copiables et la factory `AssetFactory<T>`.
- Relier les API réelles `Mesh<TVertex>`, `Texture::Handle`, `GPUResourceCollection`, SSBO et bindings shaders ; vérifier le bake CPU sans contexte OpenGL.
- Définir les signatures des nouveaux `Light::Contributor`, `Point`, `Spot`, `Directional`, `Prioritizer`, `Channel`, `ShadowResults`, `ShadowResult` et les `ContractProvider` de fermeture ; ne pas créer de service supplémentaire par défaut.
- Consolider les commandes GPU typées d'état, les attachments explicites des faces de cubemap et les liaisons des `ModelRenderingComponent`/`AssemblyRenderingComponent`.

## 3. Clôture

Un point est retiré dès qu'il est **résolu et décrit dans la spécification**, ou explicitement déclaré hors périmètre V0.1.4. Une revue finale d'implémentabilité et de cohérence doit vérifier les responsabilités, signatures, transitions CPU/GPU et tests de validation sans réouvrir les choix déjà approuvés.

Ce document ne constitue ni une autorisation de modification du code C++, ni une modification de la PR #38.
