# PR-27 (provisoire) — Model3D::Animation et Model3D::Animator

> **Fiche d'architecture collaborative, non validée pour implémentation.**
>
> **Branche documentaire :** `docs/v0.1.4/pr27-model3d-animation-design`, créée depuis `V0.1.4`.
> **Base technique examinée :** `V0.1.4` au 13 novembre 2026.
> **Statut :** décisions de la discussion du 13 novembre consolidées ; questions d'architecture encore ouvertes.
> **Aucune modification de code ni fusion demandée à ce stade.**

## 1. Objectif

Ajouter des animations déterministes et réutilisables à `spk::Model3D`, sans dépendre d'OpenGL ni déplacer automatiquement la transformation ECS de l'entité.

Cas d'usage prioritaires :
- faire marcher un personnage en animant ses articulations ;
- le faire saluer par une rotation de bras ;
- réaliser un squat en abaissant des éléments du modèle et en pliant les jambes, **sans modifier la position de l'entité** ;
- modifier localement les proportions/tailles d'éléments ;
- conserver le fonctionnement des objets attachés (arme en main, sous-`ObjectHeadless`) et des rendus existants.

L'animation doit pouvoir s'exécuter dans le Core/headless. Un `Object<TGraphics>` récupère naturellement les transformations finales de son `ObjectHeadless`.

## 2. Décisions acquises dans la discussion

