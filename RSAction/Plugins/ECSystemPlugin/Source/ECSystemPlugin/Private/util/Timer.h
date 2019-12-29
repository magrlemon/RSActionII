///-------------------------------------------------------------------------------------------------
/// File:	include\util\Timer.h.
///
/// Summary:	Declares the Timer class.
///-------------------------------------------------------------------------------------------------

#ifndef __TIMER_H___
#define __TIMER_H___

#include "API.h"
#include "Timer.generated.h"
namespace ECS { namespace util {

	class ECS_API Timer
	{
		using Elapsed = std::chrono::duration<f32, std::milli>;

	private:

		Elapsed m_Elapsed;

	public:

		Timer();
		~Timer();

		///-------------------------------------------------------------------------------------------------
		/// Fn:	void Timer::Tick(DurationRep ms);
		///
		/// Summary:	Advances timer by adding elapsed milliseconds.
		///
		/// Author:	magi
		///
		/// Date:	3/10/2019
		///
		/// Parameters:
		/// ms - 	milliseconds (can be fractions).
		///-------------------------------------------------------------------------------------------------

		void Tick(f32 ms);

		///-------------------------------------------------------------------------------------------------
		/// Fn:	void Timer::Reset();
		///
		/// Summary:	Resets this timer to zero.
		///
		/// Author:	magi
		///
		/// Date:	3/10/2019
		///-------------------------------------------------------------------------------------------------

		void Reset();

		///-------------------------------------------------------------------------------------------------
		/// Fn:	inline TimeStamp Timer::GetTimeStamp() const
		///
		/// Summary:	Gets a TimeStamp from current timer value.
		///
		/// Author:	magi
		///
		/// Date:	3/10/2019
		///
		/// Returns:	The time stamp.
		///-------------------------------------------------------------------------------------------------

		inline TimeStamp GetTimeStamp() const
		{
			return TimeStamp(this->m_Elapsed.count());
		}

	}; // class Timer

}} // namespace ECS::util

#endif // __TIMER_H___

