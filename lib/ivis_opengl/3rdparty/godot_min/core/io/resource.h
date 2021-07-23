/*
	This file is part of Warzone 2100.
	Copyright (C) 2021  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

// Only used to provide a very simplified Ref implementation

//template<typename T>
//using Ref = std::shared_ptr<T>;
#include <memory>

template<typename T>
class Ref : private std::shared_ptr<T>
{
public:
//	Ref() : std::shared_ptr<T>() { }
	using std::shared_ptr<T>::get;
	using std::shared_ptr<T>::operator->;
	inline bool is_valid() const { return std::shared_ptr<T>::get() != nullptr; }
	inline bool is_null() const { return std::shared_ptr<T>::get() == nullptr; }
	void instance()
	{
		std::shared_ptr<T>::reset(new T());
	}
};
