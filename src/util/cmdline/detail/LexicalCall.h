/*
 * Copyright 2013-2021 Arx Libertatis Team (see the AUTHORS file)
 *
 * This file is part of Arx Libertatis.
 *
 * Original source is copyright 2010 - 2011. Alexey Tsoy.
 * http://sourceforge.net/projects/interpreter11/
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef ARX_UTIL_CMDLINE_DETAIL_LEXICALCALL_H
#define ARX_UTIL_CMDLINE_DETAIL_LEXICALCALL_H

#include <functional>
#include <utility>

#include "util/cmdline/detail/ArgsAdapter.h"
#include "util/cmdline/detail/LFunction.h"

namespace util::cmdline {

template <typename FnSign>
class lexical_call_t;

template <typename Result, typename Args>
class lexical_call_t<Result(Args)> {
	
	typedef lexical_call_t<Result(Args)> self_t;
	
public:
	
	typedef Args argument_type;
	typedef Result result_type;
	
	template <typename FnSign, typename Function>
	static self_t construct(const Function & fn) {
		self_t ret;
		ret.set_(make_lfunction<FnSign>(fn));
		return ret;
	}
	
	template <typename Function>
	static self_t construct(Function * fn) {
		self_t ret;
		ret.set_(make_lfunction(fn));
		return ret;
	}
	
	template <typename FnSign, typename Function>
	static self_t construct(Function * fn) {
		self_t ret;
		ret.set_(make_lfunction<FnSign>(fn));
		return ret;
	}
	
	template <typename Function>
	static self_t construct(const Function & fn) {
		self_t ret;
		ret.set_(make_lfunction(fn, &Function::operator()));
		return ret;
	}
	
	result_type operator()(argument_type args) {
		return function(args);
	}
	
	result_type operator()(argument_type args) const {
		return function(args);
	}
	
	void swap(self_t & rh) {
		function.swap(rh.swap());
	}
	
private:
	
	template <typename Fn>
	struct proxy_function {
		
		Fn m_fn;
		
		explicit proxy_function(const Fn & fn) : m_fn(fn) {
		}
		
		result_type operator()(argument_type args) {
			detail::args_adapter<typename Fn::signature> decoded_args(args);
			return m_fn(decoded_args);
		}
		
	};
	
	template <typename Function>
	void set_(const Function & fn) {
		function = proxy_function<Function>(fn);
	}
	
	typedef std::function<Result(Args)> function_t;
	
	function_t function;
	
};

template <typename Result, typename ValueType, typename TypeCast>
class lexical_call_t<Result(ValueType, ValueType, TypeCast)> {
	
	typedef TypeCast type_cast_t;
	
	struct Args {
		
		type_cast_t & m_cast;
		
		Args(const Args &) = delete;
		Args & operator=(const Args &) = delete;
		
		explicit Args(type_cast_t & cast) : m_cast(cast) { }
		
		template <typename R>
		R front() {
			return m_cast.template cast<R>(v_front());
		}
		
		virtual ValueType v_front() const = 0;
		virtual void pop() {}
		virtual bool empty() const = 0;
		virtual bool opt_empty() const = 0;
		
		virtual ~Args() = default;
		
	};
	
	template <typename Iterator>
	struct VArgs : Args {
		
		Iterator & m_begin;
		Iterator m_optend;
		Iterator m_end;
		bool m_is_optend;
		
		VArgs(type_cast_t & cast, Iterator & begin, Iterator optend, Iterator end)
			: Args(cast)
			, m_begin(begin)
			, m_optend(optend)
			, m_end(end)
			, m_is_optend(begin == optend)
		{ }
		
		ValueType v_front() const override {
			return *m_begin;
		}
		
		void pop() override {
			++m_begin;
			m_is_optend = m_is_optend || (m_begin == m_optend);
		}
		
		bool empty() const override {
			return m_begin == m_end;
		}
		
		bool opt_empty() const override {
			return m_is_optend;
		}
		
	};
	
	typedef lexical_call_t<Result(Args &)> impl_t;
	typedef lexical_call_t self_t;
	impl_t  m_impl;
	
	explicit lexical_call_t(impl_t impl) : m_impl(std::move(impl)) { }
	
public:
	
	lexical_call_t() : m_impl() { }
	
	template <typename FnSign, typename Function>
	static self_t construct(const Function & fn) {
		return self_t(impl_t::template construct<FnSign>(fn));
	}
	
	template <typename Function>
	static self_t construct(Function * fn) {
		return self_t(impl_t::construct(fn));
	}
	
	template <typename Function>
	static self_t construct(const Function & fn) {
		return self_t(impl_t::construct(fn));
	}
	
	template <typename Iterator>
	Result operator()(Iterator & begin, Iterator optend, Iterator end, TypeCast & cast) {
		VArgs<Iterator> args(cast, begin, optend, end);
		return m_impl(args);
	}
	
	template <typename Iterator>
	Result operator()(Iterator & begin, Iterator optend, Iterator end, TypeCast & cast) const {
		VArgs<Iterator> args(cast, begin, optend, end);
		return m_impl(args);
	}
	
	template <typename Iterator>
	Result operator()(Iterator & begin, Iterator end, TypeCast & cast) {
		return operator()(begin, end, end, cast);
	}
	
	template <typename Iterator>
	Result operator()(Iterator & begin, Iterator end, TypeCast & cast) const {
		return operator()(begin, end, end, cast);
	}
	
};

} // namespace util::cmdline

#endif // ARX_UTIL_CMDLINE_DETAIL_LEXICALCALL_H
