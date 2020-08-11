#ifndef FACT_StateMachineAsio
#define FACT_StateMachineAsio

#include <boost/asio.hpp>
#include <boost/bind.hpp>

template <class T>
class StateMachineAsio : public T, public boost::asio::io_service, public boost::asio::io_service::work
{
    boost::asio::deadline_timer fTrigger;

    void HandleTrigger(const boost::system::error_code &error)
    {
        // 125: Operation canceled (bs::error_code(125, bs::system_category))
        if (error && error!=boost::asio::error::basic_errors::operation_aborted)
            return;

        fTrigger.expires_from_now(boost::posix_time::microseconds(10000));
        fTrigger.async_wait(boost::bind(&StateMachineAsio::HandleTrigger,
                                        this, boost::asio::placeholders::error));

        if (!T::HandleNewState(Execute(), 0, "by HandleTrigger()"))
            Stop(-1);
    }

    void Handler()
    {
        const auto ptr = T::PopEvent();
        if (!T::HandleEvent(*ptr))
            Stop(-1);
    }

    void PushEvent(Event *cmd)
    {
        T::PushEvent(cmd);
        post(boost::bind(&StateMachineAsio::Handler, this));
    }

    int Execute()=0;

    int Run(bool)
    {
        fTrigger.expires_from_now(boost::posix_time::microseconds(0));
        fTrigger.async_wait(boost::bind(&StateMachineAsio::HandleTrigger,
                                        this, boost::asio::placeholders::error));

        T::SetCurrentState(StateMachineImp::kSM_Ready, "by Run()");

        T::fRunning = true;

        while (run_one())
        {
            if (!T::HandleNewState(Execute(), 0, "by Run()"))
                Stop(-1);
        }
        reset();

        T::fRunning = false;

        if (T::fExitRequested==-1)
        {
            T::Fatal("Fatal Error occured... shutting down.");
            return -1;
        }

        T::SetCurrentState(StateMachineImp::kSM_NotReady, "due to return from Run().");

        const int exitcode = T::fExitRequested-1;
        T::fExitRequested = 0;
        return exitcode;
    }


public:
    StateMachineAsio(std::ostream &out, const std::string &server) :
        T(out, server), boost::asio::io_service::work(static_cast<boost::asio::io_service&>(*this)),
        fTrigger(static_cast<boost::asio::io_service&>(*this))
    {
        // ba::io_service::work is a kind of keep_alive for the loop.
        // It prevents the io_service to go to stopped state, which
        // would prevent any consecutive calls to run()
        // or poll() to do nothing. reset() could also revoke to the
        // previous state but this might introduce some overhead of
        // deletion and creation of threads and more.
    }

    void Stop(int code=0)
    {
        T::Stop(code);
        stop();
    }
};

#endif
