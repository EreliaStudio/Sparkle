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

- **Vues 3D nommées dans `sparkle::core` :** l'Engine possède des `spk::EngineView`, identifiées par chaîne, chacune associée à une caméra explicite de cet Engine et à une résolution choisie par le jeu. `EngineView` et `CameraComponent` ne possèdent aucun `Framebuffer` ni ressource OpenGL. `"main"` est seulement le `ViewID` sélectionné par défaut par `EngineWidget` ; le jeu doit enregistrer la vue et la caméra. `EngineWidget` **met à jour, rend toutes les vues de l'Engine et affiche la vue sélectionnée** ; `EngineViewWidget` **affiche seulement** une vue nommée, sans update, capture ou nouvel ordonnanceur.
- **Cache graphique partagé par fenêtre :** dans `sparkle::sparkle`, les cibles persistantes sont indexées par `(Window::Surface, Engine, ViewID)`. **Un `EngineWidget` produit chaque vue une fois dans sa propre contribution** au snapshot ; les widgets d'affichage peuvent réutiliser cette texture sans nouveau dessin 3D. Si le développeur crée deux `EngineWidget` sur un même Engine, Sparkle n'empêche et ne déduplique **ni leurs mises à jour ni leurs captures** : leur bon usage relève du développeur.
- **Aucun resize implicite :** seul le code du jeu décide de modifier la taille d'une `EngineView` et, indépendamment, les paramètres de projection de sa caméra. Le resize d'une fenêtre ou d'un widget ne modifie jamais ces valeurs ; le cache graphique ajuste ses ressources uniquement après changement explicite de la résolution de l'EngineView. Un widget peut afficher une texture à une autre taille sans redimensionner la capture.

## 2. Décisions structurantes à arbitrer

### P0 — Intégration des EngineView dans les snapshots de fenêtre

**Référence :** sections 7, 7.1, 7.4 et 8.1 de la spécification.

**Constat de revue de zéro.** `EngineView` est une description logique dans `sparkle::core`, alors que `RenderingEngine`, `Window::Surface` et `Framebuffer` appartiennent à `sparkle::sparkle`. Aujourd'hui, `Application::UpdateRuntime::_buildRenderSnapshot` fabrique un `RenderSnapshot::Builder` **sans lui passer l'identité de la fenêtre** et appelle seulement `state.root().buildRenderSnapshot(builder)`. Le `RenderingEngine` actuel est membre d'`EngineWidget` et ne connaît qu'un Engine, sans contexte de vue transmis aux contributeurs. La cible par `(Window::Surface, Engine, ViewID)` est validée mais sa résolution au moment du build **n'est pas implémentée**.

**Questions bloquantes :**

1. Comment fournir **l'identité de fenêtre/surface** au code qui construit les snapshots, pour obtenir le bon cache **sans appels OpenGL dans le thread update** ni nouveau scheduler ? Quelle durée de vie du cache quand une fenêtre, un Engine ou une EngineView disparaît ?
2. Comment faire contribuer `RenderingEngine`, `VoxelRenderingSystem` et les autres contributeurs **pour chaque EngineView** en leur transmettant la caméra, les matrices, la cible et les données de lumières de la vue, puisque `buildRenderSnapshot(Builder&)` seul ne transporte pas ce contexte aujourd'hui ?
3. Quels contrats de lecture et de propriété permettent d'enregistrer, supprimer, rebinder ou renommer une EngineView et de remplacer sa caméra sans références pendantes ? Qui valide l'appartenance de la caméra à l'Engine, la résolution (dimensions nulles, limites GPU), et que faire si la vue `"main"` ou la texture recherchée est absente ?
4. À quel instant et sous quel ordre de passes les **handles d'une nouvelle cible** sont-ils fournis aux commandes de production et aux widgets consommateurs, notamment si la résolution de la vue change entre deux snapshots encore en vol ?

**Contrat déjà acquis.** `Engine` possède les vues logiques, sans OpenGL ; un cache par fenêtre possède les ressources persistantes ; le jeu contrôle **seul** la résolution et les projections. `EngineWidget` effectue update + captures de toutes les vues + présentation d'une vue. `EngineViewWidget` ne fait que présenter une texture déjà produite. Les `RenderPass` et `RenderSnapshot` restent les seuls mécanismes d'ordonnancement des commandes.

**Critère de clôture.** Un exemple réel de construction de snapshot dans une fenêtre montre comment accéder à la même ressource de vue depuis le producteur et les consommateurs, sans dépendance interdite de `sparkle::core` vers `sparkle::sparkle`.

### P0 — Contrat du pipeline Forward, des ombres et du WBOIT

**Référence :** sections 7.1 à 7.5.

**Constat de revue de zéro.** Les primitives `RenderPass`, `RenderSnapshot`, `Framebuffer` et `GPUResource::Handle` existent. Les passes sont triées par ordre numérique ; les phases `.init` sont des passes **explicitement ordonnées**, pas un hook magique. En revanche, la configuration des render targets, la gestion de l'état OpenGL par passe, le WBOIT complet, le rendu des lumières et les shadow maps ne sont **pas** fournis comme pipeline générique complet aujourd'hui. Les formats couleur flottants nécessaires au WBOIT ne sont pas tous exposés par `Texture::Format`.