| Sujet | Décision |
|---|---|
| Objets publics | Deux concepts supplémentaires : `spk::Model3D::Animation` (données) et `spk::Model3D::Animator` (lecture). Ne pas créer un framework d'animation parallèle. |
| Intégration au moteur | `Animator` est un `spk::Behaviour` ; il est mis à jour via le cycle normal du moteur et son `UpdateContext`. |
| Domaine des animations | Les modifications portent sur l'état **interne** de `ObjectHeadless`, pas sur la transformation ECS de l'entité. |
| Format de clip | Une `Animation` est une suite ordonnée de poses/états. Chaque passage d'une pose vers la suivante possède une durée en **millisecondes** ; durée totale = somme des durées des transitions. |
| Poses | **Partielles** : chaque pose décrit seulement les propriétés qu'elle anime, et non l'intégralité du modèle. |
| Articulations | `AttachmentNode` gère la **rotation dynamique** uniquement (conserve ses points d'attachement définis par le `Blueprint`). |
| Éléments | `ObjectHeadless::ElementNode` reçoit une **translation** et une **échelle** dynamiques dans son repère local ; **pas de rotation dynamique propre à l'élément**. |
| Interpolation | Rotations par interpolation de quaternions (`slerp`) ; translations et échelles par `lerp`. Ne jamais interpoler directement les 16 coefficients d'une matrice. |
| Définition statique | Le `Blueprint` et ses matrices d'ancrage restent immuables. |
| Pas de GPU requis | Le calcul temporel et les changements de pose doivent fonctionner indépendamment des vues graphiques. |

**Correction de la proposition initiale :** l'idée précédente d'un TRS complet (avec rotation) sur chaque `ElementNode`, et l'idée de translations dynamiques sur `AttachmentNode`, sont **remplacées** par la séparation finale : **rotation sur articulation ; translation + échelle sur élément**.

## 3. Architecture existante, vérifiée dans `V0.1.4`

- `Model3D::Blueprint` décrit les éléments, leurs matrices d'ancrage et leurs `AttachmentPoint` statiques.
- `ObjectHeadless` fournit `root()`, `element(name)`, `attachment(elementName, attachmentName)`, les copies/déplacements et l'attachement/détachement de sous-objets.
- `AttachmentNode` expose déjà `setRotation(Quaternion | Vector3)`, `rotate(...)`, `rotationMatrix()` et `transform()`.
- `ElementNode::_localTransform()` renvoie aujourd'hui uniquement `definition().anchor()` : pas de translation ni échelle runtime propres à l'élément.
- `Attachable::globalTransform()` est calculée à la demande et mise en cache ; les changements d'articulation invalident les descendants concernés.
- `spk::Behaviour` hérite d'`UpdatableTrait` ; la surcharge d'extension est `_updateState(spk::UpdateContext&)`. `UpdateContext` donne `time` et `deltaTime` sous forme de `std::chrono::steady_clock::duration`.
- `Model3D::Object<TGraphics>` construit les snapshots graphiques depuis les matrices globales des éléments ; il ne doit pas être requis pour animer le Core.

Important : les fichiers d'historique `PR-08.2`/`08.3` portant les anciens noms `Assembly`/`Puppet` ne constituent pas l'API actuelle.

## 4. Transformations locales : contrat de travail

Convention actuelle : matrices avec vecteurs colonnes, composition parent × enfant.

### Articulation

```text
M_attachment = M_attachmentBlueprint × R_dynamic
```

- `R_dynamic` est portée par `AttachmentNode`.
- Le pivot et l'orientation de repos viennent du `Blueprint`.
- Aucune translation ou échelle *dynamique propre à l'articulation* n'est introduite.

### Élément

```text
M_elementLocal = M_anchorBlueprint × T_dynamic × S_dynamic
```

- La translation `T_dynamic` est exprimée selon les axes du repère d'ancrage local de l'élément.
- L'échelle `S_dynamic` est elle aussi locale.
- Pas de rotation dynamique sur `ElementNode` : celle-ci est assurée par l'articulation parente.
- Valeurs neutres : translation `(0,0,0)`, échelle `(1,1,1)`.
- Conserver les composantes numériques séparées et une matrice locale dérivée/cachée ; une matrice 4×4 peut être exposée pour la consommation, sans être le stockage unique de l'état interpolable.
- Recalculer les caches uniquement lorsque les valeurs changent ; invalider les transformations globales pertinentes et leurs descendants.

La matrice mondiale/relative d'un élément reste la composition de sa hiérarchie existante, avec les articulations intermédiaires et sa transformation locale.

**Point encore à arbitrer :** le redimensionnement d'un élément propage-t-il son échelle aux points d'attachement et descendants ? C'est le comportement naturel si `S_dynamic` est dans la chaîne hiérarchique. Nous l'avons proposé, pas encore explicitement validé.

## 5. `spk::Model3D::Animation` — données, sans horloge

Une animation définit :
- une succession ordonnée de poses partielles ;
- les durées en millisecondes des transitions entre poses consécutives ;
- des **rotations d'articulations**, adressées sans ambiguïté par le couple `(nomElement, nomArticulation)` ;
- des **translations d'éléments**, adressées par leur nom ;
- des **échelles d'éléments**, adressées par leur nom.

La présence d'une propriété est indépendante des autres : animer l'échelle de `torso` n'impose pas une translation de `torso` ni une rotation de `shoulder`.

Exemple **conceptuel** (pas une signature C++ arrêtée) :

```text
Animation "squat"
  Pose 0:
    element("torso").translation = (0, 0, 0)
    attachment("upperLegL", "knee").rotation = neutral
    attachment("upperLegR", "knee").rotation = neutral
  transition 0 -> 1: 250 ms
  Pose 1:
    element("torso").translation = (0, -0.35, 0)
    attachment("upperLegL", "knee").rotation = bent
    attachment("upperLegR", "knee").rotation = bent
  transition 1 -> 2: 250 ms
  Pose 2:
    element("torso").translation = (0, 0, 0)
    attachment("upperLegL", "knee").rotation = neutral
    attachment("upperLegR", "knee").rotation = neutral

  total = 500 ms
```

Les noms et valeurs de cet exemple sont illustratifs, pas des contraintes sur le `Blueprint`.

**À préciser avant développement :** validation des durées nulles/négatives, représentation de la première/dernière pose et comportement d'une animation à une seule pose. La durée ne doit pas être confondue avec un nombre de frames.

## 6. `spk::Model3D::Animator` — `spk::Behaviour` responsable de la lecture

Responsabilités prévues :
- associer une ou plusieurs définitions `Animation` à un `ObjectHeadless` cible ;
- faire progresser un temps de lecture à partir d'`UpdateContext` ;
- identifier la transition active et sa progression, y compris lorsque `deltaTime` traverse plusieurs transitions ;
- évaluer les valeurs interpolées et les appliquer aux `AttachmentNode` et `ElementNode` ;
- conserver le découplage Core / Graphics et la gestion habituelle de l'activité du `Behaviour`.

Le résultat d'une animation à un instant donné doit dépendre du **temps de lecture**, pas du nombre d'appels `updateState`. Deux découpages temporels qui aboutissent au même instant doivent produire la même pose, à politique de lecture égale.

`Animator` ne modifie pas silencieusement la position de son `Entity3D` : le déplacement du personnage dans la scène relève d'un autre comportement.

**APIs à concevoir ensemble**, et non encore approuvées : association au modèle, ajout/sélection d'animations, `play`/`pause`/`stop`, réglage/recherche du temps, vitesse, boucle, reprise et politique de fin de lecture. Éviter les options implicites lorsqu'elles modifient le résultat.

## 7. Sémantique des poses partielles — à définir

**Validé :** seuls les champs explicitement ciblés par une pose sont animés.

**Non tranché :** si un canal est présent dans la pose B mais absent de A, sa valeur initiale provient-elle de la pose courante du modèle, de la dernière valeur du clip, ou d'une valeur de repos ? Même question à la fin, au redémarrage et en cas d'arrêt.

Ce choix détermine la reproductibilité à temps arbitraire, la compatibilité avec des changements manuels de pose et les éventuels conflits entre animations. Il faut fixer une règle commune avant les signatures définitives.

## 8. Animations simultanées — question ouverte

Cas soulevé : **marcher tout en saluant**.

Deux orientations possibles, à arbitrer :
1. une animation active à la fois (première version plus simple) ;
2. plusieurs animations simultanées ciblant des canaux distincts, avec politique explicite pour les conflits.

Ne pas introduire silencieusement du blending, des layers, des priorités ou des masques tant que ce besoin n'est pas approuvé. Une politique de composition des canaux est indispensable si l'option 2 est retenue.

## 9. Contraintes de cycle de vie et d'intégrité

- L'animation de l'`ObjectHeadless` doit fonctionner même sans moteur graphique.
- La duplication d'un `ObjectHeadless` doit conserver **indépendamment** les translations, échelles et rotations de la pose en cours ; les moves/attachements/détachements ne doivent pas casser les caches ou les références.
- Une pose doit cibler les bons noms d'éléments et couples de points d'attachement ; erreurs explicites pour références inconnues/ambiguës.
- Les valeurs non finies, les échelles invalides selon la politique retenue, et les durées invalides doivent être refusées avant de laisser un état incohérent.
- Ne pas introduire de nouvel ordonnanceur, nouveau renderer voxel, cache GPU ni dépendance d'`Animation` envers `Object<TGraphics>`.
- Les transformations se lisent dans les matrices utilisées par la production de snapshots, sans contourner l'invalidation existante.
- Ne pas supposer qu'un `Model3D::Blueprint` est directement accepté par `Voxel::Renderer` : les voies PR08 et PR13 sont actuellement distinctes.

## 10. Tests d'acceptation à prévoir lors de l'implémentation

### Core / headless
- pose de repos, translation/échelle locale et composition `M_anchor × T × S` ;
- rotation d'articulation avec translation locale d'un élément enfant ;
- calcul des matrices et invalidation ciblée des caches ;
- absence de modification de la transformation ECS après squat ;
- poses partielles : propriété non ciblée inchangée selon la politique décidée ;
- interpolation : `lerp` translation/échelle, `slerp` rotation et valeurs de borne ;
- durée totale, passage de plusieurs étapes sur un grand `deltaTime`, résultats identiques pour différents découpages temporels ;
- activité du Behaviour, arrêt/reprise/boucle selon les politiques approuvées ;
- copies, moves, attache/détache de sous-`ObjectHeadless`, refs invalides, durées et valeurs invalides.

### Graphics (si le rendu existant est affecté)
- personnage articulé en marche, salut et squat, avec arme attachée ;
- cohérence de la pose affichée par `Object<TGraphics>` sans changer les règles de snapshot ;
- golden images via les outils Sparkle, avec comparaison/revue humaine ;
- tests Debug/Release ; pas d'appels OpenGL bruts dans les TU.

Les scénarios graphiques ne doivent pas imposer la création d'un nouveau chemin `Model3D` → `Voxel::Renderer`.

## 11. Hors périmètre initial

- cinématique inverse (IK), contraintes physiques, locomotion ECS automatique/root motion ;
- système générique de rig/skeleton séparé de `Model3D` ;
- refonte du renderer voxel, des snapshots, de l'ECS ou du moteur graphique ;
- import de formats externes d'animation et éditeur de timeline ;
- interpolation directe des matrices, ou rotation dynamique ajoutée aux éléments.

## 12. Décisions restantes pour notre discussion

1. **Propagation du scale :** échelle hiérarchique incluant les descendants/attachements, ou échelle visuelle limitée à l'élément ?
2. **Lecture simultanée :** une animation unique au départ ou plusieurs clips indépendants, et comment résoudre leurs conflits ?
3. **Canaux partiels :** valeur initiale d'une propriété absente des états adjacents, maintien en fin de clip et interaction avec les modifications manuelles ?
4. **Contrôle de lecture :** `play`/`stop`/`pause`, fin de clip, bouclage, vitesse, changement de clip et politique de transition.
5. **Validation :** échelles nulles/négatives, durées nulles, animation vide et correspondance clip/modèle.
6. **Interface publique exacte :** objets valeurs et setters à exposer sur `ElementNode`, type de clé d'articulation et modèle de propriété/liaison dans `Animator`.

**Ne pas présenter ces points comme actés.** Cette fiche constitue la base de la prochaine discussion d'architecture, pas un mandat pour coder immédiatement.

## Références

- [Architecture PR08 implémentée](IMPLEMENTED-01-08.md)
- [Model3D Object actuel](PR-08.4b-model3d-object.md)
- [Architecture générale de la roadmap](README.md)
- [Guide de développement Sparkle](../../ai-development-guidelines.md)
- Sources techniques : `include/model3d/{blueprint,object_headless,object}.hpp`, `src/model3d/{object_headless,object_attachment_node}.cpp`, `include/engine/behaviour.hpp`, `include/core/context/update_context.hpp`.
