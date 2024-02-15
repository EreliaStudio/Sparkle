#include "design_pattern/spk_notifier.hpp"
#include "spk_basic_functions.hpp"

namespace spk
{
    Notifier::Notifier() :
        _deactivated(false)
    {

    }

    Notifier::~Notifier()
    {
        /*
        Deactivating the Notifier ensures that none of the other methods will
        even try to acquire the mutex, and will instead abort instantly.
        */
        if (_deactivated == false)
        {
            _deactivated = true;

            std::unique_lock<std::mutex> contractsLk(_mu);

            for (Contract *contract : _contracts)
            {
                contract->cancel();
            }

            for (Contract *contract : _inactiveContracts)
            {
                contract->cancel();
            }
        }
    }

    Notifier::Notifier(Notifier&& p_other) :
        _contracts(std::move(p_other._contracts)),
        _inactiveContracts(std::move(p_other._inactiveContracts)),
        _deactivated(p_other._deactivated.load())
    {
        for (size_t i = 0; i < _contracts.size(); i++)
        {
            _contracts[i]->_subject = this;
        }

        for (size_t i = 0; i < _inactiveContracts.size(); i++)
        {
            _inactiveContracts[i]->_subject = this;
        }

        p_other._deactivated = true;

        p_other._contracts.clear();
        p_other._inactiveContracts.clear();
    }

    Notifier& Notifier::operator=(Notifier&& p_other)
    {
        if (this != &p_other)
        {
            _contracts.clear();
            _inactiveContracts.clear();

            _contracts = std::move(p_other._contracts);
            for (size_t i = 0; i < _contracts.size(); i++)
            {
                _contracts[i]->_subject = this;
            }
            _inactiveContracts = std::move(p_other._inactiveContracts);
            for (size_t i = 0; i < _inactiveContracts.size(); i++)
            {
                _inactiveContracts[i]->_subject = this;
            }
            _deactivated = p_other._deactivated.load();

            p_other._contracts.clear();
            p_other._inactiveContracts.clear();

            p_other._deactivated = true;
        }
        return *this;
    }

    std::unique_ptr<Notifier::Contract> Notifier::subscribe(const Callback& p_callback)
    {
        if (_deactivated == true)
        {
            /*
            This can happen if this method is called during deletion.
            Instead of returning a nullptr (that would require checking for nullptr
            each time it is called), we instead return an already cancelled contract,
            that will never be used. It effectively can never be used since this
            Notifier is being destroyed.
            */
            return (std::unique_ptr<Contract>(new Contract(nullptr, nullptr)));
        }

        std::unique_lock<std::mutex> lk(_mu);

        auto contract = std::unique_ptr<Contract>(new Contract(this, p_callback));

        _contracts.push_back(contract.get());

        return std::move(contract);
    }

    void Notifier::notify_all()
    {
        if (_deactivated)
        {
            return;
        }

        std::unique_lock<std::mutex> lk(_mu);

        for (Contract *contract : _contracts)
        {
            contract->notify();
        }
    }

    size_t Notifier::nbContracts() const
    {
        return (_contracts.size());
    }

    void Notifier::unsubscribe(Contract *p_contract)
    {
        if (_deactivated)
        {
            return;
        }

        std::unique_lock<std::mutex> lk(_mu);

        auto itActive = std::find(_contracts.begin(), _contracts.end(), p_contract);
        if (itActive != _contracts.end())
        {
            _contracts.erase(itActive);
            return;
        }

        auto itInactive = std::find(_inactiveContracts.begin(), _inactiveContracts.end(), p_contract);
        if (itInactive != _inactiveContracts.end())
        {
            _inactiveContracts.erase(itInactive);
            return;
        }
    }

    void Notifier::pause(Contract *p_contract)
    {
        if (_deactivated)
        {
            return;
        }

        std::unique_lock<std::mutex> lk(_mu);

        auto it = std::find(_contracts.begin(), _contracts.end(), p_contract);
        if (it == _contracts.end())
        {
            // The element was already deactivated, or has been unregistered.
            return;
        }

        _contracts.erase(it);
        _inactiveContracts.push_back(p_contract);
    }

    void Notifier::resume(Contract *p_contract)
    {
        if (_deactivated)
        {
            return;
        }

        std::unique_lock<std::mutex> lk(_mu);
        auto it = std::find(_inactiveContracts.begin(), _inactiveContracts.end(), p_contract);

        if (it == _inactiveContracts.end())
        {
            // The element is already active, or has been unregistered.
            return;
        }

        _inactiveContracts.erase(it);
        _contracts.push_back(p_contract);
    }

    Notifier::Contract::Contract() :
        _subject(nullptr),
        _callback(nullptr),
        _isCanceled(true),
        _isPaused(true)
    {

    }

    Notifier::Contract::Contract(Notifier *p_subject, const Callback& p_callback) :
        _subject(p_subject),
        _callback(p_callback),
        _isCanceled((p_subject == nullptr ? true : false)),
        _isPaused(false)
    {

    }

    bool Notifier::Contract::isCanceled() const
    {
        return (_isCanceled);
    }

    void Notifier::Contract::notify()
    {
        if (_isCanceled)
        {
            return;
        }

        _callback();
    }

    bool Notifier::Contract::isPaused() const
    {
        return (_isPaused);
    }

    void Notifier::Contract::pause()
    {
        if (_isCanceled)
        {
            return;
        }

        _isPaused = true;
        _subject->pause(this);
    }

    void Notifier::Contract::resume()
    {
        if (_isCanceled)
        {
            return;
        }

        _subject->resume(this);
    }

    void Notifier::Contract::cancel()
    {
        /*
        Only true if _isCanceled is false. The value is exchanged for true the first time it happens.
        This is used to guarantee that _subject->unregister cannot be called more than once.
        */
        bool checker = false;
        if (_isCanceled.compare_exchange_strong(checker, true))
        {
            _subject->unsubscribe(this);
        }
    }

    Notifier::Contract::~Contract()
    {
        cancel();
    }
}
