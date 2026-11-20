# Sparkle v0.1.5 — Contrats normatifs proposés
Les décisions ci-dessous reprennent la discussion de conception ; les signatures exactes seront confirmées par les PR. Ne pas rétablir un ancien contrat contredisant cette table.

| Sujet | Contrat |
| --- | --- |
| Chunk | spk::Chunk<SX,SY,SZ> héritable, dimensions positives d'unités monde, coordonnée hors de l'objet |
| ChunkCoordinate | XYZ signés, conversion floor négatif, monde/local avec contrôles de débordement |
| Modification | TChunk est mutable ; chaque setter appelle explicitement notifyChunkEdition(), événement par instance |
| Copie/move | Un nouvel objet ne reprend jamais les abonnements source ; affectation conserve les abonnements propres à la destination |
| Collection | std::map ordonnée ChunkCoordinate → ChunkEntry possédant TChunk **par valeur** |
| Entity | Une Entity3D par chunk + Entity3D racine, disponibles en headless, préservées lors d'un remplacement |
| ChunkFilter | Component3D non propriétaire pointant vers le chunk de sa collection, sans double stockage, pointeur sûr pendant la vie du chunk |
| Lookup | at / operator[] renvoient référence ou spk::Exception ; tryGet pointeur nullable ; contains ; iteration [coord,entry] |
| Collection events | S'abonne à chaque chunk, relaie immédiatement édition + signal insertion/remplacement, avec coord ; pas de notification de suppression générique séparée |
| Remove | Entity3D/composants détruits avant chunk ; notification LifeCycle::Destruction disponible |
| Engine | Attach/detach/rebind explicite, une collection/monde par Engine, pas de mutation collection depuis worker |
| Generator | Generator<TChunk,TGenerationData> synchrone, addPass successifs ; Pass::apply(builder, coordinate, data) virtuel |
| Builder | Pas imposé par Chunk générique ; types utilisés avec Generator ont TChunk::Builder, build() consomme et retourne TChunk par valeur |
| Optional | _createChunkBuilder -> optional<Builder>, generate -> optional<TChunk> ; nullopt succès sans chunk |
| Generation Data | Nouvel objet par demande, peut référencer données persistantes de l'utilisateur ; seed et verrouillage non imposés |
| Erreur | spk::Exception lors de génération ; Task Failed distincte de réussite nullopt ; timeout supervisé par streaming |
| Requester | Task<std::optional<TChunk>>::Answer par chunk, sources locales/fichier/réseau possibles sans implémentation d'un protocole |
| Transfert Task | Task::Answer::result renvoie const& ; TChunk doit être copiable à la frontière réponse→collection en l'état |
| Worker | WorkerPool local, aucune manipulation de l'Engine sur worker ; dépendances des jobs maintenues en vie |
| Observer | WorldObserver Component3D, rayon XYZ en chunks, union de toutes les zones inclusives |
| Replanification | Refaire union et file uniquement au changement de coordonnée chunk d'un observateur, de rayon ou du jeu d'observateurs |
| Priorité | Distance Manhattan en unités monde pondérée SX/SY/SZ, minimum des observateurs intéressés ; égalités stables |
| Budget | Maximum configurable de Tasks réellement soumises non terminées, y compris expirées |
| Timeout | Résultat tardif ignoré, pas d'annulation ; retry sous budget, ID monotone et contrôle d'intérêt/version |
| Nullopt cache | Absent mémorisé seulement tant que coordonnée intéressée, oublié à la sortie, redemandé à la réentrée |
| Éviction | Tout chunk hors union retiré immédiatement ; pas de LRU |
| Voxel Chunk | Volume exact SX×SY×SZ, chunk mutable via setVolume et notifications, Volume lui-même immuable |
| ChunkMesher | Dérive de Mesher, outsideCell cherche voisins lors de chaque bake ; aucune conservation durable |
| Render sync | Bind scanne collection, événement immediate → dirty set, rebuild à render une fois par coord |
| Six voisins | À toute modification, reconstruire chunk + six voisins sans différentiel de frontières ; destruction invalide six voisins |
| Lifecycle | Acquisition/Release/Destruction distincts ; Widget, Entity et Pool<T> adaptés sans changer possession Lease |
| Exclusions | Ni save, collision, réseau, biomes/villes/routes/cavernes, ni gestion multimonde |

## Esquisse API (les types finaux peuvent varier)
~~~cpp
class GameChunk : public spk::Chunk<32, 16, 32> {
public:
    class Builder;
    void setSomething(Value value) {
        _value = std::move(value);
        notifyChunkEdition();
    }
private:
    Value _value;
};

spk::ChunkCollection<GameChunk> chunks;
GameChunk prepared = makeExampleChunk(); // Utilitaire illustratif du jeu
chunks.addChunk({1, 0, -2}, std::move(prepared));
GameChunk& chunk = chunks.at({1, 0, -2});
// L'Entity3D est accessible via l'entrée de la collection.

class GameGenerator : public spk::Generator<GameChunk, MyGenerationData> {
private:
    std::optional<GameChunk::Builder> _createChunkBuilder(const ChunkCoordinate&) override;
    MyGenerationData _createGenerationData(const ChunkCoordinate&) override;
};
// Pass::apply(GameChunk::Builder&, const ChunkCoordinate&, MyGenerationData&);
// generate(coordinate) -> std::optional<GameChunk>
// request(coordinate) -> spk::Task<std::optional<GameChunk>>::Answer
~~~

## Règles techniques
- C++23, spk::Exception, absence valide jamais signalée par exception. Aucune allocation de TChunk avec unique_ptr exigée du développeur.
- Le stockage par valeur doit préserver l'adresse du chunk d'une entrée existante. Une affectation ne transfère pas les abonnements d'une autre instance. Les tests PR01/02 valident le respect de ContractProvider.
- Task::Answer::subscribeToCompletion peut appeler immédiatement le callback si tâche déjà terminée ; un abonnement et sa réentrance doivent être testés. Task résultat const implique transfert par copie sans modification non approuvée de Task.
- L'arrêt du streamer ne doit jamais attendre les Tasks, mais WorkerPool contient des jthread dont la destruction peut attendre : ne pas promettre un shutdown global instantané.
- Un même résultat final ne doit jamais réécrire un chunk après sortie ou réentrée de la zone d'intérêt sans correspondance de version.
- Les implémentations devront préserver la possibilité d'utiliser ChunkCollection seule, sans Generator ou RenderSystem.

Pour les tests et responsabilités, consulter [les fiches PR](implementation/README.md), [les risques](architecture-review.md) et [la validation](validation.md).
