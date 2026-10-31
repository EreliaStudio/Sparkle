# PR-02 — Volume dense et Builder

> **Fiche de préparation pour une future PR GitHub, pas une PR déjà créée.** Ne développer que cette fiche.
>
> **Branche source :** `impl/v0.1.4/02-voxel-volume`, à créer depuis la **tête courante de `V0.1.4`** après fusion des prérequis. **Branche cible de la PR GitHub : `V0.1.4`**. Ne jamais cibler `main` ni partir d'une autre branche de feature.

**Objectif :** Éditer une grille voxel puis publier des snapshots immuables.

**Prérequis :** [PR-01](PR-01-voxel-foundations.md)

## Spécifications normatives

- [02-modele-voxel.md](../02-modele-voxel.md)
- [10-contrats-et-validation.md](../10-contrats-et-validation.md)
- [Spécification principale](../../voxel-engine-v0.1.4-specification.md)
- [Contrats et scénarios d'acceptation](../10-contrats-et-validation.md)

## Périmètre exact à réaliser

- [ ] Volume::Builder depuis dimensions ou Volume existant
- [ ] grille dense compacte
- [ ] table locale de Definitions
- [ ] Air index 0
- [ ] lecture Cell par valeur
- [ ] set et erase
- [ ] tailles immuables.

## Instructions techniques et intégration

Indexation et allocation sans débordement; déduplication par identité Definition::Data; copie légère des snapshots; toute exception build laisse Builder inchangé et réessayable; pas de resize ni de span<Cell>.

Avant de coder, inspecter la forme **actuelle** des classes, constructions, tests et CMake ; réutiliser `Mesh<T>`, `Registry`, `ContractProvider`, `ThreadSafeSlot`, `GPUResourceCollection`, `RenderSnapshot::Builder`, `ShaderStorageBuffer`, `spk::Exception` selon pertinence. Les exemples d'API dans les spécifications sont parfois indicatifs : la signature finale doit respecter les invariants sans créer une abstraction parallèle.

## Extension demandée pendant PR-02 : fichier compact et collection nommée

`DefinitionRegistry` est une collection **possédée par l'application**, non globale, associant des IDs entiers non signés (29 bits utiles) à des `Definition`. L'ID 0 est Air et ne peut pas être redéfini. Les autres IDs doivent être déclarés explicitement, uniques et valides. Le Builder peut copier une collection via `bind(registry)` ; toute modification ultérieure de la collection initiale n'affecte pas le Builder. `set(position, id, orientation...)` utilise cette collection.

Le constructeur `Volume::Builder(dimensions, filename, registry)` et `Builder::loadPacked(filename)` lisent un fichier **brut sans en-tête** de **4 octets par cellule**, en little-endian. Les dimensions sont fournies par l'appelant, pas devinées depuis le fichier. Chaque mot correspond à 2 bits de rotation horizontale, 1 bit d'inversion verticale, et 29 bits d'ID de Definition dans la collection fournie. Le parcours est X le plus rapide, puis Y, puis Z. Un fichier doit contenir exactement `x*y*z*4` octets ; une taille différente, un ID non défini ou une erreur de lecture rejette toute l'opération. Le constructeur échoue sans publier de Volume partiel ; `loadPacked` conserve le Builder complètement inchangé en cas d'échec. L'import convertit les IDs externes en **indices locaux dédupliqués par identité** ; il ne suppose jamais que l'ID externe est l'indice interne du Volume.

Ce format volontairement minimal ne transporte ni dimensions, ni identités de Definitions, ni version. Le fichier et le registre correspondant doivent être fournis ensemble ; aucun `TightData` public ou serialization générique n'est introduit.

## Tests obligatoires

- [ ] Initialisation Air partout
- [ ] duplication par identité et non par contenu
- [ ] orientation maintenue
- [ ] set/erase et hors bornes
- [ ] overflow
- [ ] ancien Volume après nouveau build
- [ ] construction échouée non destructive.

**Non-régression commune :** conserver la compilation et les tests existants. Lancer `cmake --workflow --preset testDebug` et `cmake --workflow --preset testRelease` pour les configurations prises en charge, en plus des builds Core sans Graphics lorsque pertinent. Tests GPU et golden-image sur l'infrastructure graphique existante (Windows/WGL) ; ne pas remplacer automatiquement une golden image de référence.

## Définition de terminé / critère de fusion

Volume de plusieurs Definitions relu sans Engine ni OpenGL.

- [ ] Tous les invariants et erreurs publiques ci-dessus sont testés et documentés ; aucun état partiel n'est publié après un échec lorsqu'une garantie forte est prescrite.
- [ ] Les dépendances listées sont disponibles sur la branche de base ; aucune fonctionnalité des fiches suivantes n'est embarquée sans nécessité stricte.
- [ ] La PR présente le périmètre, les tests réellement exécutés et les résultats, les screenshots/diffs si graphiques, et les limitations éventuelles.
- [ ] Aucune fusion sans validation du propriétaire.

## Hors périmètre

chunking, grille sparse, streaming, meshing.

## Consigne de passation

Confier **uniquement PR-02** à l'IA. Les détails locaux (signatures C++, choix numérique aux bornes, états GPU) peuvent être finalisés pendant l'implémentation avec tests. Ne pas réouvrir les choix validés ni recréer le registre de questions supprimé. En cas de contradiction fonctionnelle publique **démontrée**, soumettre au plus une question ciblée ; poursuivre les éléments indépendants. [Prompt de démarrage générique](AI-HANDOFF.md).
