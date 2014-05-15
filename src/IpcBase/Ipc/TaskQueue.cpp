#include "TaskQueue.h"


TaskQueue::TaskQueue()
	: stop_(true)
	, pause_(false)
	, runing_threads_count_(0)
{
}

TaskQueue::~TaskQueue()
{
}

void TaskQueue::Start(int thread_cnt)
{
	try
	{
		boost::this_thread::disable_interruption di;

		boost::mutex::scoped_lock lck(start_stop_mutex_);
		if (!stop_)return;

		stop_=false;
		threads_.resize(thread_cnt);
		for (int i=0;i<thread_cnt;++i)
			threads_[i]=new boost::thread(boost::bind(&TaskQueue::ExecLoop,this));

		//确保线程启动
		while(runing_threads_count_ < thread_cnt)
		{
			boost::this_thread::sleep(boost::posix_time::milliseconds(1));
		}
	}
	catch (...)
	{
	}
}

void TaskQueue::Stop()
{
	try
	{
		boost::this_thread::disable_interruption di;

		boost::mutex::scoped_lock lck(start_stop_mutex_);
		if (stop_)return;

		stop_=true;
		pause_=false;

		condition_.notify_all();
		for (size_t i=0;i<threads_.size();++i)
		{
			boost::thread* threadPtr=threads_[i];
			if (threadPtr->joinable())
			{
				threadPtr->join();
			}
			delete threadPtr;
		}
		threads_.clear();
		task_que_.clear();
		runing_threads_count_=0;
	}
	catch (...)
	{
	}
}

void TaskQueue::Reset()
{
	boost::mutex::scoped_lock lck(work_mutex_);
	task_que_.clear();
}

void TaskQueue::Pause()
{
	pause_=true;
}

void TaskQueue::Resume()
{
	pause_=false;
	condition_.notify_one();
}

void TaskQueue::Post(const TaskFunction& h, int priority)
{
	{
		boost::mutex::scoped_lock lck(work_mutex_);
		task_que_.insert(HandlerElm(h, priority));
	}
	condition_.notify_one();
}

void TaskQueue::ExecLoop()
{
	try
	{
		boost::this_thread::disable_interruption di;
		++runing_threads_count_;

		for(;!stop_;)
		{
			boost::mutex::scoped_lock lck(work_mutex_);
			while((task_que_.empty() || pause_) && !stop_)
			{
				condition_.wait_for(lck, boost::chrono::milliseconds(1000));
			}

			if(task_que_.size()!=0)
			{
				TaskContainer::iterator itr=task_que_.begin();
				TaskContainer::const_reference handlerElm=*itr;
				TaskFunction h = handlerElm.handler;
				//h.swap(handlerElm.handler);
				task_que_.erase(itr);

				lck.unlock();
				try
				{
					h();
				}
				catch (...)
				{
				}
			}
		}
	}
	catch (...)
	{
	}
}