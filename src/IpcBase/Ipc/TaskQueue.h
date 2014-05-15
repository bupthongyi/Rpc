#ifndef _TASK_QUEUE_H__
#define _TASK_QUEUE_H__
#include <set>

#include <boost/thread/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/pool/pool_alloc.hpp>
#include <boost/function/function0.hpp>
#include <boost/atomic.hpp>

class TaskQueue
{
	typedef boost::function0<void> TaskFunction;
	struct HandlerElm{
		int priority;
		TaskFunction handler;
		HandlerElm(const TaskFunction& h,int p)
			: priority(p)
			, handler(h)
		{}
	};

	struct PriorityGreater
	{
		bool operator()(const HandlerElm&e1,const HandlerElm& e2)const
		{
			return e1.priority>e2.priority;
		}
	};
	typedef std::multiset<HandlerElm,PriorityGreater,
		boost::fast_pool_allocator<HandlerElm> > TaskContainer;
public:
	TaskQueue();
	virtual ~TaskQueue();
	void Start(int thread_cnt=1);
	void Stop();
	void Reset();
	void Pause();
	void Resume();

	void Post(const TaskFunction& h, int priority=0);

private:
	void ExecLoop();

private:
	mutable boost::mutex start_stop_mutex_;
	mutable boost::mutex work_mutex_;

	boost::condition	condition_;
	boost::atomic_bool	stop_;
	boost::atomic_bool	pause_;

	boost::atomic_int			runing_threads_count_;
	std::vector<boost::thread*> threads_;
	TaskContainer				task_que_;
};
#endif