# PR00 — LifeCycle, Entity/Widget et Pool Lease
**Couche :** Core. **Dépendances :** aucune. **État :** cahier des charges, sans C++.

## Objectif
Créer un socle de notifications de durée de vie pour les entités headless et les objets recyclables ; ne pas inventer de nouveau mécanisme de possession.

## Contrat
- spk::LifeCycle expose trois événements indépendants : Acquisition, Release, Destruction. Aucune notification d'acquisition lors du constructeur ; l'objet commence acquis.
- Transitions acquire()/release() idempotentes, protégées, notification immédiate en cas de changement réel. Aucun appel utilisateur arbitraire permettant de contourner la possession d'un Pool.
- À la destruction : Release si acquis, puis Destruction, sans laisser sortir d'exception du destructeur. Finalisation anticipée dans les destructeurs Entity/Widget avant destruction des membres ; secours dans ~LifeCycle si nécessaire.
- Migrer Entity (donc Entity3D) et Widget. Supprimer l'ancien fournisseur de destruction Widget et utiliser exclusivement LifeCycle::subscribeToDestruction (callback sans argument).
- Dans Pool<T>, seulement si T dérive de LifeCycle : objet neuf normalisé Released, Acquisition à l'obtention, Release au retour de Lease, puis destruction conforme. Le Pool générique pour int et autres types reste inchangé.
- Conserver la mécanique existante de Pool::Lease : élément + weak_ptr<State>, recyclage si State existe, delete sinon. Une Lease ne prolonge pas la durée de vie du Pool.
- Ne jamais appeler un abonné arbitraire sous le verrou interne du Pool.

## Tests obligatoires
Ordre Release/Destruction, idempotence, acquérir/libérer plusieurs fois, nouvel objet et pool réutilisé, Lease survivant au Pool, abonnements de destruction Widget sans adaptateur de compatibilité, Entity3D destruction avec composants, callbacks réentrants et exceptions, aucun impact sur Pool<int>.

## Limites
Aucune classe Chunk, aucun rendu et aucune refonte de WorkerPool. Ce contrat est une dépendance de PR02 et PR08.
