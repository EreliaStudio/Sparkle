# Sparkle V0.1.4 — Points restant à clarifier

> **Document de travail — uniquement les points non résolus.**
>
> La [spécification d'architecture](voxel-engine-v0.1.4-specification.md) est la source de vérité pour toutes les décisions déjà validées. Le présent document **ne les répète pas**, ne les remet pas en discussion et n'autorise aucune modification du code C++.
>
> Une question est supprimée de cette liste dès que sa résolution est intégrée à la spécification. Les signatures présentées comme indicatives dans la spécification ne constituent pas, à elles seules, de nouveaux arbitrages à soumettre au mainteneur. Les points ci-dessous sont regroupés par responsabilité et non par ordre historique des discussions.

## 1. Arbitrages architecturaux restant à valider

Ces sujets peuvent modifier le contrat public ou le comportement observable de Sparkle. Ne soumettre **qu'une question importante à la fois** au mainteneur, avec des solutions compatibles avec la spécification existante.

### A1 — Source du mesh et valeurs initiales de \`ShadowMap::Contributor\`

**Références :** spécification §7.2–7.3.

Préciser le contrat exact du \`MeshFilter\` intégré :

- Un \`ShadowMap::Contributor\` doit-il obligatoirement recevoir une géométrie d'ombre explicite, ou peut-il utiliser par défaut un mesh visible existant ? Si un fallback existe, quelle source fait foi lorsqu'une Entity contient plusieurs éléments visuels ?
- Que se passe-t-il si aucun mesh de projection exploitable n'est disponible : source ignorée, état invalide ou exception, et à quel moment ?
- Quelle propriété ou quel emprunt le \`MeshFilter\` conserve-t-il sur la géométrie ? Comment une modification, substitution ou disparition de cette source est-elle signalée ?
- Quelles sont les valeurs initiales de \`ShadowMode\`, \`ShadowPropagation\` et des layers propres à ce composant ? Le caractère facultatif de sa présence sur une Entity doit être explicite.

**À préserver :** les sources d'ombres restent distinctes des composants visuels et du \`VoxelRenderingSystem\`; aucune création implicite d'un caster ne doit être présumée sans décision.

### A2 — Propagation des modes d'ombres et conflits de chemins

**Références :** spécification §7.2–7.3.

- Définir précisément quelles relations de **collecte graphique explicitement déclarées** complètent la parenté \`InherenceTrait\` des Entities et comment elles sont représentées.
- Lorsque plusieurs chemins atteignent le même \`ShadowMap::Contributor\`, définir la priorité des overrides \`Descendants\` concurrents, notamment s'ils portent des modes différents. Préciser le comportement en cas de reparenting et de retrait d'un chemin.
- Garantir le calcul déterministe d'un mode effectif unique sans modifier les valeurs enregistrées et sans dessiner deux fois la même source.

Ne pas confondre inscription dans le Registry et parenté, ni réintroduire de logique d'ombres dans \`EngineView\`.

### A3 — Mise à disposition des cartes d'ombres finales à l'éclairage

**Références :** spécification §7.3–7.4 et §7.8.

Déterminer le **contrat de publication/consommation** des résultats de \`ShadowMap::RenderingSystem\` :

- Par quelle interface générique les commandes Forward+ retrouvent-elles la carte finale, ses matrices et son état de validité pour une lumière et une vue données ?
- Où sont capturées les références logiques et les générations, et comment leur réalisation est-elle résolue pour la \`Window::Surface\` courante ?
- Comment signaler au shader une lumière dont les ombres ne sont pas disponibles ou sont différées, sans échantillonner de carte périmée ?
- Comment éviter toute dépendance de \`EngineView\` envers \`ShadowMap\`, tout en conservant l'ordre de production puis de consommation au sein du sous-snapshot ?

Ne pas confondre cette publication de ressources d'ombres avec le **culling Forward+ des lumières**, qui est indépendant.

### A4 — Découpage GPU de l'éclairage Forward+

**Référence :** spécification §7.8.

Il reste à choisir **tiled Forward+ ou clustered Forward**, puis à préciser les règles fonctionnelles qui en dépendent :

- Dimensions des tiles ou clusters et domaine de profondeur considéré.
- Sélection pertinente des PointLight et SpotLight, y compris pour les fragments translucides que la profondeur opaque ne couvre pas.
- Représentation des listes GPU et traitement explicite de leur dépassement de capacité, sans disparition silencieuse d'une lumière.
- Position des calculs et barrières GPU dans le sous-snapshot de chaque vue.

Le choix Forward+ lui-même, l'absence de G-buffer et le shading Forward de WBOIT sont clos.

### A5 — Textures de vue dans les \`Material\` et compatibilité des atlas voxel

**Références :** spécification §4.1, §4.4, §7.1 et §7.7.

- Définir l'interface publique minimale permettant à un Material immuable de référencer **sans propriété** une \`EngineView\`, puis au snapshot de capturer la source logique correspondant à l'état de cette vue.
- Définir l'identité d'une texture issue d'une vue pour contrôler la contrainte d'**un atlas \`albedo\` par passe et par association Model/Palette**, y compris avant première production et après changement explicite de résolution.
- Décider si une sortie de vue peut être utilisée comme \`albedo\` d'un Material voxel et, dans ce cas, définir les erreurs de compatibilité et les éventuels cycles de consommation entre vues.

Les politiques de sortie d'une vue détachée, la validation CPU puis GPU, la réalisation par surface et l'interdiction de relire la vue mutable pendant \`execute()\` sont déjà spécifiées : ne pas les réarbitrer.

### A6 — Composition d'une \`Voxel::Assembly\` dans une autre

**Référence :** spécification §6.1.

Choisir le comportement d'une Assembly B attachée à une Assembly A :

- **Importation de parties :** copie des parties de B dans l'unique hiérarchie de A, sans relation vivante avec B.
- **Sous-assembly référencée ou possédée :** B reste une structure distincte sous une Part de A, avec des contrats explicites de propriété, de copie, de mutation et de cycles.

L'arbitrage devra fixer les conséquences pour les identifiants de parties, les transformations composées, l'animation, le détachement et la copie autonome.

### A7 — Structure publique du Material JSON complet

**Références :** spécification §4.1–4.3 et §5.2.

- Définir le document JSON complet d'un Material comportant **plusieurs rôles nommés**, et pas seulement la description JSON d'une texture ou de ses UV.
- Séparer clairement l'**identité et la configuration d'un asset** de la **sélection UV propre au rôle** dans le Material.
- Définir l'articulation entre \`Material::Builder\`, sa liaison optionnelle à \`AssetManager\`, le parsing JSON et, si elle est retenue, une \`AssetFactory<Material>\` ; tous les chemins doivent produire la même valeur Material.
- Fixer la validation des rôles dupliqués, des sources de textures et des champs incompatibles.

Ces règles devront être illustrées par un Material JSON complet à au moins deux rôles et par son équivalent construit manuellement.

## 2. Contrats techniques à préciser avant implémentation

Ces points ne nécessitent **pas automatiquement** une question au mainteneur. Les régler au niveau technique lorsque les décisions déjà validées suffisent ; ne demander un arbitrage que si plusieurs comportements publics substantiellement différents demeurent possibles.

### T1 — \`RenderPass::Key\`, phases et frontière moteur/UI

**Références :** spécification §7.1–7.3.

- Fixer le tableau des passes standard de chaque vue, leurs noms, clés numériques, réservations, bornes pour passes personnalisées et règles de collision.
- Encoder la séquence des vues explicitement ordonnées puis non ordonnées, sans sentinelle pouvant entrer en collision à la borne numérique maximale.
- Assurer dans le snapshot de fenêtre la position des commandes de retour à la présentation **immédiatement après** chaque délégation au snapshot moteur et **avant** le dessin UI.
- Préciser si la passe d'ombres unique est absente ou vide lorsqu'aucune lumière ne la nécessite ; déterminer la déclaration de sa clé commune.
- Fixer l'API de retour du filtre des layers d'une EngineView à l'état « non configuré », sans assimiler cet état à \`setLayers({})\`.

### T2 — Versions, invalidations et sûreté des snapshots

**Références :** spécification §7.1–7.4.

- Définir le suivi des générations de cartes statiques/finales et le suivi d'une image effectivement produite, **par version logique de cible et par surface**.
- Détailler la collecte et l'invalidation lors des changements de \`MeshFilter\`, activation, layers, transformation, parenté, lumière, projection et membership de la vue. Aucune invalidation ne doit être perdue pendant une capture ou le détachement d'une caméra.
- Vérifier que tout remplacement de cible après resize crée les **nouveaux States** nécessaires, sans modifier un attachment capturé dans un ancien snapshot.
- Décrire comment les demandes de reconstruction de PointLight de **toutes les vues dessinées dans une même image sur une surface** sont réunies et arbitrées **avant** l'exécution des reconstructions ; préserver le score brut décroissant, le budget commun et l'ordre des vues. Définir le compteur d'image et le traitement d'une éventuelle réexécution du même sous-snapshot.
- Conserver l'immutabilité et la réentrance des commandes : aucun accès à une Entity ou à un Contributor mutable durant l'exécution GPU.

**Contrat existant à exploiter, pas à redéfinir :** \`GPUResource::Handle\`, \`State::LifeTime\`, \`GPUResourceCollection::ReclamationQueue\` et le recyclage natif des ressources. Vérifier uniquement que \`RenderRuntime::tickOnce()\` rend le **bon contexte OpenGL courant** avant \`reclaimReleased()\`, y compris sans nouveau rendu et avec plusieurs surfaces. Ne pas ajouter de file de destruction ShadowMap ni de \`GLsync\` spécifique sans nécessité démontrée.

### T3 — Paramètres lumineux et qualité des ombres

**Références :** spécification §7.3–7.4.

- Finaliser les paramètres de caméra et des lumières (portée, atténuation, angle des spots, intensité, espaces de coordonnées et validations).
- Fixer la région couverte, les matrices et les plans near/far d'une shadow map directionnelle **sans cascades**, ainsi que la projection des spots.
- Fixer les valeurs/configurations de résolution, biais, comparaison, filtrage/PCF, profondeur stockée et échantillonnage, notamment aux coutures des cubemaps.
- Déterminer les valeurs initiales de \`targetShadowCount\`, \`maxShadowCount\`, \`shadowPriorityTolerance\`, \`shadowSelectionHysteresis\`, \`minShadowDistance\` et \`maxPointShadowRebuildsPerImage\`, ainsi que le périmètre public de leur configuration.
- Définir les erreurs et les diagnostics exposés par les configurations invalides et l'indisponibilité d'une carte.

**Ne pas rouvrir :** le score de priorité, la précédence tolérance/hystérésis, les deux budgets distincts, l'éviction immédiate des caches PointLight sorties de sélection, la reconstruction atomique des six faces et la poursuite de l'éclairage sans ombre.

### T4 — Pipeline WBOIT et états graphiques

**Référence :** spécification §7.2 et §7.8.

- Fixer les valeurs de clear des attachments d'accumulation et de révélation.
- Fixer la fonction de poids, les facteurs/équations de blending indépendants, la comparaison de profondeur exacte et la formule de composition.
- Documenter les préconditions OpenGL et leur validation, la complétude des framebuffers, le binding du depth attachment commun et les transitions explicites d'état entre les passes.
- Vérifier les limites numériques et les tests de précision du buffer de révélation.

Les formats \`RGBA16F\` et \`R8\`, la passe géométrique MRT unique, la composition séparée et le depth partagé avec écriture désactivée pendant WBOIT sont déjà décidés.

### T5 — Concurrence, input et commandes GPU réentrantes

**Références :** spécification §6 et §7.1–7.2.

- Formaliser la frontière de synchronisation entre mutations des composants/Assemblies/vues/lumières et \`refreshRenderSnapshot()\`, ainsi que la durée de vie des objets empruntés pendant la capture.
- Décrire la cohérence attendue de deux captures demandées à des instants différents par deux fenêtres, sans introduire de déduplication automatique.
- Préserver la propagation existante des événements et du \`DeviceContext\` en passant du \`RenderingEngine\` adaptateur à \`RenderingEngine : Engine\`, notamment lorsque plusieurs widgets possèdent des rôles distincts.
- Auditer les \`RenderCommand\` existantes, les UBO et les caches CPU mutables (notamment le viewport), puis adapter le \`Profiler\` à l'exécution imbriquée de sous-snapshots.
- Vérifier la disponibilité d'un contexte GL lors des opérations natives de préparation et de récupération des ressources.

### T6 — Meshing voxel et cache d'occlusion : précision numérique

**Références :** spécification §2–3.

- Spécifier les tests de coplanarité, d'union/différence polygonale, de fermeture complète et de dégénérescence avec des unités/tolérances cohérentes.
- Définir une triangulation déterministe pour les polygones convexes à UV arbitraires, avec interpolation correcte après clipping.
- Fixer l'encodage des clés directionnelles d'\`OcclusionCache\` et démontrer que les chemins avec et sans cache donnent les mêmes fragments visibles.
- Couvrir par tests rotations, flip/winding, \`innerShell\`, cellules extérieures \`Empty/Occupied/Unknown\` et recouvrements partiels.

Le modèle géométrique, le bake complet et la règle de culling des surfaces sont déjà fixés.

### T7 — Adaptation aux API C++23 et GPU réellement disponibles

**Références :** spécification §2–8.

Consolider dans une **seule revue d'interfaces publiques** :

- Les Builders de Shape, Definition, Volume, Model/mesher, Material et Palette ; les bindings, le type des orientations, des identifiants de Part et des transformations locales.
- La forme exacte de \`AssetFactory<T>\` et la construction en emplacement final d'un T non copiable/non déplaçable.
- L'intégration de \`spk::Mesh<TVertex>\`, des formats de vertex, des handles de texture, des SSBO et des bindings de shaders. Confirmer qu'un bake hors thread GPU ne nécessite pas un contexte OpenGL actif.
- Les commandes génériques de changement d'état, de framebuffer et d'attachement **explicite d'une face de cubemap**, sans sous-système GPU supplémentaire propre aux ombres.
- Les règles de propriété des sources de \`ModelRenderingComponent\` et \`AssemblyRenderingComponent\`, leur inscription au Registry et les notifications nécessaires.

Les noms secondaires et les détails de signature ne doivent pas susciter une nouvelle question au mainteneur s'ils découlent mécaniquement du contrat fixé.

## 3. Conditions de clôture du document

Un point peut être retiré lorsqu'il a été :

1. **Résolu et intégré à la spécification**, avec mise à jour des diagrammes, exemples, séquences et critères de test concernés ; ou
2. **Explicitement reporté hors V0.1.4**, avec une frontière de fonctionnalité claire approuvée par le mainteneur.

Avant d'autoriser l'implémentation, une revue finale doit vérifier que les interfaces importantes sont réalisables, que les passages entre composants ne reposent pas sur des décisions implicites et qu'il existe des tests pour les contrats observables. Les scénarios de test des décisions **déjà validées** figurent dans la spécification et les futurs tests d'implémentation : ils n'ont pas à être répétés ici.

**Priorité de travail suggérée :** traiter d'abord les questions d'intégration transversale (A1–A5), puis les contrats d'Assembly et de Material JSON (A6–A7) ; consolider T1–T7 en parallèle de la préparation de l'interface sans poser systématiquement de nouvelles questions.

Ce document ne crée aucune nouvelle décision et ne modifie pas les choix approuvés dans la spécification.
