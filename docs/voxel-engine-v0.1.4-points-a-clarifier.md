# Sparkle V0.1.4 — Questions d'architecture et précisions restantes

> **Document de travail — aucune question ci-dessous n'est une décision validée.** Ce fichier complète la [spécification d'architecture](voxel-engine-v0.1.4-specification.md) sur la branche `V0.1.4`, dans la PR #37. Il ne remplace pas les contrats déjà validés dans les chapitres thématiques. **Aucun développement C++ n'est autorisé** par cette liste.

## Mode de suivi

- **Q-ARCH** : arbitrage d'architecture ou de contrat public à soumettre explicitement au mainteneur, **une décision importante à la fois**.
- **Q-TECH** : contrat technique/numerique à rédiger et tester pendant la consolidation, sans demander un arbitrage de conception quand les contraintes approuvées suffisent ; remonter au mainteneur si un choix modifie l'API ou le comportement public.
- Chaque point contient le **déjà validé**, le **reste à définir** et un **critère de clôture**. Quand un point est résolu, intégrer sa décision dans les documents d'architecture, diagrammes et tests correspondants, puis le retirer de cette liste.
- Priorités : **P0** = choix potentiellement bloquant avant implémentation ; **P1** = précision nécessaire à une implémentation vérifiable.
- Les numéros `Q-ARCH-` et `Q-TECH-` sont nouveaux : ils évitent toute confusion avec les anciens identifiants **A1/A2/A4** des décisions déjà clôturées figurant dans les chapitres.

## 1. Arbitrages architecturaux encore ouverts

**Aucun arbitrage architectural Q-ARCH ne reste ouvert.** Les décisions Q-ARCH-001 à Q-ARCH-007 ont été validées et intégrées aux chapitres normatifs ; seules les précisions techniques Q-TECH suivantes restent à consolider avant implémentation.

## 2. Précisions techniques à consolider

### Q-TECH-002 — Paramètres lumineux, projections et cas limites restants
**Priorité : P1** — [Lumières et ombres, §7.4](voxel-engine-v0.1.4/08-lumieres-et-ombres.md) · [Scénarios de validation, §8.5](voxel-engine-v0.1.4/10-contrats-et-validation.md)

**Décisions déjà validées — ne pas rouvrir :**
- Point/Spot/Directional : couleur `spk::RGB` linéaire, canaux bornés à `[0,1]`, intensité scalaire `float` finie `>=0` sans plafond artificiel ; `spk::Color` devient `spk::RGBA`, avec `using Color = RGBA;`. Point/Spot ont une portée `range` indépendante de l'intensité, Spot un seul angle, Directional n'a pas de portée et utilise l'orientation de `Transform3D`.
- Point/Spot : atténuation radiale cubique Smoothstep `1-3x²+2x³`, `x=d/range` dans `[0,1]`, nulle à ou au-delà de `range`, ou pour `range=0`. `d_floor`, l'inverse du carré et `E_min` ne sont **plus** applicables. Spot multiplie par le Smoothstep angulaire en cosinus, de l'axe au bord du cône ; Directional contribue `RGB×intensity` sans décroissance de distance.
- Top-N Point/Spot : `score = intensity × range² × priorityWeight / max(distanceCamera,minLightDistance)` après culling ; RGB exclu du score ; les Directional actives restent hors Top-N.
- Projections d'ombres Point/Spot : `near=range×0,001`, `far=range` ; six perspectives 90° pour Point. `range=0` interdit la construction d'une carte ; une projection non représentable déclenche `spk::Exception`, sans clamp caché. Récepteur avant `near` : échantillon non ombré.
- Biais automatique appliqué au **test** (pas à la génération de carte), avec `constantFactor`/`slopeFactor` publics (défaut 1), `max(B_texel,ULP(z)) × (constantFactor + slopeFactor×sinθ)`, dépendant du format, de la résolution, de la normale géométrique et de la projection historique.
- Point PCF : filtre manuel fixe `3×3` (9 taps), `nearest`, noyau aligné UV sur la face centrale à ±1 texel ; chaque tap reprojeté en 3D vers sa face réelle, comparaison, référence et biais historiques propres à cette face ; départage `X > Y > Z` aux égalités, sans mélange des noyaux. Au centre exact `P=L`, visibilité 1 et aucune texture lue ; normalisation robuste des vecteurs non nuls ; tap avant `near` = visible sans accès profondeur.

**Encore à préciser, sans réécrire les choix précédents :** la valeur et la politique extrême de `minLightDistance` pour le classement ; les conversions explicites RGB/RGBA ; les limites/paramètres des projections directionnelles, couverture et résolution de l'unique carte ; résolutions/format depth des Spot/Point/Directional ; la convention d'égalité exacte à `near`/`far`, les valeurs non finies et la précision numérique aux frontières ; les détails de `B_texel` et des saturations numériques ; tests de continuité PCF aux douze arêtes et huit coins ; valeurs par défaut des budgets de shadow map (X/Y/tolérance et hystérésis) non encore choisies. **Aucun CSM, aucune politique Tiled Forward+, aucun changement de taille de filtre implicite.**

