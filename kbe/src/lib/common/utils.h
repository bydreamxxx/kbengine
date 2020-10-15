/*
This source file is part of KBEngine
For the latest info, see http://www.kbengine.org/

Copyright (c) 2008-2020 KBEngine.

KBEngine is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

KBEngine is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with KBEngine.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef KBE_THREAD_UTILS
#define KBE_THREAD_UTILS

#include <memory>

namespace KBEngine {

template<class T, class... Args>
std::unique_ptr<T> kbe_make_unique(Args... args) {
	return std::unique_ptr<T>(new T(std::forward<T>(args)...));
}

template<class T, class... Args>
std::shared_ptr<T> kbe_make_shared(Args... args) {
	return std::shared_ptr<T>(new T(std::forward<T>(args)...));
}

#define	KBE_MAKE_UNIQUE kbe_make_unique

#define	KBE_MAKE_SHARED kbe_make_shared

}	// end namespace KBEngine

#endif // !KBE_THREAD_UTILS
