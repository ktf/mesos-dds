#ifndef DDSScheduler_H
#define DDSScheduler_H

#include <vector>
#include <deque>
#include <condition_variable>
#include <mutex>

#include "boost/foreach.hpp"

#include <mesos/scheduler.hpp>
#include <mesos/resources.hpp>

struct DDSSubmitInfo {
    uint32_t m_nInstances;          ///< Number of instances.
    std::string m_cfgFilePath;      ///< Path to the configuration file.
    std::string m_id;               ///< ID for communication with DDS commander.
    std::string m_wrkPackagePath;   ///< A full path of the agent worker package, which needs to be deployed.
};

class DDSScheduler
  : public mesos::Scheduler 
{
public:
    
    DDSScheduler(std::condition_variable& mesosStarted,
                const mesos::ExecutorInfo& executorInfo, 
                const mesos::Resources& resourcesPerTask,
                const mesos::ContainerInfo& containerInfo);

    /*
     * Empty virtual destructor (necessary to instantiate subclasses).
     */
    virtual ~DDSScheduler() override;

    /*
     * Invoked when the scheduler successfully registers with a Mesos
     * master. A unique ID (generated by the master) used for
     * distinguishing this framework from others and MasterInfo
     * with the ip and port of the current master are provided as arguments.
     */
    virtual void registered(mesos::SchedulerDriver* driver,
                            const mesos::FrameworkID& frameworkId,
                            const mesos::MasterInfo& masterInfo) override;

    /*
     * Invoked when the scheduler re-registers with a newly elected Mesos master.
     * This is only called when the scheduler has previously been registered.
     * MasterInfo containing the updated information about the elected master
     * is provided as an argument.
     */
    virtual void reregistered(mesos::SchedulerDriver* driver,
                              const mesos::MasterInfo& masterInfo) override;

    /*
     * Invoked when the scheduler becomes "disconnected" from the master
     * (e.g., the master fails and another is taking over).
     */
    virtual void disconnected(mesos::SchedulerDriver* driver) override;

    /*
     * Invoked when resources have been offered to this framework. A
     * single offer will only contain resources from a single slave.
     * Resources associated with an offer will not be re-offered to
     * _this_ framework until either (a) this framework has rejected
     * those resources (see SchedulerDriver::launchTasks) or (b) those
     * resources have been rescinded (see Scheduler::offerRescinded).
     * Note that resources may be concurrently offered to more than one
     * framework at a time (depending on the allocator being used). In
     * that case, the first framework to launch tasks using those
     * resources will be able to use them while the other frameworks
     * will have those resources rescinded (or if a framework has
     * already launched tasks with those resources then those tasks will
     * fail with a TASK_LOST status and a message saying as much).
     */
    virtual void resourceOffers(mesos::SchedulerDriver* driver,
                                const std::vector<mesos::Offer>& offers) override;

    /*
     * Invoked when an offer is no longer valid (e.g., the slave was
     * lost or another framework used resources in the offer). If for
     * whatever reason an offer is never rescinded (e.g., dropped
     * message, failing over framework, etc.), a framework that attempts
     * to launch tasks using an invalid offer will receive TASK_LOST
     * status updats for those tasks (see Scheduler::resourceOffers).
     */
    virtual void offerRescinded(mesos::SchedulerDriver* driver,
                                const mesos::OfferID& offerId) override;

    /*
     * Invoked when the status of a task has changed (e.g., a slave is
     * lost and so the task is lost, a task finishes and an executor
     * sends a status update saying so, etc). If implicit
     * acknowledgements are being used, then returning from this
     * callback _acknowledges_ receipt of this status update! If for
     * whatever reason the scheduler aborts during this callback (or
     * the process exits) another status update will be delivered (note,
     * however, that this is currently not true if the slave sending the
     * status update is lost/fails during that time). If explicit
     * acknowledgements are in use, the scheduler must acknowledge this
     * status on the driver.
     */
    virtual void statusUpdate(mesos::SchedulerDriver* driver,
                              const mesos::TaskStatus& status) override;

    /*
     * Invoked when an executor sends a message. These messages are best
     * effort; do not expect a framework message to be retransmitted in
     * any reliable fashion.
     */
    virtual void frameworkMessage(mesos::SchedulerDriver* driver,
                                  const mesos::ExecutorID& executorId,
                                  const mesos::SlaveID& slaveId,
                                  const std::string& data) override;

    /*
     * Invoked when a slave has been determined unreachable (e.g.,
     * machine failure, network partition). Most frameworks will need to
     * reschedule any tasks launched on this slave on a new slave.
     */
    virtual void slaveLost(mesos::SchedulerDriver* driver,
                           const mesos::SlaveID& slaveId) override;

    /*
     * Invoked when an executor has exited/terminated. Note that any
     * tasks running will have TASK_LOST status updates automagically
     * generated.
     */
    virtual void executorLost(mesos::SchedulerDriver* driver,
                              const mesos::ExecutorID& executorId,
                              const mesos::SlaveID& slaveId,
                              int status) override;

    /*
     * Invoked when there is an unrecoverable error in the scheduler or
     * scheduler driver. The driver will be aborted BEFORE invoking this
     * callback.
     */
    virtual void error(mesos::SchedulerDriver* driver, const std::string& message) override;

public:

    // Setters
    void addAgents(const DDSSubmitInfo& submit);

private:

    // Condition variable to synchronise with DDS
    std::condition_variable& mesosStarted;

    // Mutex to protect method calls
    std::mutex ddsMutex;

    const mesos::ExecutorInfo& executorInfo;
    const mesos::Resources& resourcesPerTask;
    const mesos::ContainerInfo& containerInfo;

    // Queues and Lists
    std::deque<mesos::TaskInfo> waitingTasks;
    std::vector<mesos::TaskInfo> runningTasks;
    std::vector<mesos::TaskStatus> finishedTasks;
};

#endif  /* DDSScheduler_H */