**Clôture :** rendre tous les paramètres restants vérifiables dans §7.4 et §8.5 puis retirer Q-TECH-002 du suivi.

### Q-TECH-003 — WBOIT et états OpenGL
**Priorité : P1** — [GPU, §7.8](voxel-engine-v0.1.4/09-pipeline-gpu-transparence.md)

Préciser valeurs de clear, blending individuel des MRT, pondération d'accumulation, formule de revealage et de composition, gestion du `R8`, restauration d'états GL et comportement des cas limites alpha/profondeur. **Une seule passe géométrique translucide**, sans ombres projetées par les surfaces translucides.

### Q-TECH-004 — Précision numérique du VoxelMesher et d'OcclusionCache
**Priorité : P1** — [Meshing, §3](voxel-engine-v0.1.4/03-meshing-occlusion.md)

Préciser tolérances de coplanarité, union/différence de polygones, triangulation déterministe, dégénérescences, interpolation UV, orientations/indices et propriété d'équivalence des résultats avec et sans cache. Prévoir des tests de géométrie reproductibles.

### Q-TECH-005 — Signatures C++23 et adaptation aux interfaces existantes
**Priorité : P1** — [Contrats, §8](voxel-engine-v0.1.4/10-contrats-et-validation.md)

Finaliser signatures de Builders, identifiants de Parts, factories d'assets, API des composants et des vues, `CaptureContext`, abonnements RAII et exceptions. Vérifier l'intégration à `Mesh<T>`, `RenderSnapshot::Builder`, `Texture::Handle`, `GPUResourceCollection`, `Registry` et `VersionedTrait`, sans changer les contrats publics approuvés.

### Q-TECH-006 — Formats CPU/GPU, limites et bindings
**Priorité : P1** — [GPU, §7.5–7.8](voxel-engine-v0.1.4/09-pipeline-gpu-transparence.md)

Décrire les structures d'instances, matériaux et lumières, alignements/tailles SSBO, bindings, limites OpenGL, régénération des buffers par snapshot, allocation des formats `RGBA16F`/`R8` et depth cubemap, tests de capacité et validation de commandes réentrantes multi-surface.

### Q-TECH-007 — Synchronisation, fermeture de surface et durée de vie GPU
**Priorité : P1** — [Vues, §7.1–7.2](voxel-engine-v0.1.4/07-vues-et-passes.md) · [Ombres, §7.3](voxel-engine-v0.1.4/08-lumieres-et-ombres.md)

Définir les verrous de fusion incrémentale des `ShadowResult` par surface, l'ordre de désabonnement `ContractProvider`, la notification de fermeture, le maintien des canaux nécessaires aux snapshots en vol, les sections critiques lors des captures concurrentes, et le contexte GL valide pour `GPUResourceCollection::reclaimReleased()` même en absence de dessin.

## 3. Décisions validées à ne pas rouvrir

Ce rappel sert seulement à **exclure les faux points ouverts** :

- **Éclairage Forward classique**, sans Tiled Forward+, sans listes GPU par tile ni politique de saturation ; sélection globale `Light::Prioritizer` **Top-N commun Point/Spot**, Directional actives hors quota ; sélection immuable commune aux consommateurs.
- **Sous-snapshots par EngineView**, intégralement exécutés dans l'ordre choisi par le jeu ; aucun ordonnanceur global caché ni dépendance automatique entre vues ; resize explicitement piloté par le jeu.
- **Ombres :** producteur `ShadowMap::RenderingSystem`, consommateur `Lighting::System` ; `ShadowMode` de chaque Contributor indépendant de sa hiérarchie, pas de propagation ; aucune shadow map générée par `VoxelRenderingSystem`.
- **PointLight :** budget souple X/Y/tolérance/hystérésis pour retenir des ombres ; budget commun de reconstruction par image et surface, priorité stricte des scores, sans bonus d'attente ; sortie de sélection = éviction immédiate des **caches logiques actifs**, récupération GPU par l'infrastructure générique.
- **Publication d'ombres :** cubemap de six faces atomique, publication incrémentale par lumière, anciens résultats complets conservés sans régression si encore admissibles ; aucune file d'éviction ou recyclage spécialisé ShadowMap.
- **Voxel :** Volume/Model/Material/VoxelPalette immuables après construction, meshing synchrone ; un atlas par mesh/passe par association Model/Palette ; Assembly animée explicitement ; casters d'ombres définis par le jeu et non inférés du système voxel.
- **Transparence :** WBOIT avec une passe géométrique MRT et depth opaque partagé ; les surfaces translucides ne projettent pas d'ombres.

## 4. Procédure de clôture

1. Prioriser une **seule Q-ARCH** significative, exposer les options et obtenir le choix explicite du mainteneur.
2. Inscrire le contrat adopté **dans le ou les chapitres normatifs**, y compris diagrammes, séquences et tests ; ne pas conserver une alternative comme si elle était encore admise.
3. Supprimer ici la question résolue ; ne conserver que les arbitrages et précisions réellement ouverts.
4. Pour chaque `Q-TECH`, ajouter le contrat vérifiable dans sa section avant l'implémentation et signaler toute découverte nécessitant une nouvelle validation architecturale.
