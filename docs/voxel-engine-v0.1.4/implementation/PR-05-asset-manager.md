# PR-05 — asset::Manager et asset::Factory typés

> **Fiche de préparation pour une future PR GitHub, pas une PR déjà créée.** Ne développer que cette fiche.
>
> **Branche source :** `impl/v0.1.4/05-asset-manager`, à créer depuis la **tête courante de `V0.1.4`** après fusion des prérequis. **Branche cible de la PR GitHub : `V0.1.4`**. Ne jamais cibler `main` ni partir d'une autre branche de feature.

**Objectif :** Charger et posséder des assets C++ génériques à identité stable.

**Prérequis :** Aucune PR d'implémentation requise.

## Spécifications normatives

- [05-asset-manager.md](../05-asset-manager.md)
- [10-contrats-et-validation.md](../10-contrats-et-validation.md)
- [Spécification principale](../../voxel-engine-v0.1.4-specification.md)
- [Contrats et scénarios d'acceptation](../10-contrats-et-validation.md)

## Périmètre exact à réaliser

- [ ] asset::Manager::load<T>
- [ ] asset::Factory<T> statique
- [ ] clé type C++, id et JSON structurel
- [ ] config omise = objet JSON vide
- [ ] construction directe de T non déplaçable
- [ ] dépendances et cycles
- [ ] clear().
- [ ] get<T>(id) sans config : retourne l'unique instance déjà chargée ou lève spk::Exception si absente ou ambiguë.

## Instructions techniques et intégration

Factory retourne T par valeur et peut recharger U dans le même gestionnaire; cache ne publie qu'asset achevé; adresses stables; échec nettoie pile active mais garde dépendances déjà réussies; application synchronise accès concurrents.

Avant de coder, inspecter la forme **actuelle** des classes, constructions, tests et CMake ; réutiliser `Mesh<T>`, `Registry`, `ContractProvider`, `ThreadSafeSlot`, `GPUResourceCollection`, `RenderSnapshot::Builder`, `ShaderStorageBuffer`, `spk::Exception` selon pertinence. Les exemples d'API dans les spécifications sont parfois indicatifs : la signature finale doit respecter les invariants sans créer une abstraction parallèle.

## Tests obligatoires

- [ ] get<T>(id) sans création, exception si absent ou si deux configurations du même type/id sont chargées ; retour après clear() également refusé.

- [ ] Deux configs 16x16 et 8x8 distinctes
- [ ] ordre des clés JSON indifférent
- [ ] null distinct de {}
- [ ] T ni copiable ni déplaçable
- [ ] cycle direct/indirect
- [ ] réessai après exception
- [ ] clear et emprunts invalidés.

**Non-régression commune :** conserver la compilation et les tests existants. Lancer `cmake --workflow --preset testDebug` et `cmake --workflow --preset testRelease` pour les configurations prises en charge, en plus des builds Core sans Graphics lorsque pertinent. Tests GPU et golden-image sur l'infrastructure graphique existante (Windows/WGL) ; ne pas remplacer automatiquement une golden image de référence.

## Définition de terminé / critère de fusion

Trois load identiques renvoient la même référence et les configs différentes deux objets.

- [ ] Tous les invariants et erreurs publiques ci-dessus sont testés et documentés ; aucun état partiel n'est publié après un échec lorsqu'une garantie forte est prescrite.
- [ ] Les dépendances listées sont disponibles sur la branche de base ; aucune fonctionnalité des fiches suivantes n'est embarquée sans nécessité stricte.
- [ ] La PR présente le périmètre, les tests réellement exécutés et les résultats, les screenshots/diffs si graphiques, et les limitations éventuelles.
- [ ] Aucune fusion sans validation du propriétaire.

## Hors périmètre

factory registries dynamiques, LRU, thread-safety automatique.

## Consigne de passation

Confier **uniquement PR-05** à l'IA. Les détails locaux (signatures C++, choix numérique aux bornes, états GPU) peuvent être finalisés pendant l'implémentation avec tests. Ne pas réouvrir les choix validés ni recréer le registre de questions supprimé. En cas de contradiction fonctionnelle publique **démontrée**, soumettre au plus une question ciblée ; poursuivre les éléments indépendants. [Prompt de démarrage générique](AI-HANDOFF.md).
