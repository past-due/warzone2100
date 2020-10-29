///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2015 Microsoft Corporation. All rights reserved.
//
// This code is licensed under the MIT License (MIT).
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef __INCLUDED_GSL_FINALLY_H__
#define __INCLUDED_GSL_FINALLY_H__

namespace gsl {
	// final_act allows you to ensure something gets run at the end of a scope
	template <class F>
	class final_act
	{
	public:
		explicit final_act(F f) noexcept : f_(std::move(f)), invoke_(true) {}

		final_act(final_act&& other) noexcept : f_(std::move(other.f_)), invoke_(other.invoke_)
		{
			other.invoke_ = false;
		}

		final_act(const final_act&) = delete;
		final_act& operator=(const final_act&) = delete;

		~final_act() noexcept
		{
			if (invoke_) f_();
		}

	private:
		F f_;
		bool invoke_;
	};

	// finally() - convenience function to generate a final_act
	template <class F>
	inline final_act<F> finally(const F& f) noexcept
	{
		return final_act<F>(f);
	}

	template <class F>
	inline final_act<F> finally(F&& f) noexcept
	{
		return final_act<F>(std::forward<F>(f));
	}
}

#endif // __INCLUDED_GSL_FINALLY_H__