**Questions bloquantes :**

1. Quels **ordres et identifiants de passes** permettent, pour plusieurs vues du même snapshot, le chaînage **ombres → opaque → accumulation/révélation WBOIT → composition de la texture finale → présentation des widgets**, sans collisions entre `ViewID` et passes de l'UI ? Comment remettre explicitement la cible et l'état graphique adéquats avant chaque phase ?
2. Quels formats GPU, attachments, règles de profondeur partagée/lecture-écriture, blend states et opérations de composition exacts implémentent le WBOIT ? Comment restaurer un état valide pour les widgets 2D et les autres objets 3D après une capture ?
3. Quels sont les contrats du **Forward Lighting** générique pour voxel **et non voxel** (paramètres, contribution des trois lumières, limites), et des **shadow maps** directionnelles/spots (projection, qualité, biais, résolution, coût, rafraîchissement et partage éventuel entre vues) ? Le rendu Deferred/Forward+ n'est pas requis, les lights ponctuelles ne projettent pas d'ombres, une seule shadow map par directionnelle et vue est retenue.
4. Comment conserver **la validité des anciens snapshots** lorsqu'un framebuffer ou ses textures sont remplacés après un resize **explicitement demandé par le jeu**, sans mutation rétroactive, accès inter-thread non sécurisé ou lecture de données `CameraComponent` / `Assembly` mutables pendant les commandes GPU ?

**Contrat déjà acquis.** Les `EngineView` sont enregistrées explicitement par le jeu ; une vue est produite depuis son `CameraComponent` dans le cache propre à la surface puis sa texture peut être présentée librement. L'ordre `RenderPass::Key::order` décide de l'exécution ; la présence du suffixe `.init` ne décide rien. Le jeu, **pas Sparkle**, choisit comment la taille/projection change lors d'un resize.

**Critère de clôture.** Un diagramme de passes exécutable et une séquence de captures puis présentation, avec deux vues de résolutions différentes et un snapshot antérieur toujours en vol, ne nécessitent ni comportement GPU implicite ni nouveau framework de passes.

### P0 — Limites voulues des widgets (pas de protection multi-EngineWidget)

**Référence :** sections « EngineView » et 7.1.

**Contrat acquis, pas une question à arbitrer.** `EngineWidget` met à jour l'Engine, déclenche les captures de ses vues et affiche l'une d'elles. `EngineViewWidget` n'effectue **que** l'affichage, sans simulation ni capture, et peut réutiliser une texture produite pour la même fenêtre. Enregistrer deux `EngineWidget` pour **le même Engine** peut provoquer plusieurs updates et plusieurs rendus : **Sparkle ne les bloque pas, ne les déduplique pas et n'a pas à les protéger**. L'application assume ce choix. Les ressources partagées ne constituent pas un verrou de déduplication de la production. Il reste seulement à finaliser le comportement documenté d'une texture absente et la présentation d'une texture à une taille d'affichage différente de sa résolution de capture.

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

**Situation.** L'Assembly est mutable ; les composants empruntent des pointeurs ; le système de rendu construit un snapshot qui ne doit plus lire ces objets ensuite. Sparkle possède des threads update et render distincts, et désormais des descriptions d'EngineView modifiables par le jeu. Le document ne précise pas suffisamment **qui peut modifier/détruire quoi pendant la collecte du snapshot**.

**Questions à arbitrer :**

1. Dans quel contexte/thread s'effectue la **lecture** des composants, des transforms d'Entity, des Assemblies, des CameraComponent et des EngineView ? Quelles sont les exigences de synchronisation face aux modifications de simulation ?
2. À quel moment les références non propriétaires (`Assembly*`, `Model*`, `VoxelPalette*`) peuvent-elles être détachées ou détruites, sans être observées pendant une collecte ?
3. Quelles données sont copiées ou capturées par le snapshot (matrices de vue/projection, lumière, handles de framebuffer et textures, meshes, SSBO), et quelles données mutables restent-elles interdites d'accès depuis les RenderCommand différées ?
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

1. **Résolution du cache de vues par fenêtre dans les snapshots**, puis **pipeline Forward/ombres/WBOIT et ordre des passes GPU** : ce sont les seuls points de raccordement majeurs à l'architecture graphique existante.
2. **Composition des Assemblies** et **frontière de synchronisation des snapshots** : ces décisions affectent la propriété et la validité des données.
3. **JSON Material complet** : stabiliser la forme publique de construction/chargement.
4. **Préparation technique** du meshing, de l'intégration GPU et des signatures finales : produire des propositions et exemples de tests sans multiplier les questions du mainteneur.
5. Reporter les décisions validées dans la **spécification d'architecture cible**, puis retirer les questions correspondantes de ce fichier jusqu'à ce qu'il ne reste aucun **point bloquant**.

Ce document peut rester présent tant que l'architecture est encore discutée, puis être supprimé lorsque la spécification principale devient suffisamment autonome pour guider le développement.
