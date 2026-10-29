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

### Q-TECH-001 — Règles de Top-N et d'hystérésis lumineuse
**Priorité : P1** — [Lumières, §7.4](voxel-engine-v0.1.4/08-lumieres-et-ombres.md)

**Décisions partielles résolues :** chaque nouvelle EngineView initialise son `Light::Prioritizer` avec `N = 8` et `h = 0,10`, paramètres indépendants et configurables par vue. **Top-N complet :** les nouvelles candidates Point/Spot sont examinées par score décroissant ; chacune affronte **la plus faible lumière actuellement retenue** et ne la remplace que si `score(candidate) > score(lowestRetained) * (1 + h)` ; recalculer le minimum après chaque remplacement. **Toujours ouvert dans Q-TECH-001 :** identité de départage stable des ex æquo, remplissage initial et des places libres, cas `N=0`, réduction de N et reconfiguration de `h`, réactions aux changements de score/admissibilité, mutex de cache et notifications Registry/lumières/caméra. **Ne pas rouvrir** le Forward classique, le Top-N commun Point/Spot, les Directional hors quota ou les budgets d'ombres ponctuelles distincts.

### Q-TECH-002 — Constantes d'éclairage et projections d'ombres
**Priorité : P1** — [Lumières, §7.4](voxel-engine-v0.1.4/08-lumieres-et-ombres.md)

Fixer unités d'intensité et de distance, loi d'atténuation, `minLightDistance`, seuil d'influence, valeurs par défaut des budgets et taux, projections Spot/Directional/Point, domaine de couverture de l'unique carte directionnelle, résolution et qualité de filtrage/biais. **Pas de CSM ni de nouveau type de lumière implicite.**

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
