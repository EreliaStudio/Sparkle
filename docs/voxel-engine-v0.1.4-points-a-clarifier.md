# Sparkle V0.1.4 — Questions ouvertes pour finaliser l'architecture

> **Document de travail complémentaire** à la [spécification d'architecture cible](voxel-engine-v0.1.4-specification.md).
>
> **But :** identifier uniquement les décisions et les précisions nécessaires pour transformer la spécification en un contrat de développement réalisable, sans faire choisir au mainteneur chaque détail technique. Ce document n'est **ni un historique de discussion, ni une liste de tâches d'implémentation, ni une nouvelle source de vérité concurrente** : lorsqu'un point est résolu, la règle retenue doit être intégrée dans la spécification principale et le point retiré ou simplifié ici.
>
> **Statut :** questions issues d'une lecture de la spécification comme document autonome et d'une confrontation ciblée avec les API existantes de Sparkle sur la branche `V0.1.4`. Aucun des choix proposés ci-dessous n'est réputé validé sans décision explicite du mainteneur. Aucun code n'est autorisé par ce document.

## 1. Contexte confirmé : ce que la revue ne doit pas remettre en question

La spécification principale établit déjà les éléments suivants, qui ne sont **pas** à rouvrir à l'occasion de cette liste :

- Les Shapes et Definitions immuables, Cells par valeur, Volume dense à table de Definitions, bake synchrone et complet, deux meshes (opaque/translucide), culling par géométrie/opacité des slots, et cache d'occlusion directionnel facultatif.
- Les Materials génériques, les VoxelPalettes immuables, la sélection de sprites par `SpriteSheet`, la limitation à un atlas `albedo` par passe et par couple Model/Palette, et la séparation apparence/géométrie.
- La composition de Model dans un Assembly à parties articulées, et un VoxelRenderingSystem qui collecte les composants et reconstruit les SSBO de chaque snapshot sans cache inter-frame.
- Les factories statiques par type de l'AssetManager, la clé `(T, id, JSON structurel)`, les retours `T&` empruntés, les constructions `T` par valeur, la synchronisation externe, la détection des cycles et la purge explicite `clear()`.
- **Confirmation du mainteneur : Sparkle ne dispose pas encore d'un système d'éclairage ou d'ombres. Il faut créer ces fonctionnalités dans V0.1.4.** La branche actuelle ne contient pas non plus de pipeline WBOIT complet identifié durant la revue. Les briques graphiques disponibles (`Program`, `Framebuffer`, `RenderPass`, `RenderSnapshot`, `Texture`, `ShaderStorageBuffer`, etc.) sont des fondations, **pas** un pipeline 3D complet à réutiliser tel quel.
- L'éclairage, les ombres et la transparence ne doivent pas devenir des services exclusivement voxels : l'objectif reste une bibliothèque Sparkle publique et générique.
- **Le pipeline générique 3D utilise le Forward Rendering en V0.1.4 :** l'éclairage est calculé pendant le dessin des surfaces, pour l'opaque comme pour le translucide avant accumulation WBOIT. Ni rendu Deferred avec G-buffer d'éclairage, ni organisation Forward+ des lumières par tiles/clusters ne sont requis. Les passes auxiliaires de profondeur, shadow maps et WBOIT restent possibles.
- **L'ordonnancement existant est conservé :** `RenderingEngine` et les `RenderSnapshot`/`RenderPass`/`RenderCommand` orchestrent les nouvelles étapes 3D. Si une passe nécessite une initialisation distincte, une paire de passes nommées par convention `renderPassA.init` puis `renderPassA` est possible, avec `init.order < draw.order` **explicitement défini**. Le nom ne crée pas de comportement automatique. Aucun nouvel ordonnanceur ou `SceneRenderingSystem` central n'est exigé.

## 2. Décisions structurantes à arbitrer

### P0 — Architecture du rendu 3D générique (priorité la plus élevée)

**Référence dans la spécification :** sections 1, 7.1 et 7.5.

**Situation.** La cible décrit des meshes voxel opaques et translucides, l'éclairage, les ombres, les calculs de profondeur et la composition WBOIT. Ces capacités doivent être **créées dans V0.1.4**, et non supposées déjà présentes. Les commandes et passes actuelles de Sparkle fournissent de la structure, pas un système de scène 3D illuminée. **Le choix du Forward Rendering est acté :** l'éclairage des surfaces est calculé dans les shaders au dessin, sans passe Deferred/G-buffer ni Forward+ imposé. **Les caméras et lumières sont des `Component3D` attachés aux `Entity3D`, utilisant leurs `Transform3D` et les Registries par Engine. La caméra est sélectionnée explicitement par chaque vue/viewport 3D, sans caméra principale implicite par Engine et avec plusieurs vues indépendantes possibles.** Ces deux règles sont fixées dans la spécification principale et ne doivent plus être rediscutées ici. **Caméras et cibles de rendu sont indépendantes :** la caméra ne possède pas son framebuffer ; une demande de capture associe explicitement Engine, caméra et cible. L'image produite peut ensuite être affichée sans nouveau rendu de la caméra. Une vue 3D dans `EngineWidget` recevra une caméra explicitement fournie par l'application, sans recherche de caméra principale. L'API exacte de binding (p. ex. `setCamera`, nom indicatif), les projections et la gestion des références restent ouvertes. **Les trois catégories de lumières sont fixées pour V0.1.4 : directionnelle, ponctuelle et spot. Seules les directionnelles et les spots pourront projeter des ombres ; les ponctuelles éclaireront sans ombres.** Ces choix sont fixés dans la spécification principale. Les paramètres détaillés des lumières et les réglages de qualité des ombres restent ouverts. **Les ombres sont activées par défaut sur les lumières directionnelles et les spots, avec désactivation et réactivation explicites par lumière.** Les lumières ponctuelles restent sans ombres. **Les ombres directionnelles utilisent une shadow map unique par lumière pour chaque rendu concerné, sans Cascaded Shadow Maps en V0.1.4 ;** son cadrage, sa résolution et les interactions avec plusieurs viewports restent à préciser. Les ombres des spots ne sont pas encore détaillées.

**Questions architecturales :**

1. Quelles sont les **techniques de génération et les politiques de qualité des ombres** directionnelles et spots ? Leur **activation par défaut et la désactivation/réactivation explicites par lumière sont acquises** ; restent les projections/cadrages, la résolution, le biais, le rafraîchissement, la coexistence de plusieurs vues et la technique des spots. Le principe d'**une seule shadow map par directionnelle** reste acquis (sans CSM). Quel est le périmètre initial de l'éclairage des matériaux ?
2. Quelles **projections de caméra** sont disponibles et quelle **API explicite** associe `EngineWidget` à une caméra (p. ex. `setCamera`, signature non validée), sans caméra principale implicite ? Comment vérifier la correspondance Engine/caméra et gérer la disparition ou le changement de ces objets, tout en préservant un `EngineWidget` sans rendu 3D ?
3. Les mécanismes généraux de préparation et d'exécution de `RenderingEngine`/`RenderSnapshot`/`RenderPass` sont **conservés**. Il reste à définir **quels producteurs** préparent les ressources GPU communes d'une vue (cibles, lumières, ombres, WBOIT), **qui les possède**, et quelles commandes spécialisées contribuent aux passes, sans déplacer ces responsabilités dans le `VoxelRenderingSystem`.
4. Un consommateur de rendu 3D **non voxel** doit-il pouvoir utiliser exactement le même éclairage, les mêmes ombres et les mêmes passes dès V0.1.4 ?

**Contrat acquis.** Pas de second ordonnanceur du rendu 3D : les composants et systèmes contribuent aux passes du `RenderSnapshot::Builder` existant et le `RenderingEngine` assure leur collecte. Le **Forward Rendering** est acquis. **Reste à concevoir**, sans ajouter de couche centrale par principe, le propriétaire des cibles et ressources communes, les contributions de préparation, les commandes GPU et les interfaces d'éclairage/ombres. Le voxel participe à ces passes sans en posséder l'ordonnancement.

**Critère de clôture.** La représentation `Component3D` et la sélection explicite d'une caméra par viewport 3D sont acquises. Il reste à définir les projections et comportements d'indisponibilité, les paramètres des trois catégories de lumières et les détails techniques non arbitrés des ombres directionnelles/spots (hors activation par défaut et carte unique pour une directionnelle), le propriétaire de la préparation graphique, ainsi que le parcours des données jusqu'aux shaders. Un schéma doit séparer explicitement infrastructure graphique commune et adaptation voxel.

### P0 — Orchestration des passes, depth, shadow maps et WBOIT

**Référence :** sections 7.1 à 7.5.

**Situation.** `spk::RenderPass` exécute déjà une **séquence de commandes** et `RenderSnapshot::Builder` trie les passes par **`RenderPass::Key::order` numérique**. Leur réutilisation comme mécanisme d'ordonnancement est **validée**. Une phase d'initialisation peut être une passe indépendante (`renderPassA.init`) placée explicitement avant la passe de dessin (`renderPassA`) par ses valeurs d'ordre ; le suffixe `.init` est seulement une convention. Les commandes de render targets, profondeur, blending, culling et composition WBOIT ne sont pas toutes encore disponibles. Le contexte OpenGL initial de Sparkle active notamment le blending alpha classique, insuffisant pour les réglages spécifiques du WBOIT. **Le Forward Rendering est acquis** : pas de G-buffer d'éclairage différé, mais des étapes auxiliaires d'ombres, de profondeur et de composition restent nécessaires.

**Questions architecturales :**

1. Quels **noms, clés et ordres numériques** les contributions du rendu 3D doivent-elles utiliser pour les étapes ombres, opaque, accumulation/révélation WBOIT et composition ? L'ordre est **déjà assuré** par `RenderSnapshot::Builder` et des passes de préparation explicites peuvent précéder les passes de dessin ; il faut seulement définir leur contrat de contribution et leurs éventuelles contraintes de dépendance.
2. La **caméra ne possède pas de `Framebuffer`** et une cible de rendu est fournie indépendamment pour chaque capture. **Qui possède et dimensionne concrètement** les cibles des `EngineWidget`, les captures hors écran, les textures WBOIT et les shadow maps ? Quelle politique de persistance, resize, partage entre vues et conservation par les snapshots, notamment quand la cible est redimensionnée ?
3. Comment les **états graphiques** nécessaires à chaque passe sont-ils activés puis restaurés, sans dépendre des réglages globaux laissés par une commande précédente ?
4. Comment le rendu voxel **partage-t-il les buffers de profondeur et les ombres** avec les autres objets 3D, sans imposer un pipeline particulier aux widgets/UI 2D ?

**Contrat acquis.** Réutiliser exclusivement l'ordonnancement des `RenderPass` existantes. Lorsque la préparation doit être garantie avant les dessins d'une passe, séparer les phases en **deux passes explicitement ordonnées**. Ne pas créer de `SceneRenderingSystem` central ou de nouveau moteur de passes uniquement pour cela. Les commandes voxel conservent uniquement les ressources et l'état de dessin nécessaires à leur propre exécution. **À préciser :** nomenclature des clés, ressources et commandes de préparation GPU, durée de vie des cibles, transitions/restauration d'état et plusieurs viewports.

**Critère de clôture.** Les ordres numériques et les commandes (y compris les éventuelles paires `.init`/dessin) sont identifiés pour les différentes passes. Les responsabilités de préparation, les attachments, la restauration d'état, la synchronisation et la composition sont documentés. L'indépendance caméra/cible et la possibilité de capture puis réutilisation de sa texture sont acquises ; le propriétaire et le cycle de vie des ressources restent à définir. Il est clair que l'ordonnanceur et les `RenderPass` existent déjà, tandis que certaines commandes graphiques doivent encore être développées.

### P1 — Composition d'Assembly dans un autre Assembly

**Référence :** sections 6.1 et 8.1.

**Situation.** La spécification dit qu'une partie peut servir de point d'attache à un `Model` ou à un sous-assemblage, mais l'objet `Part` est surtout décrit comme portant `Model + Palette + transform + parent`.

**Question à arbitrer.** Quand on attache une Assembly B dans une Assembly A, que devient B ?

- **Approche 1 :** les parties de B sont copiées/importées dans la hiérarchie de A ; A garde une seule hiérarchie, sans référence vivante à B.
- **Approche 2 :** une Part de A contient/référence réellement B, avec des règles explicites de propriété, copie, édition, transformation et interdiction des cycles entre Assemblies.

**Conséquences.** Ces modèles diffèrent pour l'identité des PartID, l'animation, le détachement, la copie autonome, la composition récursive des matrices et le partage éventuel de sous-assemblages.

**Direction recommandée, non validée.** Privilégier **une seule hiérarchie de parties** et une importation explicite si cela suffit aux usages prévus ; éviter une deuxième structure récursive sans bénéfice concret.

**Critère de clôture.** Un exemple « personnage → bras → arme » montre l'ajout, l'animation, la copie et le détachement avec leur comportement exact.

### P1 — Forme publique complète des Materials JSON et de leur chargement

**Référence :** sections 4.1 à 4.4 et 5.2.

**Situation.** Les blocs JSON `Texture`/`UV` sont détaillés, mais ils ne décrivent **pas** un Material complet contenant plusieurs **rôles de textures nommés**. La factory générique des assets et le Builder de Material doivent se rencontrer à cette frontière.

**Questions à arbitrer :**

1. Quelle structure JSON représente **l'ensemble des rôles** (`albedo` et rôles applicatifs libres), chacun avec sa texture et sa sélection UV ? Quelles sont les règles de validation des doublons et des rôles manquants ?
2. Comment choisir explicitement entre `Texture` simple et `SpriteSheet`, et à quel niveau `AssetManager::load<T>(id, config)` est-il appelé ? La **configuration JSON de l'asset** doit rester distincte de la **région UV propre au rôle dans le Material**.
3. Quels formats de construction sont exposés publiquement : `Material::Builder` alimenté manuellement, parsing JSON lié à un AssetManager, et éventuelle spécialisation `AssetFactory<Material>` ? Ne pas créer deux chemins divergents vers la même valeur finale.

**Direction recommandée, non validée.** Un dictionnaire explicite de rôles de texture. Chaque entrée fournit la description de l'asset et éventuellement sa région UV ; le résultat compilé dans Material est toujours `Texture::Handle + Texture::Section`.

**Critère de clôture.** Un **JSON complet**, compréhensible sans explication externe, charge un Material contenant au moins deux rôles ; un second exemple sélectionne un sprite par coordonnées. Les mêmes valeurs sont constructibles en C++.

### P1 — Contrat de synchronisation entre simulation, composants et snapshots

**Référence :** sections 6.1, 6.2, 7.1 et 8.1.

**Situation.** L'Assembly est mutable ; les composants empruntent des pointeurs ; le système de rendu construit un snapshot qui ne doit plus lire ces objets ensuite. Sparkle possède plusieurs contextes d'exécution. Le document ne précise pas suffisamment **qui peut modifier/détruire quoi pendant la collecte du snapshot**.

**Questions à arbitrer :**

1. Dans quel contexte/thread s'effectue la **lecture** des composants, des transformés d'Entity et des Assemblies ? Quelles sont les exigences de synchronisation face aux modifications de simulation ?
2. À quel moment les références non propriétaires (`Assembly*`, `Model*`, `VoxelPalette*`) peuvent-elles être détachées ou détruites, sans être observées pendant une collecte ?
3. Quelles données sont copiées ou capturées par le snapshot (matrices, handles, meshes, SSBO), et quelles données restent-elles interdites d'accès depuis les RenderCommand différées ?
4. Une modification en plein build est-elle exclue par un contrat de threading existant, ou un mécanisme explicite est-il nécessaire ?

**Direction recommandée, non validée.** Utiliser les frontières de snapshot déjà présentes dans Sparkle, éviter toute référence vers la hiérarchie mutable dans une commande, et faire porter à l'application un contrat de vie clair pour les pointeurs empruntés.

**Critère de clôture.** Une séquence « update → construction snapshot → rendu » fixe le point à partir duquel les données GPU sont autonomes et les emprunts ne sont plus nécessaires.

## 3. Précisions techniques importantes à mener pendant la préparation de l'implémentation

Ces sujets nécessitent des contrats vérifiables, mais **pas nécessairement de nouvelles décisions du mainteneur** : l'IA de conception peut proposer des détails qui respectent les invariants déjà établis, puis ne soumettre que les conséquences architecturales éventuelles.

### Géométrie robuste et cache d'occlusion

La spécification impose un clipping flottant de polygones plans convexes, une triangulation déterministe des n-gones à UV arbitraires, l'union de la couverture occultante, la suppression des fragments dégénérés et une tolérance `1e-5` configurable. Avant de coder :

- préciser les tests de **coplanarité**, les seuils géométriques dimensionnellement cohérents, la détermination d'une **fermeture complète** et le traitement des frontières quasi confondues ;
- préciser l'interpolation UV/normales quand un fragment traverse plusieurs triangles sources ;
- démontrer que clipping et cache produisent la **même géométrie observable**, et qu'aucun rejet numérique ne ferme artificiellement une face ;
- détailler la forme des **clés directionnelles** du cache et la comparaison des données de Shape, des orientations et des opacités ;
- vérifier que rotation, flip, winding CCW et occlusion frontale donnent des résultats cohérents dans tous les cas.

**Risque si rien n'est précisé :** chaque développeur pourrait choisir des tolérances et une triangulation incompatibles avec le rendu souhaité. Cela justifie un plan d'algorithmes et des exemples géométriques, pas de rouvrir tous les contrats des Shapes.

### Intégration réelle de Mesh, GPUResource et durées de vie

`Model` réutilise `spk::Mesh<TVertex>`, dont les `LayoutBuffer` font partie de l'infrastructure graphique existante. Les données sources `Shape`, `Definition`, `Cell` et `Volume` restent indépendantes d'OpenGL, mais il faut vérifier :

- si un bake lancé hors thread de rendu peut préparer un `spk::Mesh` sans disposer d'un contexte OpenGL courant, la création effective des objets GPU étant différée ;
- comment `DrawVoxelBucketCommand` retient les meshes, `Texture::Handle`, SSBO et autres ressources jusqu'à l'exécution et jusqu'à la fin de leur usage effectif ;
- les bindings de sampler et de SSBO, la structure réelle de `MeshVertex` et le layout, sans inventer une seconde abstraction de buffers ;
- la comparaison des `Texture::Handle` pour établir l'unicité d'atlas par passe : le type actuel fournit `identifier()` et `generation()`, mais pas nécessairement un opérateur `==` public ; retenir une **identité de State**, pas une égalité par contenu d'image.

**Risque si rien n'est précisé :** le rendu fonctionne en test simple mais échoue avec deux snapshots en vol ou des modèles créés sur un thread de travail.

### APIs finales et erreurs

Les principales responsabilités sont établies ; plusieurs noms/types exacts restent délibérément indicatifs. Avant livraison du contrat de développement, finaliser dans une **seule passe d'interface** :

- les méthodes de construction de `Shape::Builder`, de `Definition`, de `Volume::Builder` et de `VoxelPalette::Builder`, avec la garantie forte d'exception et la consommation après succès ;
- la spécialisation `AssetFactory<T>` et ses contraintes C++23, notamment le cas d'un `T` non copiable/non déplaçable et une absence de factory ;
- les noms/types des slots, orientations, `PartID` et transforms locaux d'Assembly ;
- les liaisons des textures nommées du Material et les variantes de construction JSON ;
- les erreurs `spk::Exception` liées à la géométrie, aux associations Model/Palette, aux dépendances circulaires et aux appels invalides.

**Risque si rien n'est précisé :** la cohérence du concept subsiste, mais différentes IA produiraient des APIs publiques divergentes. Il faut obtenir une **interface compilable consolidée**, sans transformer chaque choix de nom mineur en arbitrage architectural.

## 4. Ordre de résolution recommandé

1. **Pipeline graphique 3D générique**, puis **organisation des passes GPU** : ce bloc est le plus dépendant de capacités encore inexistantes dans Sparkle.
2. **Composition des Assemblies** et **frontière de synchronisation des snapshots** : ces décisions affectent la propriété et la validité des données.
3. **JSON Material complet** : stabiliser la forme publique de construction/chargement.
4. **Préparation technique** du meshing, de l'intégration GPU et des signatures finales : produire des propositions et exemples de tests sans multiplier les questions du mainteneur.
5. Reporter les décisions validées dans la **spécification d'architecture cible**, puis retirer les questions correspondantes de ce fichier jusqu'à ce qu'il ne reste aucun **point bloquant**.

Ce document peut rester présent tant que l'architecture est encore discutée, puis être supprimé lorsque la spécification principale devient suffisamment autonome pour guider le développement.
